#ifndef BEACON_HPP
#define BEACON_HPP

#include <revolution/types.h>

#include "transmission.hpp"
#include "packets/beacon.hpp"
#include "packetProcessor.hpp"
#include "timestamps.hpp"

namespace Timestamps {

namespace implementation {
    class _Beacon;
}

class Beacon {
    friend class implementation::_Beacon;
    s32 offsetClientToServerMs;
    u32 commDelayMs;
    u16 timeSinceUpdateFrames;
    volatile bool init;

    void _update(Transmission::Transmitter<Packets::PacketProcessor> &);
    
public:

    void init1();
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

    inline void update(Transmission::Transmitter<Packets::PacketProcessor> &transmitter) {
        if(!init || --timeSinceUpdateFrames <= 0) _update(transmitter);
    }

    void process(const Packets::TimeResponse &);

};

extern Beacon beacon; // Accessible read-only from all threads post-init,
                      // rw from callback thread pre-init

}


#endif
