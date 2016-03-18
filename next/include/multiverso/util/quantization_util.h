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

		virtual bool filter(std::vector< multiverso::Blob> &blobs) = 0;
	private:

	};

	class SparseFilter : public QuantizationFilter
	{
	public:
		SparseFilter(){};
		~SparseFilter(){};
		bool filter(std::vector< multiverso::Blob> &blobs);

	private:

	};

}


#endif //MULTIVERSO_QUANTIZATION_UTIL_H