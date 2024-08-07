#ifndef PACKETS_CONNECT_HPP
#define PACKETS_CONNECT_HPP

#include "packets.hpp"

namespace Packets {

class _Connect {
    const static u32 implementationSize;
public:
    u32 minor;
    u32 major;

    NetReturn netWriteToBuffer(void *buff, u32 len) const;
    static NetReturn netReadFromBuffer(Packet<_Connect> *out, const void *buff, u32 len);
    static inline Tag getTag() {return CONNECT;}
    inline u32 getSize() const {return implementationSize;}
};

typedef Packet<_Connect> Connect;

}

#endif
