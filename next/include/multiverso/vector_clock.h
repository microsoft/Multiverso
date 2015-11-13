#ifndef MULTIVERSO_VECTOR_CLOCK_
#define MULTIVERSO_VECTOR_CLOCK_

/*!
 * \file vector_clock.h
 * \brief Defines VectorClock for multi thread/process 
 */

#include <vector>

namespace multiverso
{
    /*!
     * \brief VectorClock manages states of multi threads/processes clock
     */
    class VectorClock
    {
    public:
        explicit VectorClock(int n);
        /*!
         * \brief Updates i-th clock
         * \return Return true if i-th clock is slowest, false otherwise
         */
        bool Update(int i);
    private:
        std::vector<int> vectors_;
        int clock_; // min clock of all clocks in vector
    };
}

#endif // MULTIVERSO_VECTOR_CLOCK_
