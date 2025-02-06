#ifndef PACKETS_STARPIECE_HPP
#define PACKETS_STARPIECE_HPP

#include "packets.hpp"
#include "timestamps.hpp"
#include "playerCommon.hpp"
#include <JSystem/JGeometry/TVec.hpp>

namespace Packets {

class _StarPiece {
    const static u32 implementationSize;
public:
    Multiplayer::Id playerId;
    
    Timestamps::ServerTimestamp timestamp;
    
    TVec3f initLineStart;
    TVec3f initLineEnd;
    
    Timestamps::LocalTimestamp arrivalTime;

    inline _StarPiece() : timestamp(Timestamps::makeEmptyServerTimestamp()) {}
    NetReturn netWriteToBuffer(void *buff, u32 len) const;
    static NetReturn netReadFromBuffer(Packet<_StarPiece> *out, const void *buff, u32 len);
    static inline Tag getTag() {return STAR_PIECE;}
    inline u32 getSize() const {return implementationSize;}
};

typedef Packet<_StarPiece> StarPiece;

}

#endif
