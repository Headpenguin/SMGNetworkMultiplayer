#include "net.h"
#include "transmission.hpp"
#include "packetProcessor.hpp"
#include <JSystem/JKernel/JKRHeap.hpp>
#include <kamek/hooks.h>

static bool initialized;
static bool connected;

static const u32 queryCooldown = 60;
static u32 queryTimer = 0;

static Transmission::Transmitter<Packets::PacketProcessor> transmitter;

const static sockaddr_in serverAddr = {8, 2, 5000, 0x0A000060};

static void init() {
    s32 err = netinit();
    if(err < 0) return;
    s32 sd = netsocket(2, 2, 0); // AF_INET, SOCK_DGRAM
    if(sd < 0) return;

    u8 *buff = new (32) u8[5 * Packets::MAX_PACKET_SIZE];
    if(buff == nullptr) return;

    Transmission::Reader reader(16, buff, Packets::MAX_PACKET_SIZE, sd);
    Transmission::Writer writer (buff + Packets::MAX_PACKET_SIZE, 4 * Packets::MAX_PACKET_SIZE, sd, &serverAddr);
    transmitter = Transmission::Transmitter<Packets::PacketProcessor>(reader, writer, Packets::PacketProcessor(&connected));
    initialized = true;
}


extern void start__15DrawSyncManagerFUll(unsigned long, long);

static void initWrapper(unsigned long a, long b) {
    start__15DrawSyncManagerFUll(a, b);
    init();
}

extern kmSymbol init__10GameSystemFv;

kmCall(&init__10GameSystemFv + 0x94, init); // Replaces a call to `DrawSyncManager::start`

// Call this every frame
static void updatePackets() {
    if(queryTimer > 0) queryTimer--;
    else {
        queryTimer = queryCooldown;
        if(!connected) {
            Packets::Connect connect;
            connect.minor = 0;
            connect.major = 0;
            transmitter.addPacket(connect);
        }
    }
    transmitter.update();
}

extern kmSymbol control__10MarioActorFv;

kmBranch(&control__10MarioActorFv + 0x124, updatePackets);

