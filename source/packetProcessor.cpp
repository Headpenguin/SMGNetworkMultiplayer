#include "packetProcessor.hpp"

namespace Packets {

NetReturn PacketProcessor::process(Tag tag, const u8 *buffer, u32 len) {
    switch(tag) {
        case CONNECT:
        {
            break;
        }
        case ACK:
        {
            Ack ack;
            NetReturn res = Ack::netReadFromBuffer(&ack, buffer, len);
            if(res.err != NetReturn::OK) return res;
            if(ack.seqNum == 0) *connected = true;
            break;
        }
    }
    return NetReturn::Ok();
}

}
