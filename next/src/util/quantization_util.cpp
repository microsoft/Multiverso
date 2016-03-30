
#include <multiverso/util/quantization_util.h>
#include <algorithm>
#include <multiverso\multiverso.h>

namespace multiverso{

    typedef float data_type;
    typedef long index_type;

	std::vector< multiverso::Blob> SparseFilter::FilterIn(const std::vector< multiverso::Blob> &blobs){
		std::vector< multiverso::Blob> result;

        for (auto iter = blobs.cbegin(); iter != blobs.cend(); iter++){
            Blob compressed_blob;
            auto compressed = try_compress(*iter, compressed_blob);
            Blob flag_blob(sizeof(data_type) * 2);
            flag_blob.As<long>(0) = compressed ? 1 : 0;            // compressed or not
            flag_blob.As<long>(1) = iter->size();                  // blob size
            result.push_back(flag_blob);
            result.push_back(compressed ? std::move(compressed_blob) : *iter);
        }

        return result;
	}

    std::vector< multiverso::Blob> SparseFilter::FilterOut(const std::vector< multiverso::Blob> &blobs){
        std::vector< multiverso::Blob> result;
        CHECK(blobs.size() % 2 == 0);

        for (auto i = 0; i < blobs.size(); i += 2)
        {
            auto is_compressed  = blobs[i].As<long>(0) == 1;
            auto size           = blobs[i].As<long>(1);
            auto& blob = blobs[i + 1];
            result.push_back(is_compressed ? de_compress(blob, size) : blob);
        }

        return result;
    }

    bool SparseFilter::try_compress(const multiverso::Blob& in_blob, multiverso::Blob& out_blob)
    {
        CHECK(sizeof(data_type) == sizeof(long));
        const auto& clip_value = this->clip_value;
        //auto array = static_cast<float*>(in_blob.data());
        auto data_count = in_blob.size<data_type>();
        auto non_zero_count = 0;
        for (auto i = 0; i < data_count; ++i){
            if (std::abs(in_blob.As<data_type>(i)) > clip_value) {
                ++non_zero_count;
            }
        }
            
        if (non_zero_count * 2 >= data_count)
            return false;

        if (non_zero_count == 0){
            // Blob does not support empty content, send the first one
            Blob result(2 * sizeof(data_type));
            result.As<long>(0) = 0;
            result.As<data_type>(1) = in_blob.As<data_type>(0);
            out_blob = result;
        }
        else{
            Blob result(non_zero_count * 2 * sizeof(data_type));
            auto result_index = 0;
            for (auto i = 0; i < data_count; ++i)
            {
                auto abs_value = std::abs(in_blob.As<data_type>(i));
                if (abs_value > clip_value)
                {
                    result.As<long>(result_index++) = i;
                    result.As<data_type>(result_index++) = in_blob.As<data_type>(i);
                }
            }
            CHECK(result_index == non_zero_count * 2);
            out_blob = result;
        }
        
        return true;
    }

    Blob SparseFilter::de_compress(const multiverso::Blob& in_blob, size_t size)
    {
        CHECK(sizeof(data_type) == sizeof(long));
        CHECK(size % sizeof(data_type) == 0);
        auto original_data_count = size / sizeof(data_type);
        Blob result(size);
        for (auto i = 0; i < original_data_count; ++i) result.As<data_type>(i) = 0;
        auto data_count = in_blob.size<data_type>();
        for (auto i = 0; i < data_count; i += 2){
            auto index = in_blob.As<long>(i);
            auto value = in_blob.As<data_type>(i + 1);
            result.As<data_type>(index) = value;
        }

        return result;
    }
}