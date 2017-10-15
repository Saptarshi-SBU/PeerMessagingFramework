#ifndef __CHRONO_HPP__
#define __CHRONO_HPP__

#include <boost/chrono.hpp>
#include <boost/timer/timer.hpp>

//Note using nanoseconds here for duration. Assuming platform
//supports so

namespace Timer {

typedef boost::chrono::time_point<boost::chrono::steady_clock> TimePoint;

inline TimePoint Now()
{
   return boost::chrono::steady_clock::now();
}


inline uint64_t NanoSecsSince(const TimePoint &tp)
{
   return boost::chrono::nanoseconds(Now() - tp).count();
   //return boost::chrono::duration_cast<boost::chrono::nanoseconds>(tp - tp).count();
}


inline uint64_t NanoSecsDelta(const TimePoint &start,
                            const TimePoint &end)
{
   return boost::chrono::nanoseconds(end-start).count();
}

}
#endif
