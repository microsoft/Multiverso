#ifndef MULTIVERSO_BASE_H_
#define MULTIVERSO_BASE_H_

#include <string>
#include <vector>

namespace multiverso {

#define DISALLOW_COPY_AND_ASSIGN(Type) \
	Type(const Type&) = delete; 		     \
	void operator=(const Type&) = delete
	
} // namespace multiverso

#endif // MULTIVERSO_BASE_H_
