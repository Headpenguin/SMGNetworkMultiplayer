#ifndef BEACON_HPP
#define BEACON_HPP

#include <revolution/types.h>

#include "transmission.hpp"
#include "packets/beacon.hpp"
#include "packetProcessor.hpp"

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


class Beacon {
    s32 offsetClientToServerMs;
    u32 commDelayMs;
    u16 timeSinceUpdateFrames;
    bool init;

    void _update(Transmission::Transmitter<Packets::PacketProcessor> &);
    
    const static u16 MAX_UPDATE_INTERVAL_FRAMES = 20 * 60 * 60; // Remember to match size with timeSinceUpdate
public:

    inline Beacon() : init(false), offsetClientToServerMs(0), commDelayMs(0), timeSinceUpdateFrames(0) {}
    inline bool isInit() const {return init;}
    
    // Estimate the conversion, assuming mostly symmetrical communication delays
    inline LocalTimestamp convertToLocal(ServerTimestamp timestamp) const {
        LocalTimestamp t = {timestamp.t.timeMs - offsetClientToServerMs + commDelayMs / 2};
        return t;
    }
    inline ServerTimestamp convertToServer(LocalTimestamp timestamp) const {
        ServerTimestamp t = {timestamp.t.timeMs + offsetClientToServerMs - commDelayMs / 2};
        return t;
    }

    static inline LocalTimestamp now() {
        LocalTimestamp t = {OSTicksToMilliseconds(OSGetTime())};
        return t;
    }
    inline s32 ms(const LocalTimestamp &start, const LocalTimestamp &end) {
        return end.t.timeMs - start.t.timeMs;
    }

    inline void update(Transmission::Transmitter<Packets::PacketProcessor> &transmitter) {
        if(!init || timeSinceUpdateFrames++ >= MAX_UPDATE_INTERVAL_FRAMES) _update(transmitter);
    }

    void process(const Packets::TimeResponse &);

};

extern Beacon beacon; // Accessible read-only from all threads post-init,
                      // rw from callback thread pre-init

}


#endif
