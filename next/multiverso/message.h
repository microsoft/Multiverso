#ifndef MULTIVERSO_MESSAGE_H_
#define MULTIVERSO_MESSAGE_H_

#include <function>

#include "base.h"

// NOTE(feiga): using zmq c api instead of c++
#include <third_party/zmq/zmq.h> 

namespace multiverso {

class Arena;

// Callback function type when message is deleted.
typedef std::function<void(Arena&)> MsgDeleteCallBack;

// Currently Message is a wrapper of ZeroMQ message
// TODO(feiga), considering a more general message interface. Zmq can be 
// a kind of implementation.
class Message {
 public:
	Message();
	explicit Message(size_t size);

	// Allocate a message from arena 
	Message(Arena& arena, size_t size);

	// Construct a Message from a pre-allocated memory
	Message(char* mem, size_t size);

  // Delete message
	~Message();
  
	void* RawPtr();

	template<typename T>
	T* RawPtr() { return reinterpret_cast<T*>(RawPtr()); }

 private:
	enum MemOwner {
		kArena,
		kInternal,
		kOutside
	};

	char* mem_;
	size_t size_;
	MemOwner owner_;
	
	// TODO: Whether copy allowed?

}

} // namespace multiverso

#endif // MULTIVERSO_MESSAGE_H_
