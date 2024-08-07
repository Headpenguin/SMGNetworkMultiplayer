#ifndef PACKETPROCESSOR_HPP
#define PACKETPROCESSOR_HPP

#include "packets.hpp"
#include "packets/connect.hpp"
#include "packets/ack.hpp"

namespace Packets {

class PacketProcessor {
    bool *connected;
public:
    inline PacketProcessor() {}
    inline PacketProcessor(bool *connected) : connected(connected) {}
    NetReturn process(Tag tag, const u8 *buff, u32 len);
};

}

#endif
