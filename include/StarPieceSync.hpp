#ifndef STARPIECESYNC_HPP
#define STARPIECESYNC_HPP

#include "timestamps.hpp"
#include "packets/starPiece.hpp"
#include "concurrencyUtils.hpp"

#include <Game/MapObj/StarPiece.hpp>

class NetStarPiece : public StarPiece {
public:

    NetStarPiece(const char *name);
    virtual void kill();

    bool isPlayerGenerated;
    bool isNetActorGenerated;
    Timestamps::LocalTimestamp launchTime;
    Timestamps::LocalTimestamp prevTime;
    f32 scalar;
};

extern ConcurrentQueue<Packets::StarPiece> netStarPieceQueue;

#endif
