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

	class SparseFilter : public QuantizationFilter
	{
	public:
		SparseFilter(double clip):clip_value(clip){};

		~SparseFilter(){};

		std::vector< multiverso::Blob> FilterIn(const std::vector< multiverso::Blob> &blobs) override;
        std::vector< multiverso::Blob> FilterOut(const std::vector< multiverso::Blob> &blobs) override;

    protected:
        bool try_compress(const multiverso::Blob& in_blob, multiverso::Blob& out_blob);
        Blob de_compress(const multiverso::Blob& in_blob, size_t size);

	private:
		double clip_value;

	};

	class OneBitsFilter : public QuantizationFilter
	{

	};
}


#endif //MULTIVERSO_QUANTIZATION_UTIL_H