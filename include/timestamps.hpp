#ifndef TIMESTAMPS_HPP
#define TIMESTAMPS_HPP

namespace Timestamps {

struct _Timestamp {
    u32 timeMs;
};

template<typename T>
struct ClockboundTimestamp {
    _Timestamp t;
};

struct LocalClockTag {};
struct ServerClockTag {};

typedef ClockboundTimestamp<LocalClockTag> LocalTimestamp;
typedef ClockboundTimestamp<ServerClockTag> ServerTimestamp;

inline ServerTimestamp makeEmptyServerTimestamp() {
   ServerTimestamp ret = {0xFFFFFFFF};
   return ret;
}

}

#endif
