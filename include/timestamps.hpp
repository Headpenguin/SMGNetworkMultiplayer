#ifndef TIMESTAMPS_HPP
#define TIMESTAMPS_HPP

#include <revolution/types.h>

namespace Timestamps {

struct _Timestamp {
    s32 timeMs;
};

template<typename T>
struct ClockboundTimestamp {
    _Timestamp t;
    inline bool operator<(const ClockboundTimestamp<T> &other) const {
        return t.timeMs < other.t.timeMs;
    }
    inline bool operator>(const ClockboundTimestamp<T> &other) const {
        return t.timeMs > other.t.timeMs;
    }
    inline bool operator<=(const ClockboundTimestamp<T> &other) const {
        return t.timeMs <= other.t.timeMs;
    }
    inline bool operator>=(const ClockboundTimestamp<T> &other) const {
        return t.timeMs >= other.t.timeMs;
    }
    inline bool operator==(const ClockboundTimestamp<T> &other) const {
        return t.timeMs == other.t.timeMs;
    }
};

struct LocalClockTag {};
struct ServerClockTag {};

typedef ClockboundTimestamp<LocalClockTag> LocalTimestamp;
typedef ClockboundTimestamp<ServerClockTag> ServerTimestamp;

inline ServerTimestamp makeEmptyServerTimestamp() {
   ServerTimestamp ret = {-0x80000000};
   return ret;
}

inline bool isEmpty(const ServerTimestamp &ts) {return ts == makeEmptyServerTimestamp();}

inline s32 differenceMs(const LocalTimestamp &start, const LocalTimestamp &end) {
    return end.t.timeMs - start.t.timeMs;
}

}

#endif
