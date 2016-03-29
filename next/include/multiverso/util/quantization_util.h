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

		virtual std::vector< multiverso::Blob> FilterOut(const std::vector< multiverso::Blob> &blobs);
	private:

	};

	class SparseFilter : public QuantizationFilter
	{
	public:
		SparseFilter(double clip):clip_value(clip){};

		~SparseFilter(){};

		std::vector< multiverso::Blob> FilterIn(const std::vector< multiverso::Blob> &blobs);

	private:
		double clip_value;

	};

	class OneBitsFilter : public QuantizationFilter
	{

	};
}


#endif //MULTIVERSO_QUANTIZATION_UTIL_H