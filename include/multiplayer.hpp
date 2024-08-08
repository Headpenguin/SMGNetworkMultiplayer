#ifndef MULTIPLAYER_HPP
#define MULTIPLAYER_HPP

#include "atomic.h"
#include "packets/playerPosition.hpp"

namespace Multiplayer {

const u32 MAX_PLAYER_COUNT = 8;
const u32 MAJOR = 0;
const u32 MINOR = 0;

typedef u32 PlayerBufferStatus;
typedef u32 PlayerActivityStatus;

inline u32 getMostRecentBuffer(u8 player, u32 bufferStatus) {
    return (bufferStatus >> (player - 1)) & 1;
}

inline u32 setMostRecentBuffer(u8 player, u32 buffer, u32 bufferStatus) {
    return bufferStatus | (buffer << player);
}

struct PlayerDoubleBuffer {
    simplelock_t locks[2];
    Packets::PlayerPosition pos[2];
};

struct MultiplayerInfo {
    PlayerBufferStatus status;
    PlayerActivityStatus activityStatus;
    PlayerDoubleBuffer players[MAX_PLAYER_COUNT - 1];
};

extern MultiplayerInfo info;

}

#endif
