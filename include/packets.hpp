#ifndef PACKETS_HPP
#define PACKETS_HPP

#include <revolution/types.h>
#include "netCommon.hpp"

namespace Packets {

static const u32 MAX_PACKET_SIZE = 128;

enum Tag {
    CONNECT = 0,
    ACK
};

template<typename T>
class Packet : public T {
public:


    // Write the contents of this packet into `buff` so we can send
    // it over the network (perform any necessary endian conversions too).
    // Return the number of bytes written and possibly an error
    inline NetReturn netWriteToBuffer(void *buff, u32 len) const {
        return T::netWriteToBuffer(buff, len);
    }

    // Read the contents of `buff` into `out`. Return the number of bytes read
    // and possibly an error
    inline static NetReturn netReadFromBuffer(Packet<T> *out, const void *buff, u32 len) {
        return T::netReadFromBuffer(out, buff, len);
    }

    inline static Tag getTag() {return T::getTag();}
    inline u32 getSize() const {return T::getSize();}
};

}

#endif
