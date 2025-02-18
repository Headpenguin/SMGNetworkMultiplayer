#include <Game/Player/MarioActor.hpp>
#include <Game/Player/MarioAnimator.hpp>
#include <Game/Player/J3DModelX.hpp>
#include <Game/Util/CameraUtil.hpp>
#include <JSystem/J3DGraphAnimator/J3DMtxBuffer.hpp>
#include <JSystem/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/J3DGraphBase/J3DSys.hpp>
#include <kamek/hooks.h>

#include "multiplayer.hpp"
#include "beacon.hpp"
#include "debug.hpp"
#include "accurateTime.hpp"

//static const BASE_INTERPOLATE_EPSILON = 0.1f;

const static f32 STANDARD_FPS = 60.0f;
const static f32 CORRECTION_ACCEL = 10.0f;
const static f32 CORRECTION_MAX_V = 15.0f;
const static f32 CORRECTION_S_EPSILON = 20.0f;

// Persists after receiving new positions
class AlignmentState {
public:

    // Does not persist after receiving new positions
    class HomingAlignmentPlan {
        f32 acceleration;
        f32 maxRelativeSpeed;
        
        f32 sEpsilon, vEpsilon;

        TVec3f origin, referenceVelocity;
        TVec3f relativePosition, relativeVelocity;
        TVec3f *absPosition, *absVelocity;
        Timestamps::LocalTimestamp time;
        bool initFlag;
        bool complete;

        void updateRelative();
        void updateAbsolute();
        void updateReferenceFrame(f32 framesElapsed);
    public:
        HomingAlignmentPlan(const Packets::PlayerPosition &truePos, f32 acceleration, f32 maxRelativeSpeed, f32 epsilon);
        inline HomingAlignmentPlan() : initFlag(false) {}
        void init(TVec3f &absPos, TVec3f &absVelocity, Timestamps::LocalTimestamp now);
        inline bool isInit() const {
            return initFlag;
        }
        void update(f32 framesElapsed);
        inline bool isInvalid(const HomingAlignmentPlan &other) const {
            return other.time > time || !initFlag;
        }
    };

private:
    HomingAlignmentPlan plan;
    TVec3f position;
    TVec3f velocity;
    Timestamps::LocalTimestamp time;

public:

    inline AlignmentState() {}
    AlignmentState(const TVec3f &pos, const TVec3f &velocity, Timestamps::LocalTimestamp time);

    // Could use more generic AlignmentPlan if implemented
    void invalidateAlignmentPlan(const HomingAlignmentPlan &plan);
    void update(Timestamps::LocalTimestamp now);

    const TVec3f& getPos() const;

};

AlignmentState::AlignmentState(const TVec3f &pos, const TVec3f &velocity, Timestamps::LocalTimestamp time) 
    : position(pos), velocity(velocity), time(time), plan() {}

void AlignmentState::invalidateAlignmentPlan(const AlignmentState::HomingAlignmentPlan &_plan) {
    if(plan.isInvalid(_plan)) {
        plan = _plan;
        plan.init(position, velocity, time);
    }
}

void AlignmentState::update(Timestamps::LocalTimestamp now) {
    f32 framesElapsed = Timestamps::differenceMs(time, now) /** realtimeRate*/ / 1000.0f * STANDARD_FPS;
    if(plan.isInit()) plan.update(framesElapsed);
    position += velocity * framesElapsed;
    time = now;
}

const TVec3f& AlignmentState::getPos() const {
    return position;
}

AlignmentState::HomingAlignmentPlan::HomingAlignmentPlan (
    const Packets::PlayerPosition &truePos, 
    f32 acceleration, 
    f32 maxRelativeSpeed,
    f32 epsilon
) : acceleration(acceleration), 
    maxRelativeSpeed(maxRelativeSpeed),
    sEpsilon(epsilon),
    vEpsilon(acceleration),
    origin(truePos.position),
    referenceVelocity(truePos.velocity),
    initFlag(false),
    complete(false)
{
    time = Timestamps::beacon.isInit() && !Timestamps::isEmpty(truePos.timestamp) ?
        Timestamps::beacon.convertToLocal(truePos.timestamp)
        : truePos.arrivalTime;
}

void AlignmentState::HomingAlignmentPlan::updateRelative() {
    relativePosition = *absPosition - origin;
    relativeVelocity = *absVelocity - referenceVelocity;
}

void AlignmentState::HomingAlignmentPlan::updateAbsolute() {
    *absPosition = relativePosition + origin;
    *absVelocity = relativeVelocity + referenceVelocity;
}

void AlignmentState::HomingAlignmentPlan::updateReferenceFrame(f32 framesElapsed) {
    TVec3f ds = referenceVelocity * framesElapsed;
    origin += ds;
}

void AlignmentState::HomingAlignmentPlan::init(TVec3f &currPos, TVec3f &currVelocity, Timestamps::LocalTimestamp now) {
    absPosition = &currPos;
    absVelocity = &currVelocity;
    f32 framesElapsed = Timestamps::differenceMs(time, now) / 1000.0f * STANDARD_FPS;
    updateReferenceFrame(framesElapsed);
    initFlag = true;
}

void AlignmentState::HomingAlignmentPlan::update(f32 framesElapsed) {
    
    if(complete) return;

    updateRelative();
    
    if(MR::isNearZero(relativePosition, sEpsilon * framesElapsed) && MR::isNearZero(relativeVelocity, vEpsilon * framesElapsed)) {
        relativePosition.zero();
        relativeVelocity.zero();
        updateAbsolute();
        complete = true;
        return;
    }

    f32 s = PSVECMag(&relativePosition);
    f32 v_sq = relativeVelocity.squared();
    f32 v = sqrt(v_sq);

    if(v_sq > (s - sEpsilon * framesElapsed) * acceleration * 2.0f) { // stop
        f32 targetSpeed = 1.0f - acceleration * framesElapsed / v;
        targetSpeed = targetSpeed < 0.0f ? 0.0f : targetSpeed;
        relativeVelocity *= targetSpeed;
    }
    else { // home
        f32 speedComponent = v > maxRelativeSpeed ? 
            -relativeVelocity.dot(relativePosition) / s 
            : 0.0f;

        f32 targetSpeed = maxRelativeSpeed > speedComponent ? 
            maxRelativeSpeed 
            : speedComponent;
        f32 stopSpeed = sqrt(s * acceleration * 2.0f);
        targetSpeed = stopSpeed < targetSpeed ? stopSpeed : targetSpeed;
    
        TVec3f velocityDifferenceCurrentTarget = -relativePosition;
        velocityDifferenceCurrentTarget.setLength(targetSpeed);
        velocityDifferenceCurrentTarget -= relativeVelocity;

        TVec3f velocityChange = velocityDifferenceCurrentTarget;
        velocityChange.setLength(framesElapsed * acceleration);

        f32 maxChange = PSVECMag(&velocityDifferenceCurrentTarget);
        if(maxChange < PSVECMag(&velocityChange)) {
            velocityChange.setLength(maxChange);
        }
        relativeVelocity += velocityChange;
        if(PSVECMag(&relativeVelocity) > targetSpeed) relativeVelocity.setLength(targetSpeed);
    }

    updateAbsolute();
    updateReferenceFrame(framesElapsed);
}

static struct {
    AlignmentState state;
    bool isInit;
} alignmentStates[Multiplayer::MAX_PLAYER_COUNT - 1];

J3DMtxBuffer playerBuffs[Multiplayer::MAX_PLAYER_COUNT - 1];
Mtx playerBaseMtx[Multiplayer::MAX_PLAYER_COUNT - 1];

struct XanimeWrapper {u32 raw[sizeof(XanimePlayer) / sizeof(u32)];};
static XanimeWrapper xanimeWrapper[Multiplayer::MAX_PLAYER_COUNT - 1];
static XanimeWrapper xanimeUpperWrapper[Multiplayer::MAX_PLAYER_COUNT - 1];

static XanimePlayer *playerXanimes = (XanimePlayer *)xanimeWrapper;
static XanimePlayer *playerUpperXanimes = (XanimePlayer *)xanimeUpperWrapper;

void createMtxBuffers(J3DModelData *data, const J3DMtxBuffer *basis, J3DMtxBuffer *dstArray, u32 numBuffs) {
    for(u32 i = 0; i < numBuffs; i++) {
        memcpy(dstArray + i, basis, sizeof(*basis));
        dstArray[i].createDoubleDrawMtx(data, 1);
    }
}

J3DModelX* createMtxBuffers_ep(MarioActor *self) {
    J3DModelX *model = (J3DModelX*) MR::getJ3DModel(self);
    createMtxBuffers(model->mModelData, model->_84, playerBuffs, Multiplayer::MAX_PLAYER_COUNT - 1); // inconsistent
    return model;
}

extern kmSymbol initDrawAndModel__10MarioActorFv;
// Replaces `LiveActor::getJ3DModel` immediately after `LiveActor::initModelManagerWithAnm`
kmCall(&initDrawAndModel__10MarioActorFv + 0x214, createMtxBuffers_ep);

void createXanimes(MarioAnimator *anim) {
    anim->init();

    for(u32 i = 0; i < Multiplayer::MAX_PLAYER_COUNT - 1; i++) {
        playerXanimes[i] = XanimePlayer(MR::getJ3DModel(anim->mActor), anim->mResourceTable);
        playerXanimes->changeAnimation("\x97\x8e\x89\xba"); 
        playerXanimes->setDefaultAnimation("\x97\x8e\x89\xba");
        playerXanimes[i].mCore->enableJointTransform(MR::getJ3DModel(anim->mActor)->mModelData);


//        playerUpperXanimes[i] = XanimePlayer(MR::getJ3DModel(anim->mActor), anim->mResourceTable);
//        playerUpperXanimes[i].mCore->enableJointTransform(MR::getJ3DModel(anim->mActor)->mModelData);
    }
}
extern kmSymbol __ct__13MarioAnimatorFP10MarioActor;
kmCall(&__ct__13MarioAnimatorFP10MarioActor + 0x24, createXanimes);

void calcAnim(MarioAnimator *anim, J3DModel *model, const Mtx *base, J3DMtxBuffer *buffs, u32 numBuffs) {
    j3dSys.mCurrentModel = model;
    for(u32 i = 0; i < numBuffs; i++) {
        if(!Multiplayer::getMostRecentBuffer(i, Multiplayer::info.activityStatus)) continue;
        
        PSMTXCopy(base[i], model->_24); // inefficient!
        model->_84 = buffs + i;
        
        XanimePlayer *currXanime = &playerXanimes[i];
        if(currXanime->mModel != model) currXanime->setModel(model);
        currXanime->updateBeforeMovement();
        currXanime->updateAfterMovement();
        u32 idx = MR::getJointIndex(anim->mActor, "Spine1");
        model->mModelData->mJointTree.mJointsByIdx[idx]->mMtxCalc = nullptr;
        currXanime->calcAnm(0);
        /*currXanime->mCore->_6 = 1;

        model->mModelData->mJointTree.calc(buffs + i, *model->_18.toCVec(), model->_24); // maybe can fix issue above?
        model->calcWeightEnvelopeMtx();
        buffs[i].calcNrmMtx();
        buffs[i].calcDrawMtx(model->_8 & 3, *model->_18.toCVec(), model->_24);*/
        currXanime->mCore->_6 = 3;
        model->mModelData->mJointTree.calc(buffs + i, model->_18, model->_24); // maybe can fix issue above?
        model->calcWeightEnvelopeMtx();
        buffs[i].calcNrmMtx();
        buffs[i].calcDrawMtx(model->_8 & 3, model->_18, model->_24);
        DCStoreRangeNoSync(buffs[i].mpDrawMtxArr[1][buffs[i].mCurrentViewNo], model->mModelData->mJointTree.mMatrixData.mDrawMatrixCount * sizeof(Mtx));
        DCStoreRange(buffs[i].mpNrmMtxArr[1][buffs[i].mCurrentViewNo], model->mModelData->mJointTree.mMatrixData.mDrawMatrixCount * sizeof(Mtx33));
        currXanime->clearAnm(0);
    }
}

XanimeGroupInfo* getGroupInfoFromIdx(const XanimePlayer &xanime, s32 idx) {
    return idx >= 0 ? xanime.mResourceTable->_10 + idx : nullptr;
}

/*
class VectorInterpolation {
    TVec3f curr, step;
public:
    VectorInterpolation(TVec3f curr, TVec3f end, u32 time) : curr(curr), step((end - curr) / time) {}
    void update(f32 dt) {
        curr += step * dt;
    }
    TVec3f get() const {return curr;}
};*/

/*class VectorInterpolation {
    f32 currMag;
    f32 endMag;
    f32 step;
    TVec3f currDir;
    TVec3f endDir;

    f32 dirStep;
    TVec3f endDirStepMag;

    f32 magEpsilon;
    f32 dirEpsilon;

public:
    // t is in iterations
    VectorInterpolation(const TVec3f &start, const TVec3f &end, f32 t) 
        : currMag(PSVecMag(start.toCVec())), endMag(PSVecMag(end.toCVec())), 
        step((endMag - currMag) / t), currDir(start * (1.0f / currMag))
    {
        endDir = end * (1.0f / endMag);
        dirStepMag = acos(endDir.dot(currDir)) * PI;
        endDirStepMag = dirStepMag;
        
        magEpsilon = step * BASE_INTERPOLATE_EPSILON;
        dirEpsilon = dirStepMag * BASE_INTERPOLATE_EPSILON;
    }
    void update() {
        if(MR::isNearZero(currMag - endMag, magEpsilon)) currMag += step;
        if(currDir.epsilonEquals(endDir, dirEpsilon)) {
            TVec3f stepDir;
            JMAVECScaleAdd(endDirStepMag.toCVec(), currDir.toCVec(), stepDir.toVec(), -currDir.dot(endDirStepMag));
            currDir += stepDir;
            currDir.setLength(1.0f);
        }

    }
    TVec3f get() const {
        return currDir * currMag;
    }
};*/

void calcAnim_ep(MarioAnimator *anim) {
    J3DModel *model = anim->mActor->getJ3DModel();
    
    for(u32 i = 0; i < Multiplayer::MAX_PLAYER_COUNT - 1; i++) {
        if(!Multiplayer::getMostRecentBuffer(i, Multiplayer::info.activityStatus)) continue;

        u32 buffIdx = Multiplayer::getMostRecentBuffer(i, Multiplayer::info.status);
        Multiplayer::PlayerDoubleBuffer &doubleBuffer = Multiplayer::info.players[i];

        if(simplelock_tryLockLoop(&doubleBuffer.locks[buffIdx]) != TRY_LOCK_RESULT_OK) {
            buffIdx = buffIdx == 1 ? 0 : 1;
            if(simplelock_tryLockLoop(&doubleBuffer.locks[buffIdx]) != TRY_LOCK_RESULT_OK) {
                continue; // invalid state
            }
        }

        const Packets::PlayerPosition pos = doubleBuffer.pos[buffIdx];
        
        simplelock_release(&doubleBuffer.locks[buffIdx]);

        if(!alignmentStates[i].isInit) {
            
            alignmentStates[i].state = AlignmentState (
                pos.position, 
                pos.velocity, 
                Timestamps::beacon.isInit() && !Timestamps::isEmpty(pos.timestamp) ? 
                    Timestamps::beacon.convertToLocal(pos.timestamp) 
                    : pos.arrivalTime // still have to fix the race condition
            );

            alignmentStates[i].isInit = true;
        }
        else {
            alignmentStates[i].state.invalidateAlignmentPlan(AlignmentState::HomingAlignmentPlan(pos, CORRECTION_ACCEL, CORRECTION_MAX_V, CORRECTION_S_EPSILON));
            alignmentStates[i].state.update(Timestamps::now());
        }
        
        Mtx &baseMtx = playerBaseMtx[i];

        const TVec3f &res = alignmentStates[i].state.getPos();

        baseMtx[0][3] = res.x;
        baseMtx[1][3] = res.y;
        baseMtx[2][3] = res.z;

        bool isXzNegative = pos.direction.x > 1.5f;
        bool isYOrthoNegative = pos.direction.z > 1.5f;
        TVec3f X(pos.direction.x - (isXzNegative ? 3.0f : 0.0f), pos.direction.y, 0.0f);
        
        f32 tmp = 1.0f - X.x * X.x - X.y * X.y;
        X.z = sqrt(tmp < 0.0f ? 0.0f : tmp);
        if(isXzNegative) X.z = -X.z;

        TVec3f Y;
        TVec3f v(0.0f, 0.0f, 0.0f); // (i|j) x X
        TVec3f u;

        if(X.x < 0.99f && X.x > -0.99f) {
            v.z = X.y;
            v.y = -X.z;
        }
        else {
            v.x = X.z;
            v.z = -X.x;
        }
        f32 z = pos.direction.z - (isYOrthoNegative ? 3.0f : 0.0f);
        PSVECCrossProduct(&v, &X, &u);
        v.setLength(z);
        tmp = 1 - z*z;
        u.setLength(sqrt(tmp < 0.0f ? 0.0f : tmp));
        if(isYOrthoNegative) u = -u;
        Y = u + v;
        TVec3f Z;
        PSVECCrossProduct(&X, &Y, &Z);
        
        baseMtx[0][0] = X.x;
        baseMtx[0][1] = Y.x;
        baseMtx[0][2] = Z.x;
        baseMtx[1][0] = X.y;
        baseMtx[1][1] = Y.y;
        baseMtx[1][2] = Z.y;
        baseMtx[2][0] = X.z;
        baseMtx[2][1] = Y.z;
        baseMtx[2][2] = Z.z;

        //setDebugMsgFloat(12, pos.position.y);
        //setDebugMsgFloat(16, pos.position.z);

        XanimeGroupInfo *info = getGroupInfoFromIdx(playerXanimes[i], pos.currAnmIdx);
        setDebugMsg(8, 0);
        if(info) playerXanimes[i].changeAnimation(info);
        else setDebugMsg(8, 1);
        
        info = getGroupInfoFromIdx(playerXanimes[i], pos.defaultAnmIdx);
        setDebugMsg(9, 0);
        if(info) playerXanimes[i].mDefaultAnimation = info;
        else setDebugMsg(9, 1);
        
        playerXanimes[i]._20->mSpeed = pos.anmSpeed;

    }
    
    Mtx tmpBaseMtx;
    PSMTXCopy(model->_24, tmpBaseMtx);
    J3DMtxBuffer *tmpMtxBuffer = model->_84;
    
    calcAnim(anim, model, playerBaseMtx, playerBuffs, Multiplayer::MAX_PLAYER_COUNT - 1);
    
    PSMTXCopy(tmpBaseMtx, model->_24);
    model->_84 = tmpMtxBuffer;
    
    anim->calc();
}

extern kmSymbol calcAnim__10MarioActorFv;
// Replaces `MarioAnimator::calcAnim`
kmCall(&calcAnim__10MarioActorFv + 0x2A4, calcAnim_ep);

void drawAll(J3DModelX *model, J3DMtxBuffer *buffs, u32 numBuffs) {
    MR::showJoint(model, "Face0");
    MR::showJoint(model, "HandL0");
    MR::showJoint(model, "HandR0");
    for(u32 i = 0; i < numBuffs; i++) {
        if(!Multiplayer::getMostRecentBuffer(i, Multiplayer::info.activityStatus)) continue;
        model->_84 = buffs + i;
        model->prepareShapePackets();
        model->directDraw(nullptr);
    }
    MR::hideJoint(model, "Face0");
    MR::hideJoint(model, "HandL0");
    MR::hideJoint(model, "HandR0");
}

void drawAll_ep(J3DModelX *model, J3DModel *model2) {
    model->directDraw(model2);
    J3DMtxBuffer *tmp = model->_84;
    //Mtx tmpMtx;
    //PSMTXCopy(model->_24, tmpMtx);
    //calcAnim(model, playerBaseMtx, playerBuffs, 1);

    //PSMTXCopy(tmpMtx, model->_24);
    drawAll(model, playerBuffs, Multiplayer::MAX_PLAYER_COUNT - 1);
    model->_84 = tmp;
}


extern kmSymbol drawMarioModel__10MarioActorCFv;
// Replaces `J3DModelX::directDraw`
kmCall(&drawMarioModel__10MarioActorCFv + 0x1B8, drawAll_ep);

void drawModel2(MarioActor *actor) {
//    f32 tmpA, tmpB, tmpC;
//    J3DModelX *model = actor->mModels[actor->mCurrModel];
    J3DModelX *model = (J3DModelX *) actor->getJ3DModel();
    model->directDraw(nullptr);
//    GXDrawDone();

    model->_24[1][3] += 500.0f;
   // actor->mMarioAnim->calc();
   
//    model->calc();
    model->mModelData->mJointTree.calc(model->_84, model->_18, model->_24);
    model->calcWeightEnvelopeMtx();
//    if(model->_10) model->_10(model, 0);
    
    //MR::updateModelDiffDL(actor);
//    model->viewCalc3(0, nullptr);
//    ((J3DMtxBuffer2 *)model->_84)->rotationMtx((MtxPtr)model->_E0[model->_DC]);
//    model->_84->calcDrawMtx(model->_8 & 3, *model->_18.toCVec(), model->_24);
//    model->directDraw(nullptr);
//    GXDrawDone();
/*    model->_84->swapNrmMtx();
    ((J3DMtxBuffer2*)model->_84)->calcNrmMtx2();
    model->calcBBoardMtx();
    model->calcBumpMtx();
    DCStoreRangeNoSync(model->getDrawMtxPtr(), model->mModelData->mJointTree.mMatrixData.mDrawMatrixCount * 0x30);
    model->prepareShapePackets();*/
//    tmpA = model->_24[0][3];
//    tmpB = model->_24[1][3];
//    tmpC = model->_24[2][3];
//    model->_24[0][3] = 4.0f;
//    model->_24[2][3] = 4.0f;
    //actor->mMarioAnim->calc();
    //MR::updateModelDiffDL(actor);
//    actor->mModelManager->calcAnim();
//    model->update();
//    MR::updateModelDiffDL(actor);
    //model->viewCalc3(0, nullptr);
    //model->directDraw(nullptr);

    model->_24[1][3] -= 500.0f;
//    MR::loadViewMtx();
//    model->setDrawView(0);
//    model->viewCalc();
   // actor->updateModelMtx(actor, nullptr, nullptr);
//    model->directDraw(nullptr);

    
//    model->_24[0][3] = tmpA;
//    model->_24[1][3] = tmpB;
//    model->_24[2][3] = tmpC;

    model->mFlags = 0;
 //   model->_1D0++;

//    model->setDrawView(0);
//    model->directDraw(nullptr);


}

void copyFake(u32, MtxPtr mtx) {
    mtx[0][3] = 0.0f;
    mtx[1][3] = -500.0f;
    mtx[2][3] = 0.0f;
}

