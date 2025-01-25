#include "netActor.hpp"
#include "multiplayer.hpp"
#include "alignment.hpp"
#include <kamek/hooks.h>
#include <Game/Util/ActorSensorUtil.hpp>
#include <Game/LiveActor/HitSensorInfo.hpp>
#include <Game/Player/MarioHolder.hpp>
static HitSensor netActorSensors[(Multiplayer::MAX_PLAYER_COUNT - 1) * NetActor::NUM_SENSOR_CATEGORIES];

NetPlayerActor multiplayerActor;

inline void* operator new(unsigned long, NetPlayerActor *ret) { return ret; }
inline void* operator new(unsigned long, HitSensor *ret) { return ret; }

#define SENSOR_TYPE_KURIBO 0x14

static u32 getPlayerFromSensor(const NetActor &self, const HitSensor *sensor) {
    return (sensor - netActorSensors) % (Multiplayer::MAX_PLAYER_COUNT - 1);
}

void NetActor::updateHitSensor(HitSensor *sensor) {
    u32 playerIdx = getPlayerFromSensor(*this, sensor);
    
    if (
        !Multiplayer::access.isPlayerActive(playerIdx) 
        || !Multiplayer::access.isPlayerPosEstimateSet(playerIdx)
    ) {
        sensor->invalidate();
        return;
    }
    sensor->validate();

    const AlignmentState &posEstimate = 
        Multiplayer::access.getPlayerPosEstimate(playerIdx);

    sensor->mPosition = posEstimate.getPos();
}

static void _init(NetActor &self) {
    self.initCounter++;
    //if(!self.isInit) {
    self.initHitSensor((Multiplayer::MAX_PLAYER_COUNT - 1) * NetActor::NUM_SENSOR_CATEGORIES);
    for(u32 i = 0; i < Multiplayer::MAX_PLAYER_COUNT - 1; i++) {
        new (netActorSensors + i) HitSensor(SENSOR_TYPE_KURIBO, 8, 60.0f, &self);
        self.mSensorKeeper->registHitSensorInfo(
            new HitSensorInfo(
                "body", netActorSensors + i, nullptr, nullptr, TVec3f(0.0f), true
            )
        );
    }
    MR::invalidateHitSensors(&self);
    //}
    self.mPosition = TVec3f(0.0f);
    //self.mPosition.y = -500.0f;
    self.mVelocity = TVec3f(0.0f);
    MR::connectToScene(&self, 0x25, -1, -1, -1);
    MR::invalidateClipping(&self);
    self.appear();
    self.mFlags.mIsDead = false;
    self.marioActor = MR::getMarioHolder()->getMarioActor();
}

void NetActor::init(const JMapInfoIter &) {
    _init(*this);
}

#define SENSOR_MSG_PUSH 0x29
#define SENSOR_MSG_JUMP 0x2C
#define SENSOR_MSG_ENEMY_ATTACK_FLIP_JUMP 0x4F
void NetActor::movement() {
    LiveActor::movement();

}

bool NetActor::receiveMsgPush(HitSensor *sender, HitSensor *receiver) {
    if(sender->mActor == (LiveActor*)marioActor) {
        sender->receiveMessage(
            Multiplayer::access
                .getPlayerPosRaw(getPlayerFromSensor(*this, receiver))
                .isHipDropStun() ? 
                    SENSOR_MSG_ENEMY_ATTACK_FLIP_JUMP
                    : SENSOR_MSG_PUSH,
            receiver
        );
    }

    return true;
}

bool NetActor::receiveMsgPlayerAttack(u32 m, HitSensor *a, HitSensor *b) {
    //HitSensor *target = a->mActor == this ? b : a;
    if(cooldown == 0) {
        //a->receiveMessage(0x2F, b);
        cooldown = 60;
    }
    return false;
    //return MR::isMsgPlayerTrample(m);
}

static void postPlayerInit() {
    new (&multiplayerActor) NetPlayerActor("Multiplayer");
    _init(multiplayerActor);
}

extern kmSymbol init2__10MarioActorFRCQ29JGeometry8TVec3fRCQ29JGeometry8TVec3fl;
kmBranch(&init2__10MarioActorFRCQ29JGeometry8TVec3fRCQ29JGeometry8TVec3fl + 0x698, postPlayerInit);

