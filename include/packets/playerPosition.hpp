#ifndef PACKETS_PLAYERPOSITION_HPP
#define PACKETS_PLAYERPOSITION_HPP

#include "packets.hpp"
#include "playerCommon.hpp"
#include "timestamps.hpp"
#include <JSystem/JGeometry/TVec.hpp>

namespace Packets {

class _PlayerPosition {
    const static u32 implementationSize;
public:

    static Multiplayer::Id consoleId;

    Multiplayer::Id playerId;

    Timestamps::ServerTimestamp timestamp;
    Timestamps::LocalTimestamp arrivalTime;

    TVec3f position;
    TVec3f velocity;
    TVec3f direction;
    s32 currAnmIdx;
    s32 defaultAnmIdx;
    f32 anmSpeed;
    
    enum {
        O_STATE_HIPDROP = 1
    }; // State flags

    u8 stateFlags;
    
    inline bool isHipDropStun() const {return stateFlags & O_STATE_HIPDROP;}

    inline _PlayerPosition() 
        : timestamp(Timestamps::makeEmptyServerTimestamp()),
        stateFlags(0)
         {}   
    NetReturn netWriteToBuffer(void *buff, u32 len) const;
    static NetReturn netReadFromBuffer(Packet<_PlayerPosition> *out, const void *buff, u32 len);
    static inline Tag getTag() {return PLAYER_POSITION;}
    inline u32 getSize() const {return implementationSize;}
};

typedef Packet<_PlayerPosition> PlayerPosition;

}

#endif
