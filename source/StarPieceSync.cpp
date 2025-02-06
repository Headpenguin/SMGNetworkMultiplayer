#include "StarPieceSync.hpp"
#include "packets/starPiece.hpp"
#include "multiplayer.hpp"
#include "beacon.hpp"
#include "timestamps.hpp"
#include "accurateTime.hpp"
#include "globalTransmitter.hpp"

#include <Game/Util/ActorSensorUtil.hpp>
#include <Game/Player/MarioActor.hpp>
#include <Game/MapObj/StarPieceDirector.hpp>

#include <kamek/hooks.h>

static bool throwToTargetCoreAndBroadcast(NetStarPiece &self, const TVec3f &tip, const TVec3f &tail, const TVec3f &gravity, f32 a, bool isFall) {
    if(Multiplayer::connected) {
        Packets::StarPiece packet;
        packet.playerId = Packets::PlayerPosition::consoleId;
        packet.timestamp = Timestamps::beacon.isInit() ?
            Timestamps::beacon.convertToServer(Timestamps::now())
            : Timestamps::makeEmptyServerTimestamp();
        packet.initLineStart = tail;
        packet.initLineEnd = tip;
        setDebugMsg(2, transmitter.addPacket(packet).err);
    }

    self.isPlayerGenerated = true;
    self.isNetActorGenerated = false;

    return self.throwToTargetCore(tip, tail, gravity, a, isFall);
}

static void timeAdjust(NetStarPiece &self) {
    Timestamps::LocalTimestamp now = Timestamps::now();
    f32 framesElapsed = Timestamps::differenceMs(self.prevTime, now) / 1000.0f * 60.0f;
    self.mVelocity *= framesElapsed;
    
    // Adjust mStep and change nerves if necessary
   
    f32 frames = Timestamps::differenceMs(self.launchTime, now) / 1000.0f * 60.0f;
    self.mSpine->mStep = frames;
    if(self.mSpine->mStep < 0) self.mSpine->mStep = 0;

    self.prevTime = now;
}

static void netStarPieceExeThrow(NetStarPiece &self) {
    self.exeThrow();
    if(self.isPlayerGenerated || self.isNetActorGenerated) timeAdjust(self);
}

static void netStarPieceExeThrowFall(NetStarPiece &self) {
    self.exeThrowFall();
    if(self.isPlayerGenerated || self.isNetActorGenerated) timeAdjust(self);
}

// All starbits will check whether or not to maintain a consisten real time speed
// but only Player- and NetActor-generated starbits will do this behavior
extern kmSymbol execute__Q212NrvStarPiece16HostTypeNrvThrowCFP5Spine;
extern kmSymbol execute__Q212NrvStarPiece20HostTypeNrvThrowFallCFP5Spine;
kmBranch(&execute__Q212NrvStarPiece16HostTypeNrvThrowCFP5Spine + 0x04, netStarPieceExeThrow);
kmBranch(&execute__Q212NrvStarPiece20HostTypeNrvThrowFallCFP5Spine + 0x04, netStarPieceExeThrowFall);

NetStarPiece::NetStarPiece(const char *name) : StarPiece(name) {
    isPlayerGenerated = false;
    isNetActorGenerated = false;
}

void NetStarPiece::kill() {
    kill();
    isPlayerGenerated = false;
    isNetActorGenerated = false;
}

static bool MarioActorOverrideReceiveMsgPlayerAttack(MarioActor &self, u32 msg, HitSensor *sender, HitSensor *receiver) {
    if (
        MR::isMsgStarPieceAttack(msg) 
        && ((const NetStarPiece*)sender->mActor)->isNetActorGenerated
    ) {
        TVec3f dir = receiver->mPosition - sender->mPosition;
        dir.setLength(30.0f);
        return self.tryVectorAttackMsg(msg, dir);
    }
    return false;
}

inline void* operator new(unsigned long, NetStarPiece *starPiece) {return starPiece;}

static NetStarPiece* generateNetStarPiece(NetStarPiece *starPiece, const char *name) {
    new (starPiece) NetStarPiece(name);
    return starPiece;
}

// Make sure the star pieces given to the director are NetStarPiece instead of StarPiece
extern kmSymbol createStarPiece__2MRFv;
kmWriteInstruction(&createStarPiece__2MRFv + 0x3C, li r3,sizeof(NetStarPiece));
kmCall(&createStarPiece__2MRFv + 0x54, generateNetStarPiece);

extern kmSymbol __vt__10MarioActor;
kmWritePointer(&__vt__10MarioActor + 0x5C, MarioActorOverrideReceiveMsgPlayerAttack);
