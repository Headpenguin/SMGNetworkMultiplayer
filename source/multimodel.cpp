#include <Game/Player/MarioActor.hpp>
#include <Game/Player/MarioAnimator.hpp>
#include <Game/Player/J3DModelX.hpp>
#include <Game/Util/CameraUtil.hpp>
#include <JSystem/J3DGraphAnimator/J3DMtxBuffer.hpp>
#include <kamek/hooks.h>

#include "multiplayer.hpp"

J3DMtxBuffer playerBuffs[Multiplayer::MAX_PLAYER_COUNT - 1];
Mtx playerBaseMtx[Multiplayer::MAX_PLAYER_COUNT - 1];

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

void calcAnim(J3DModel *model, const Mtx *base, J3DMtxBuffer *buffs, u32 numBuffs) {
    for(u32 i = 0; i < numBuffs; i++) {
        PSMTXCopy(base[i], model->_24); // inefficient!
        model->_84 = buffs + i;

        model->mModelData->mJointTree.calc(buffs + i, *model->_18.toCVec(), model->_24); // maybe can fix issue above?
        model->calcWeightEnvelopeMtx();
        buffs[i].calcDrawMtx(model->_8 & 3, *model->_18.toCVec(), model->_24);
        DCStoreRangeNoSync(buffs[i].mpDrawMtxArr[1][buffs[i].mCurrentViewNo], model->mModelData->mJointTree.mMatrixData.mDrawMatrixCount * sizeof(Mtx));
    }
}

void calcAnim_ep(MarioAnimator *anim) {
    J3DModel *model = anim->mActor->getJ3DModel();
    
    for(u32 i = 0; i < Multiplayer::MAX_PLAYER_COUNT - 1; i++) {
        //PSMTXCopy(model->_24, playerBaseMtx[i]);
        if(!Multiplayer::getMostRecentBuffer(i, Multiplayer::info.activityStatus)) continue;

        u32 buffIdx = Multiplayer::getMostRecentBuffer(i, Multiplayer::info.status);
        Multiplayer::PlayerDoubleBuffer &doubleBuffer = Multiplayer::info.players[i];

        if(!simplelock_tryLock(&doubleBuffer.locks[buffIdx])) {
            buffIdx = buffIdx == 1 ? 0 : 1;
            if(!simplelock_tryLock(&doubleBuffer.locks[buffIdx])) continue; // invalid state
        }

        const Packets::PlayerPosition &pos = doubleBuffer.pos[buffIdx];

        playerBaseMtx[i][0][3] = pos.position.x;
        playerBaseMtx[i][1][3] = pos.position.y;
        playerBaseMtx[i][2][3] = pos.position.z;
    }
    
    Mtx tmpBaseMtx;
    PSMTXCopy(model->_24, tmpBaseMtx);
    J3DMtxBuffer *tmpMtxBuffer = model->_84;
    
    calcAnim(model, playerBaseMtx, playerBuffs, Multiplayer::MAX_PLAYER_COUNT - 1);
    
    PSMTXCopy(tmpBaseMtx, model->_24);
    model->_84 = tmpMtxBuffer;
    
    anim->calc();
}

extern kmSymbol calcAnim__10MarioActorFv;
// Replaces `MarioAnimator::calcAnim`
kmCall(&calcAnim__10MarioActorFv + 0x2A4, calcAnim_ep);

void drawAll(J3DModelX *model, J3DMtxBuffer *buffs, u32 numBuffs) {
    for(u32 i = 0; i < numBuffs; i++) {
        model->_84 = buffs + i;
        model->prepareShapePackets();
        model->directDraw(nullptr);
    }
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

