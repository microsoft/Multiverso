#include "vector_clock.h"

#include <algorithm>

namespace multiverso
{
    VectorClock::VectorClock(int n) 
        : vectors_(n, 0), clock_(0) {}

    bool VectorClock::Update(int i)
    {
        ++vectors_[i];
        ;
        if (*(std::min_element(std::begin(vectors_), std::end(vectors_)))
            > clock_)
        {
            ++clock_;
            return true;
        }
        return false;
    }
}