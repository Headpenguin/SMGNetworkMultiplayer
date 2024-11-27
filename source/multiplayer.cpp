#include "net.h"
#include "transmission.hpp"
#include "packetProcessor.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>
#include <kamek/hooks.h>
#include <Game/System/DrawSyncManager.hpp>
#include <Game/Player/MarioActor.hpp>
#include <Game/Player/MarioAnimator.hpp>
#include "packets/connect.hpp"
#include "packets/playerPosition.hpp"
#include "debug.hpp"

extern kmSymbol init__10GameSystemFv;
extern kmSymbol control__10MarioActorFv;

namespace Multiplayer {

static bool initialized;
static bool connected;

static const u32 queryCooldown = 60;
static u32 queryTimer = 0;

//static int sockfd;

MultiplayerInfo info;

static Transmission::Transmitter<Packets::PacketProcessor> transmitter;

const static sockaddr_in serverAddr = {8, 2, 5000, 0x0A000060};
const static sockaddr_in debugAddr = {8, 2, 5001, 0x0A000060};

static void init() {
    if(!initialized) {
        s32 err = netinit();
        if(err < 0) return;
        s32 sd = netsocket(2, 2, 0); // AF_INET, SOCK_DGRAM
        if(sd < 0) return;

        u8 *buff = new (32) u8[5 * Packets::MAX_PACKET_SIZE];
        if(buff == nullptr) return;

        Transmission::Reader reader(16, buff, Packets::MAX_PACKET_SIZE, sd);
        Transmission::Writer writer (buff + Packets::MAX_PACKET_SIZE, 4 * Packets::MAX_PACKET_SIZE, sd, &serverAddr);
        transmitter = Transmission::Transmitter<Packets::PacketProcessor>(reader, writer, Packets::PacketProcessor(&connected, &info));
        transmitter.init();
        //sockfd = sd;
        initialized = true;
        setupDebug(sd, &debugAddr);
    }
}


static void initWrapper(unsigned long a, long b) {
    DrawSyncManager::start(a, b);
    init();
}


kmCall(&init__10GameSystemFv + 0x94, initWrapper); // Replaces a call to `DrawSyncManager::start`

//u8 test[8] __attribute__((aligned(32)));

// Call this every frame
static void updatePackets(MarioActor *mario) {
    mario->control2();
    if(initialized) {
        setDebugMsg(0, 0xFE);
        //test[0] = 0xfe;
        if(queryTimer > 0) queryTimer--;
        else {
            queryTimer = queryCooldown;
        sendDebugMsg();
            if(!connected) {
                Packets::Connect connect;
                connect.minor = MAJOR;
                connect.major = MINOR;
                setDebugMsg(2, transmitter.addPacket(connect).err);
              /*  if(test[4]) {
                    netsendto(sockfd, test, 8, &serverAddr);
                    test[4] = 0;
                }*/
            }
/*            else {
                test[6] = 1;
                netsendto(sockfd, test, 8, &serverAddr);
            }*/
        }

        if(connected) {
            Packets::PlayerPosition pos;
            pos.playerId = Packets::PlayerPosition::consoleId;
            pos.position = mario->mPosition;
            pos.velocity = mario->mVelocity;
            //pos.direction = mario->mRotation;
            const Mtx &baseMtx = mario->getJ3DModel()->_24;
            TVec3f X(baseMtx[0][0], baseMtx[1][0], baseMtx[2][0]);
            f32 magx = PSVECMag(X.toCVec());

            f32 r = 1 / magx;
            pos.direction.x = X.x * r;
            pos.direction.y = X.y * r;
            if(X.z < 0.0f) pos.direction.x += 3.0f;
            TVec3f v(0.0f, 0.0f, 0.0f), u, y(baseMtx[0][1], baseMtx[1][1], baseMtx[2][1]);
            if(X.x < 0.99f && X.x > -0.99f) {
                v.z = X.y;
                v.y = -X.z;
                v.setLength(1);
                pos.direction.z = (v.z * baseMtx[2][1] + v.y * baseMtx[1][1]) * r;
            }
            else {
                v.x = X.z;
                v.z = -X.x;
                v.setLength(1);
                pos.direction.z = (v.x * baseMtx[0][1] + v.z * baseMtx[2][1]) * r;
            }
            PSVECCrossProduct(v.toCVec(), X.toCVec(), u.toVec());
            if(y.dot(u) < 0.0f) pos.direction.z += 3.0f;

            const XanimePlayer &xanime = *mario->mMarioAnim->mXanimePlayer;
            s32 diff = xanime.mCurrentAnimation - xanime.mResourceTable->_10;
            if(diff < 0x134 && diff >= 0) {
                pos.currAnmIdx = diff;
            }
            else pos.currAnmIdx = -1;
            diff = xanime.mCurrentAnimation - xanime.mResourceTable->_10;
            if(diff < 0x134 && diff >= 0) {
                pos.defaultAnmIdx = diff;
            }
            else pos.defaultAnmIdx = -1;

            setDebugMsg(2, transmitter.addPacket(pos).err);
        }

        transmitter.update();
    }
}


kmCall(&control__10MarioActorFv + 0x100, updatePackets);

}

