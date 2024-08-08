#ifndef PACKETPROCESSOR_HPP
#define PACKETPROCESSOR_HPP

#include "packets.hpp"
#include "multiplayer.hpp"

namespace Packets {

class PacketProcessor {
    bool *connected;
    Multiplayer::MultiplayerInfo *players;
public:
    inline PacketProcessor() {}
    inline PacketProcessor(bool *connected, Multiplayer::MultiplayerInfo *players) 
        : connected(connected),
        players(players) {}
    NetReturn process(Tag tag, const u8 *buff, u32 len);
};

}

#endif
