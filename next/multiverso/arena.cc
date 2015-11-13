#include "arena.h"

namespace multiverso {

static const size_t kDefaultInitialBlockSize = 1 << 20; // 1MB
static const size_t kDefaultBlockUnitSize    = 1 << 12; // 4KB

Arena::Arena() {
	initial_mem_ = AllocateNew(kDefaultInitialBlockSize);
	next_ptr_ = initial_mem_.memory;
	available_size_ = initial_mem_.size;
	total_size_ = initial_mem_.size;

	request_size_ = 0; 
	request_time_ = 0;
}

Arena::~Arena() {
	free(initial_mem_.memory);
	FreeBlocks();
}

void Reset() {
	FreeBlocks();
	Resize();
	next_ptr_ = initial_mem_.memory;
	available_size_ = initial_mem_.size;
}



}
