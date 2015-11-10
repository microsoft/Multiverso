#ifndef MULTIVERSO_STOP_WATCH_
#define MULTIVERSO_STOP_WATCH_

#include <cstdint>

/*!
 * \file stop_watch.h
 * \brief Defines a StopWatch class
 * \author feiyan
 */

namespace multiverso
{
    /*! 
     * \brief The StopWatch class is a timer like StopWatch class in C#. In 
     *        Windows' version, it utilizes some high resolution facilities,
     *        or standard C clock() function instead.
     */
    class StopWatch
    {
    public:
        /*! \brief Creates a StopWatch and starts timing. */
        StopWatch();
        ~StopWatch();

        /*! \brief Clears the status and restart timing. */
        void Restart();
        /*! \brief Starts timing. Does nothing if it is already running. */
        void Start();
        /*! 
         * \brief Stops timing and accumulates the elapsed time since last 
         *        valid starting. Does nothing if it is already stopped. 
         */
        void Stop();
        /*! \brief Returns if the StopWatch instance is running (timing). */
        bool IsRunning();
        /*! 
         * \brief Returns the elapsed time accumulated between the valid starts
         *        and stops, it includes the time from the last valid starting
         *        to now if it is in running status.
         */
        double ElapsedSeconds();

    private:
        int64_t GetTickPerSec();
        int64_t GetCurrentTick();

        int64_t tick_per_sec_;
        int64_t elapsed_tick_;
        int64_t start_tick_;
    };
}

#endif // MULTIVERSO_STOP_WATCH_
