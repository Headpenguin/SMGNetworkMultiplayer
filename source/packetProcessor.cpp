#include "packetProcessor.hpp"
#include "packets/connect.hpp"
#include "packets/ack.hpp"
#include "packets/serverInitialResponse.hpp"
#include "packets/playerPosition.hpp"

namespace Packets {

NetReturn PacketProcessor::process(Tag tag, const u8 *buffer, u32 len) {
    switch(tag) {
        case CONNECT:
        {
            break;
        }
        case ACK:
        {
/*            Ack ack;
            NetReturn res = Ack::netReadFromBuffer(&ack, buffer, len);
            if(res.err != NetReturn::OK) return res;
            if(ack.seqNum == 0) *connected = true;*/
            break;
        }
        case SERVER_INITIAL_RESPONSE:
        {
            if(!*connected) {
                ServerInitialResponse sip;
                NetReturn res = ServerInitialResponse::netReadFromBuffer(&sip, buffer, len);
                if(res.err != NetReturn::OK) return res;
                if(sip.major != Multiplayer::MAJOR) return NetReturn::InvalidData();
                PlayerPosition::consoleId = Multiplayer::Id::selfId(sip.id);
                *connected = true;
            }

            break;
        }
        case PLAYER_POSITION:
        {
            PlayerPosition pos;
            NetReturn res = PlayerPosition::netReadFromBuffer(&pos, buffer, len);
            if(res.err != NetReturn::OK) return res;
            
            u8 id = pos.playerId.toLocalId();
            if(id >= Multiplayer::MAX_PLAYER_COUNT - 1) return NetReturn::InvalidData();
            Multiplayer::PlayerDoubleBuffer &doubleBuff = players->players[id];
            
            u32 buffIdx = Multiplayer::getMostRecentBuffer(id, players->status);
            buffIdx = buffIdx == 1 ? 0 : 1;
            
            if(!simplelock_tryLock(&doubleBuff.locks[buffIdx])) {
                buffIdx = buffIdx == 1 ? 0 : 1;
                if(!simplelock_tryLock(&doubleBuff.locks[buffIdx])) return NetReturn::Busy(); // This really should not happen
            }

            doubleBuff.pos[buffIdx] = pos;

            simplelock_release(&doubleBuff.locks[buffIdx]);

            players->status = Multiplayer::setMostRecentBuffer(id, buffIdx, players->status);
            players->activityStatus = Multiplayer::setMostRecentBuffer(id, 1, players->activityStatus);

            break;
        }
    }
    return NetReturn::Ok();
}

}
