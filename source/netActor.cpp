#include "netActor.hpp"
#include "multiplayer.hpp"
#include "alignment.hpp"
#include "accurateTime.hpp"
#include "beacon.hpp"
#include <kamek/hooks.h>
#include <Game/Util/ActorSensorUtil.hpp>
#include <Game/LiveActor/HitSensorInfo.hpp>
#include <Game/Player/MarioHolder.hpp>
#include <Game/Player/MarioActor.hpp>
static HitSensor netActorSensors[(Multiplayer::MAX_PLAYER_COUNT - 1) * NetActor::NUM_SENSOR_CATEGORIES];

static u8 jumpCooldown[Multiplayer::MAX_PLAYER_COUNT - 1];

NetPlayerActor multiplayerActor;

static struct MarioGVInfo {
    f32 gvComponent;
    Timestamps::LocalTimestamp time;
} mariogvArray[20];

static MarioGVInfo *mariogvHead = mariogvArray;
const static MarioGVInfo *mariogvEnd = mariogvArray + sizeof(mariogvArray) / sizeof(*mariogvArray);

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
    mariogvHead->gvComponent = marioActor->mVelocity.dot(marioActor->mGravity);
    mariogvHead->time = Timestamps::now();
    mariogvHead++;
    if(mariogvHead == mariogvEnd) mariogvHead = mariogvArray;
    for(u8 *i = jumpCooldown; i < jumpCooldown + sizeof(jumpCooldown) / sizeof(*jumpCooldown); i++) {
        if(*i > 0) (*i)--;
    }
}

static bool tryGetMariogv(f32 &gv, Timestamps::LocalTimestamp time) {
    MarioGVInfo *start = mariogvHead;
    s32 length = sizeof(mariogvArray) / sizeof(*mariogvArray);
    MarioGVInfo *idx;
    bool lt = false, gt = false;
    do {
        MarioGVInfo *idx = start + length / 2;
        if(idx >= mariogvEnd) idx -= sizeof(mariogvArray) / sizeof(*mariogvArray);
        Timestamps::LocalTimestamp marioTime = idx->time;
        if(marioTime > time) {
            gt = true;
            length = (length - 1) / 2;
        }
        else if(marioTime < time) {
            lt = true;
            start = idx + 1;
            length /= 2;
        }
        else {
            gv = idx->gvComponent;
            return true;
        }
    } while(length > 1);

    if(lt && gt) {
        Timestamps::LocalTimestamp upper, lower;
        lower = idx->time;
        upper = idx[1].time;
        gv = Timestamps::differenceMs(lower, time) < Timestamps::differenceMs(time, upper) ?
            idx->gvComponent : idx[1].gvComponent;
        return true;
    }
    return false;

}

const static f32 JMP_THRESHOLD = 1.0f; // Somewhere around 1-3, maybe 5

static int cmpMariogv(const MarioActor *mario, const Packets::PlayerPosition &pos) {
    f32 gv;
    if(!tryGetMariogv(gv, Timestamps::beacon.isInit() ? Timestamps::beacon.convertToLocal(pos.timestamp) : pos.arrivalTime)) return 0;
    
    f32 playergv = pos.velocity.dot(mario->mGravity);
    if(gv + JMP_THRESHOLD < playergv) return 1;
    if(gv > playergv + JMP_THRESHOLD) return -1;
    return 0;

}

static bool isTrampleEligible(const MarioActor *mario, u32 player) {
    return cmpMariogv(mario, Multiplayer::access.getPlayerPosRaw(player)) == -1;
}

static bool isTramplingPlayer(const MarioActor *mario, const Packets::PlayerPosition &pos) {
    return cmpMariogv(mario, pos) == 1;
}

bool NetActor::receiveMsgPush(HitSensor *sender, HitSensor *receiver) {
    if(sender->mActor == (LiveActor*)marioActor) {
        u32 player = getPlayerFromSensor(*this, receiver);
        const Packets::PlayerPosition &pos =Multiplayer::access.getPlayerPosRaw(player);
        if(pos.isHipDropStun()) {
            sender->receiveMessage(SENSOR_MSG_ENEMY_ATTACK_FLIP_JUMP, receiver);
        }
        else if(jumpCooldown[player] == 0 && isTramplingPlayer(marioActor, pos)) {
            jumpCooldown[player] = 60;
            marioActor->trampleJump(-91.2f, 1.0f);
        }
        else {
            sender->receiveMessage(SENSOR_MSG_PUSH, receiver);
        }
    }

    return true;
}

bool NetActor::receiveMsgPlayerAttack(u32 m, HitSensor *sender, HitSensor *receiver) {
    if(!MR::isMsgPlayerTrample(m)) return false;
    u32 player = getPlayerFromSensor(*this, receiver);
    //HitSensor *target = a->mActor == this ? b : a;
    if(jumpCooldown[player] == 0) {
        if(isTrampleEligible(marioActor, player) ) { // This console's player got the jump
            jumpCooldown[player] = 60;
            return true;
        }
    }
    return false;
}

static void postPlayerInit() {
    new (&multiplayerActor) NetPlayerActor("Multiplayer");
    _init(multiplayerActor);
}

extern kmSymbol init2__10MarioActorFRCQ29JGeometry8TVec3fRCQ29JGeometry8TVec3fl;
kmBranch(&init2__10MarioActorFRCQ29JGeometry8TVec3fRCQ29JGeometry8TVec3fl + 0x698, postPlayerInit);

