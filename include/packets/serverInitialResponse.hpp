#ifndef PACKETS_SERVERINITIALRESPONSE_HPP
#define PACKETS_SERVERINITIALRESPONSE_HPP

#include "packets.hpp"

namespace Packets {

class _ServerInitialResponse {
    const static u32 implementationSize;
public:
    u32 major;
    u32 minor;
    u8 id;

    NetReturn netWriteToBuffer(void *buff, u32 len) const;
    static NetReturn netReadFromBuffer(Packet<_ServerInitialResponse> *out, const void *buff, u32 len);

    static inline Tag getTag() {return SERVER_INITIAL_RESPONSE;}
    inline u32 getSize() const {return implementationSize;}
};

typedef Packet<_ServerInitialResponse> ServerInitialResponse;

}

#endif
