#ifndef GLOBALTRANSMITTER_HPP
#define GLOBALTRANSMITTER_HPP

#include "packetProcessor.hpp"
#include "transmission.hpp"

namespace Multiplayer {
    extern Transmission::Transmitter<Packets::PacketProcessor> transmitter;
}

#endif
