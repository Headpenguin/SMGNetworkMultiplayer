#ifndef PACKETS_ACK_HPP
#define PACKETS_ACK_HPP

#include "packets.hpp"

namespace Packets {

class _Ack {
    const static u32 implementationSize;
public:
    u32 seqNum;

    NetReturn netWriteToBuffer(void *buff, u32 len) const;
    static NetReturn netReadFromBuffer(Packet<_Ack> *out, const void *buff, u32 len);

    static inline Tag getTag() {return ACK;}
    inline u32 getSize() const {return implementationSize;}
};

typedef Packet<_Ack> Ack;

}

#endif
