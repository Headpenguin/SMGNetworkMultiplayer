#ifndef MULTIPLAYER_HPP
#define MULTIPLAYER_HPP

#include "atomic.h"
#include "packets/playerPosition.hpp"

class AlignmentState;

namespace Multiplayer {

const u32 MAX_PLAYER_COUNT = 8;
const u32 MAJOR = 0;
const u32 MINOR = 0;
//const u32 BUFFER_FRAMES = 10;

typedef u32 PlayerBufferStatus;
typedef u32 PlayerActivityStatus;

inline u32 getMostRecentBuffer(u8 player, u32 bufferStatus) {
    return bufferStatus >> player & 1;
}

inline u32 setMostRecentBuffer(u8 player, u32 buffer, u32 bufferStatus) {
    return bufferStatus & ~(1 << player) | buffer << player;
}
/*
class PlayerQueue {
    Packets::PlayerPosition pos[BUFFER_FRAMES];
    Packets::PlayerPosition fallback;
    u32 determineBufPosition() const;
public:
    void init() {
        memset(this, 0, sizeof *this);
    }
    const Packets::PlayerPosition& getCurrentFrame() const;
    void addPosition(const Packets::PlayerPosition &);
}
*/
struct PlayerDoubleBuffer {
    simplelock_t locks[2];
//    Packets::PlayerQueue pos[2];
    Packets::PlayerPosition pos[2];
};

struct MultiplayerInfo {
    PlayerBufferStatus status;
    PlayerActivityStatus activityStatus;
    PlayerDoubleBuffer players[MAX_PLAYER_COUNT - 1];
};

extern MultiplayerInfo info;
extern bool connected;

class MultiplayerAccess {
    Packets::PlayerPosition pos[MAX_PLAYER_COUNT - 1];
public:
    inline bool isPlayerActive(u32 i) const {
        return Multiplayer::getMostRecentBuffer(i, Multiplayer::info.activityStatus);
    }
    const Packets::PlayerPosition& getPlayerPosRaw(u32);
    bool isPlayerPosEstimateSet(u32) const;
    void setPlayerPosEstimate(u32) const;
    AlignmentState& getPlayerPosEstimate(u32) const;
};

extern MultiplayerAccess access;

}

#endif
