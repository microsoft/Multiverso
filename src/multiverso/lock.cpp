#include "lock.h"

namespace multiverso
{    
#if defined(_WIN32) || defined(_WIN64)
    // Defines the Windows' version Mutex which wraps a CRITICAL_SECTION object.
    class Mutex
    {
    public:
        Mutex() { InitializeCriticalSection(&mutex_); }
        ~Mutex() { DeleteCriticalSection(&mutex_); }
        void Lock() { EnterCriticalSection(&mutex_); }
        void Unlock() { LeaveCriticalSection(&mutex_); }
    private:
        CRITICAL_SECTION mutex_;
    };
#else
    // In other systems, class Mutex is a wrapper if C++ standard mutex object.
    class Mutex
    {
    public:
        Mutex() {}
        ~Mutex() {}
        void Lock() { mutex_.lock(); }
        void Unlock() { mutex_.unlock(); }
    private:
        std::mutex mutex_;
    };
#endif

    LockManager::LockManager(int num_lock) : locks_(num_lock) { }
    LockManager::~LockManager() {}

	void LockManager::Lock(int id) { locks_[id % locks_.size()].Lock(); }
    void LockManager::Unlock(int id) { locks_[id % locks_.size()].Unlock(); }
    int LockManager::Size() { return static_cast<int>(locks_.size()); }
}
