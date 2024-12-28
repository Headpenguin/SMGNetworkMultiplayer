#include "packets.hpp"

#include "packets/ack.hpp"
#include "packets/connect.hpp"
#include "packets/playerPosition.hpp"
#include "packets/serverInitialResponse.hpp"

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

    struct PlayerPosition {
        u8 playerId;
        u8 padding[3];
        
        f32 positionX;
        f32 positionY;
        f32 positionZ;

        f32 velocityX;
        f32 velocityY;
        f32 velocityZ;

        f32 directionXx; // Base Matrix, column 1 row 1 (+ 3.0f if row 3 is negative)
        f32 directionXy; // Base Matrix, column 1 row 2
        f32 directionYxz; // ^Y*(^i|^j)x^X (+ 3.0f if Y*rejection is negative)
        
        s32 currAnmIdx;
        s32 defaultAnmIdx;
        f32 anmSpeed;
    };

    struct ServerInitialResponse {
        u32 major;
        u32 minor;
        u8 id;
    };

}

const u32 _Connect::implementationSize = sizeof(implementation::Connect);
const u32 _Ack::implementationSize = sizeof(implementation::Ack);
const u32 _PlayerPosition::implementationSize = sizeof(implementation::PlayerPosition);
Multiplayer::Id _PlayerPosition::consoleId;
const u32 _ServerInitialResponse::implementationSize = sizeof(implementation::ServerInitialResponse);

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

NetReturn _ServerInitialResponse::netWriteToBuffer(void *buff, u32 len) const {
    if(len < implementationSize) return NetReturn::NotEnoughSpace(implementationSize);
    implementation::ServerInitialResponse *packet = (implementation::ServerInitialResponse *)buff;
    packet->major = major;
    packet->minor = minor;
    packet->id = id;
    return NetReturn::Ok(implementationSize);
}

NetReturn _ServerInitialResponse::netReadFromBuffer(ServerInitialResponse *out, const void *buff, u32 len) {
    if(len < implementationSize) return NetReturn::NotEnoughSpace(implementationSize);
    const implementation::ServerInitialResponse *packet = (const implementation::ServerInitialResponse *)buff;
    out->major = packet->major;
    out->minor = packet->minor;
    out->id = packet->id;
    return NetReturn::Ok(implementationSize);
}

NetReturn _PlayerPosition::netWriteToBuffer(void *buff, u32 len) const {
    if(len < implementationSize) return NetReturn::NotEnoughSpace(implementationSize);
    implementation::PlayerPosition *packet = (implementation::PlayerPosition *)buff;
    
    packet->playerId = playerId.toGlobalId();
    
    packet->padding[0] = 0;
    packet->padding[1] = 0;
    packet->padding[2] = 0;

    packet->positionX = position.x;
    packet->positionY = position.y;
    packet->positionZ = position.z;

    packet->velocityX = velocity.x;
    packet->velocityY = velocity.y;
    packet->velocityZ = velocity.z;

    packet->directionXx = direction.x;
    packet->directionXy = direction.y;
    packet->directionYxz = direction.z;

    packet->currAnmIdx = currAnmIdx;
    packet->defaultAnmIdx = defaultAnmIdx;
    packet->anmSpeed = anmSpeed;

    return NetReturn::Ok(implementationSize);
}

NetReturn _PlayerPosition::netReadFromBuffer(PlayerPosition *out, const void *buff, u32 len) {
    if(len < implementationSize) return NetReturn::NotEnoughSpace(implementationSize);
    const implementation::PlayerPosition *packet = (const implementation::PlayerPosition *)buff;
    
    out->playerId = consoleId.fromGlobalId(packet->playerId);

    out->position.set(packet->positionX, packet->positionY, packet->positionZ);
    out->velocity.set(packet->velocityX, packet->velocityY, packet->velocityZ);
    out->direction.set(packet->directionXx, packet->directionXy, packet->directionYxz);

    out->currAnmIdx = packet->currAnmIdx;
    out->defaultAnmIdx = packet->defaultAnmIdx;
    out->anmSpeed = packet->anmSpeed;

    return NetReturn::Ok(implementationSize);
}

}

