#include "netActor.hpp"
#include "multiplayer.hpp"
#include <kamek/hooks.h>
#include <Game/Util/ActorSensorUtil.hpp>

NetPlayerActor multiplayerActor;

inline void* operator new(unsigned long, NetPlayerActor *ret) { return ret; }

static void _init(NetActor &self) {
    self.initCounter++;
    //if(!self.isInit) {
        self.initHitSensor((Multiplayer::MAX_PLAYER_COUNT - 1) * NetActor::NUM_SENSOR_CATEGORIES);
        MR::addHitSensor(&self, "body", 0x14, 8, 60.0f, TVec3f(0.0f));
    //}
    self.mPosition = TVec3f(0.0f);
    self.mPosition.y = -500.0f;
    self.mVelocity = TVec3f(0.0f);
    MR::connectToScene(&self, 0x25, -1, -1, -1);
    MR::invalidateClipping(&self);
    self.appear();
    self.mFlags.mIsDead = false;
    self.isInit = true;
}

void NetActor::init(const JMapInfoIter &) {
    _init(*this);
}

#define SENSOR_MSG_JUMP 0x2C
void NetActor::movement() {
    LiveActor::movement();
    if(cooldown > 0) {
        cooldown--;
        MR::invalidateHitSensors(this);
    }
    else MR::validateHitSensors(this);
}
bool NetActor::receiveMsgPlayerAttack(u32 m, HitSensor *a, HitSensor *b) {
    //HitSensor *target = a->mActor == this ? b : a;
    if(cooldown == 0) {
        //a->receiveMessage(0x2F, b);
        cooldown = 60;
    }
    return MR::isMsgPlayerTrample(m);
}

static void postPlayerInit() {
    /*if(!multiplayerActor.isInit)*/ new (&multiplayerActor) NetPlayerActor("Multiplayer");
    _init(multiplayerActor);
}

extern kmSymbol init2__10MarioActorFRCQ29JGeometry8TVec3fRCQ29JGeometry8TVec3fl;
kmBranch(&init2__10MarioActorFRCQ29JGeometry8TVec3fRCQ29JGeometry8TVec3fl + 0x698, postPlayerInit);

