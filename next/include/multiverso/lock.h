#ifndef MULTIVERSO_LOCK_H_
#define MULTIVERSO_LOCK_H_

/*!
* \file lock.h
* \brief Defines class LockManager for contentions in multi-threading programs.
*/

#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#else
#include <mutex>
#endif

#include <vector>

namespace multiverso
{
    /*! 
     * \brief The class Mutex is a system dependent lock wrapper. For Windows
     *        system, it wraps a CRITICAL_SECTION object for high efficiency;
     *        otherwise it wraps a C++ standard mutex object.
     */
    class Mutex;

    /*! 
     * \brief The class LockManager is a Mutex container providing locking 
     *        features.
     */
    class LockManager
    {
    public:
        /*! 
         * \brief Creates a LockManager with specific number of Mutex objects.
         * \param num_lock Number of Mutex objects. Undefined behavior if
         *        num_lock <= 0
         */
        explicit LockManager(int num_lock);
        ~LockManager();

        /*! \brief Locks the Mutex object with order id % lock_num. */
        void Lock(int id);
        /*! \brief Unlocks the Mutex object with order id % lock_num. */
        void Unlock(int id);
        /*! \brief Returns the number of Mutex objects. */
        int Size();

	private:
        std::vector<Mutex> locks_;

        // No copying allowed
        LockManager(const LockManager&);
        void operator=(const LockManager&);
    };
}

#endif // MULTIVERSO_LOCK_H_
