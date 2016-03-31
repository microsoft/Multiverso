#include "gtest\gtest.h"
#include <multiverso/multiverso.h>
#include <vector>
#include <multiverso\util\quantization_util.h>
#include <cctype>

class test_filter : public multiverso::SparseFilter<float, int32_t>, public ::testing::Test    {

public:
    test_filter() : SparseFilter( 0.001){    }
    const float clip_value      = 0.001;
    const float near_zero_value = 0.0001;
    const float none_zero_value = 0.0011;

protected:
    bool is_same_content(float* array, multiverso::Blob blob, size_t count){
        for (auto i = 0; i < count; ++i){
            if (array[i] != blob.As<float>(i)) {
                return false;
            }
        }

        return true;
    }

    bool is_same_content_with_clipping(float* array, multiverso::Blob blob, size_t count){
        for (auto i = 0; i < count; ++i){
            if (std::abs(array[i] - blob.As<float>(i)) > clip_value){
                return false;
            }
        }

        return true;
    }
};

TEST_F(test_filter, test_should_compress_all_zero){
    auto size = 8;
    auto array = new float[size]{near_zero_value, near_zero_value, near_zero_value, near_zero_value,
        near_zero_value, near_zero_value, near_zero_value, near_zero_value};
    multiverso::Blob input_blob(array, sizeof(float) * size);
    multiverso::Blob compressed_blob;
    auto compressed = try_compress(input_blob, compressed_blob);
    ASSERT_EQ(compressed, true);
    auto de_compressed_blob = de_compress(compressed_blob, size * sizeof(float));
    ASSERT_TRUE(is_same_content_with_clipping(array, de_compressed_blob, size));
}

TEST_F(test_filter, test_should_compress_most_zero_a){
    auto size = 8;
    auto array = new float[size]{1, 0, 0, 0, 0, 0, near_zero_value, none_zero_value};
    multiverso::Blob input_blob(array, sizeof(float) * size);
    multiverso::Blob compressed_blob;
    auto compressed = try_compress(input_blob, compressed_blob);
    ASSERT_TRUE(compressed == true);
    auto de_compressed_blob = de_compress(compressed_blob, size * sizeof(float));
    ASSERT_TRUE(is_same_content_with_clipping(array, de_compressed_blob, size));
}

TEST_F(test_filter, test_should_compress_most_zero_b){
    auto size = 8;
    auto array = new float[size]{1, 2, 0, 0, 0, 0, 0, 3};
    multiverso::Blob input_blob(array, sizeof(float) * size);
    multiverso::Blob compressed_blob;
    auto compressed = try_compress(input_blob, compressed_blob);
    ASSERT_TRUE(compressed == true);
    auto de_compressed_blob = de_compress(compressed_blob, size * sizeof(float));
    ASSERT_TRUE(is_same_content(array, de_compressed_blob, size));
}

TEST_F(test_filter, test_should_not_compress_half_zero){
    auto size = 8;
    auto array = new float[size]{1, 0, 2, 0, 3, 0, 0, 4};
    multiverso::Blob input_blob(array, sizeof(float) * size);
    multiverso::Blob compressed_blob;
    auto compressed = try_compress(input_blob, compressed_blob);
    ASSERT_TRUE(compressed == false);
}

TEST_F(test_filter, test_should_not_compress_most_none_zero){
    auto size = 8;
    auto array = new float[size]{1, 2, 3, 4, 0, 0, 0, 5};
    multiverso::Blob input_blob(array, sizeof(float) * size);
    multiverso::Blob compressed_blob;
    auto compressed = try_compress(input_blob, compressed_blob);
    ASSERT_TRUE(compressed == false);
}

TEST_F(test_filter, test_should_not_compress_all_none_zero){
    auto size = 8;
    auto array = new float[size]{none_zero_value, none_zero_value, none_zero_value, none_zero_value,
        none_zero_value, none_zero_value, none_zero_value, none_zero_value};
    multiverso::Blob input_blob(array, sizeof(float) * size);
    multiverso::Blob compressed_blob;
    auto compressed = try_compress(input_blob, compressed_blob);
    ASSERT_TRUE(compressed == false);
}

TEST_F(test_filter, correctly_compress_and_decompress){
    auto size = 8;
    auto array0 = new float[size]{1, 1, 1, 0, 0, 0, 0, 0};
    auto array1 = new float[size]{1, 2, 3, 4, 0, 0, 0, 0};
    auto array2 = new float[size]{1, 2, 3, 4, 5, 0, 0, 0};
    auto compressed_blobs = FilterIn( std::vector< multiverso::Blob> {
        multiverso::Blob(array0, sizeof(float) * size),
        multiverso::Blob(array1, sizeof(float) * size),
        multiverso::Blob(array2, sizeof(float) * size)
    });
    ASSERT_EQ(compressed_blobs.size(), 6);
    auto de_compressed_blobs = FilterOut(compressed_blobs);
    ASSERT_EQ(de_compressed_blobs.size(), 3);
    ASSERT_TRUE(is_same_content(array0, de_compressed_blobs[0], size));
    ASSERT_TRUE(is_same_content(array1, de_compressed_blobs[1], size));
    ASSERT_TRUE(is_same_content(array2, de_compressed_blobs[2], size));
}

