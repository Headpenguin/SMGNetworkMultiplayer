#include "packets.hpp"

#include "packets/ack.hpp"
#include "packets/connect.hpp"


namespace Packets {

namespace implementation {

    static const u64 CONNECT_MAGIC = 0x436F6E6E65637400; // "Connect"
    
    struct Connect {
        u8 magic[8];
        u32 major;
        u32 minor;
    };
    
    struct Ack {
        u32 seqNum;
    };

}

const u32 _Connect::implementationSize = sizeof(implementation::Connect);
const u32 _Ack::implementationSize = sizeof(implementation::Ack);

NetReturn _Connect::netWriteToBuffer(void *buff, u32 len) const {
    if(len < implementationSize) return NetReturn::NotEnoughSpace(implementationSize);
    implementation::Connect *packet = (implementation::Connect *)buff;
    *(u64*)packet->magic = implementation::CONNECT_MAGIC;
    packet->major = major;
    packet->minor = minor;
    return NetReturn::Ok(implementationSize);
}

NetReturn _Connect::netReadFromBuffer(Connect *out, const void *buff, u32 len) {
    if(len < implementationSize) return NetReturn::NotEnoughSpace(implementationSize);
    const implementation::Connect *packet = (const implementation::Connect *)buff;
    if(*(u64*)packet->magic != implementation::CONNECT_MAGIC) return NetReturn::InvalidData();
    out->major = packet->major;
    out->minor = packet->minor;
    return NetReturn::Ok(implementationSize);
}

NetReturn _Ack::netWriteToBuffer(void *buff, u32 len) const {
    if(len < implementationSize) return NetReturn::NotEnoughSpace(implementationSize);
    implementation::Ack *packet = (implementation::Ack *)buff;
    packet->seqNum = seqNum;
    return NetReturn::Ok(implementationSize);
}

NetReturn _Ack::netReadFromBuffer(Ack *out, const void *buff, u32 len) {
    if(len < implementationSize) return NetReturn::NotEnoughSpace(implementationSize);
    const implementation::Ack *packet = (const implementation::Ack *)buff;
    out->seqNum = packet->seqNum;
    return NetReturn::Ok(implementationSize);
}

}

