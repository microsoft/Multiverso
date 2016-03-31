#ifndef MULTIVERSO_QUANTIZATION_UTIL_H
#define MULTIVERSO_QUANTIZATION_UTIL_H

#include <multiverso/blob.h>
#include <vector>

namespace multiverso {
	class QuantizationFilter
	{
	public:
		QuantizationFilter(){}

		virtual ~QuantizationFilter(){}

		virtual std::vector< multiverso::Blob> FilterIn(const std::vector< multiverso::Blob> &blobs) = 0;

		virtual std::vector< multiverso::Blob> FilterOut(const std::vector< multiverso::Blob> &blobs) = 0;
	private:

	};

    template<typename data_type, typename index_type>
	class SparseFilter : public QuantizationFilter
	{
	public:
		SparseFilter(double clip):clip_value(clip){};

		~SparseFilter(){};

        // Returns compressed blobs given input blobs. Each input blob in vector will generate two blobs in result: 
        //  the first blob contains info: compressed or not, original blob size in byte;
        //  the second blob contains info: compressed blob if it's compressed or original blob.
        std::vector< multiverso::Blob> FilterIn(const std::vector< multiverso::Blob> &blobs) override {
            std::vector< multiverso::Blob> result;

            for (auto iter = blobs.cbegin(); iter != blobs.cend(); iter++){
                Blob compressed_blob;
                auto compressed = try_compress(*iter, compressed_blob);
                Blob flag_blob(sizeof(data_type) * 2);
                flag_blob.As<index_type>(0) = compressed ? 1 : 0;            // compressed or not
                flag_blob.As<index_type>(1) = iter->size();                  // blob size
                result.push_back(flag_blob);
                result.push_back(compressed ? std::move(compressed_blob) : *iter);
            }

            return result;
        }

        // Returns de-compressed blobs from input blobs compressed by function FilterIn.
        std::vector< multiverso::Blob> FilterOut(const std::vector< multiverso::Blob> &blobs) override {
            std::vector< multiverso::Blob> result;
            CHECK(blobs.size() % 2 == 0);

            for (auto i = 0; i < blobs.size(); i += 2)
            {
                auto is_compressed = blobs[i].As<index_type>(0) == 1;
                auto size = blobs[i].As<index_type>(1);
                auto& blob = blobs[i + 1];
                result.push_back(is_compressed ? de_compress(blob, size) : blob);
            }

            return result;
        }

    protected:
        bool try_compress(const multiverso::Blob& in_blob, multiverso::Blob& out_blob) {
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
                result.As<index_type>(0) = 0;
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
                        result.As<index_type>(result_index++) = i;
                        result.As<data_type>(result_index++) = in_blob.As<data_type>(i);
                    }
                }
                CHECK(result_index == non_zero_count * 2);
                out_blob = result;
            }

            return true;
        }

        Blob de_compress(const multiverso::Blob& in_blob, size_t size) {
            CHECK(size % sizeof(data_type) == 0);
            auto original_data_count = size / sizeof(data_type);
            Blob result(size);
            for (auto i = 0; i < original_data_count; ++i) result.As<data_type>(i) = 0;
            auto data_count = in_blob.size<data_type>();
            for (auto i = 0; i < data_count; i += 2){
                auto index = in_blob.As<index_type>(i);
                auto value = in_blob.As<data_type>(i + 1);
                result.As<data_type>(index) = value;
            }

            return result;
        }

	private:
		double clip_value;
	};

	class OneBitsFilter : public QuantizationFilter
	{

	};
}


#endif //MULTIVERSO_QUANTIZATION_UTIL_H