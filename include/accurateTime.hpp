#ifndef ACCURATETIME_HPP
#define ACCURATETIME_HPP

#include "timestamps.hpp"

const static f32 STANDARD_FPS = 60.0f;

namespace Timestamps {

extern float realtimeRate; // how many real seconds pass every second OS second

void updateDolphinTime();

LocalTimestamp now();

}

#endif
