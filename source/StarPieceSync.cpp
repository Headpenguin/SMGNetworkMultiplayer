#include <Game/Util/ActorSensorUtil.hpp>
#include <Game/Player/MarioActor.hpp>

#include <kamek/hooks.h>

static bool MarioActorOverrideReceiveMsgPlayerAttack(MarioActor &self, u32 msg, HitSensor *sender, HitSensor *receiver) {
    if(MR::isMsgStarPieceAttack(msg)) {
        TVec3f dir = receiver->mPosition - sender->mPosition;
        dir.setLength(30.0f);
        return self.tryVectorAttackMsg(msg, dir);
    }
    return false;
}


extern kmSymbol __vt__10MarioActor;
kmWritePointer(&__vt__10MarioActor + 0x5C, MarioActorOverrideReceiveMsgPlayerAttack);
