#ifndef ACCURATETIME_HPP
#define ACCURATETIME_HPP

#include "timestamps.hpp"

namespace Timestamps {

extern float realtimeRate; // how many real seconds pass every second OS second

void updateDolphinTime();

LocalTimestamp now();

}

#endif
