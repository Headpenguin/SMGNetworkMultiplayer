#include <Game/Player/MarioActor.hpp>
#include <Game/Player/MarioAnimator.hpp>
#include <Game/Player/J3DModelX.hpp>
#include <Game/Util/CameraUtil.hpp>
#include <JSystem/J3DGraphAnimator/J3DMtxBuffer.hpp>
#include <JSystem/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/J3DGraphBase/J3DSys.hpp>
#include <kamek/hooks.h>

#include "multiplayer.hpp"
#include "debug.hpp"

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
        model->mModelData->mJointTree.calc(buffs + i, *model->_18.toCVec(), model->_24); // maybe can fix issue above?
        model->calcWeightEnvelopeMtx();
        buffs[i].calcNrmMtx();
        buffs[i].calcDrawMtx(model->_8 & 3, *model->_18.toCVec(), model->_24);
        DCStoreRangeNoSync(buffs[i].mpDrawMtxArr[1][buffs[i].mCurrentViewNo], model->mModelData->mJointTree.mMatrixData.mDrawMatrixCount * sizeof(Mtx));
        DCStoreRange(buffs[i].mpNrmMtxArr[1][buffs[i].mCurrentViewNo], model->mModelData->mJointTree.mMatrixData.mDrawMatrixCount * sizeof(Mtx33));
        currXanime->clearAnm(0);
    }
}

XanimeGroupInfo* getGroupInfoFromIdx(const XanimePlayer &xanime, s32 idx) {
    return idx >= 0 ? xanime.mResourceTable->_10 + idx : nullptr;
}

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
        
        Mtx &baseMtx = playerBaseMtx[i];

        baseMtx[0][3] = pos.position.x;
        baseMtx[1][3] = pos.position.y;
        baseMtx[2][3] = pos.position.z;

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
        PSVECCrossProduct(v.toCVec(), X.toCVec(), u.toVec());
        v.setLength(z);
        tmp = 1 - z*z;
        u.setLength(sqrt(tmp < 0.0f ? 0.0f : tmp));
        if(isYOrthoNegative) u = -u;
        Y = u + v;
        TVec3f Z;
        PSVECCrossProduct(X.toCVec(), Y.toCVec(), Z.toVec());
        
        baseMtx[0][0] = X.x;
        baseMtx[0][1] = Y.x;
        baseMtx[0][2] = Z.x;
        baseMtx[1][0] = X.y;
        baseMtx[1][1] = Y.y;
        baseMtx[1][2] = Z.y;
        baseMtx[2][0] = X.z;
        baseMtx[2][1] = Y.z;
        baseMtx[2][2] = Z.z;

        setDebugMsgFloat(12, pos.position.y);
        setDebugMsgFloat(16, pos.position.z);

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
    model->mModelData->mJointTree.calc(model->_84, *model->_18.toCVec(), model->_24);
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

