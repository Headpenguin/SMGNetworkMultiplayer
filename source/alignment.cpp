#include "alignment.hpp"
#include "beacon.hpp"
#include "accurateTime.hpp"
#include <Game/Util/MathUtil.hpp>

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
    f32 timeout_frames,
    f32 epsilon
) : acceleration(acceleration), 
    maxRelativeSpeed(maxRelativeSpeed),
    timeout_frames(timeout_frames),
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


namespace implementation {
    class Alignment {
    public:
        static void stop(AlignmentState::HomingAlignmentPlan &self) {
            self.relativePosition.zero();
            self.relativeVelocity.zero();
            self.updateAbsolute();
            self.complete = true;
        }
    };
}

void AlignmentState::HomingAlignmentPlan::update(f32 framesElapsed) {
    
    if(complete) return;

    updateRelative();
    
    if(MR::isNearZero(relativePosition, sEpsilon * framesElapsed) && MR::isNearZero(relativeVelocity, vEpsilon * framesElapsed)) {
        implementation::Alignment::stop(*this);
        return;
    }

    f32 s = PSVECMag(&relativePosition);
    f32 v_sq = relativeVelocity.squared();
    f32 v = sqrt(v_sq);
        
    if(v_sq > (s - sEpsilon * framesElapsed) * acceleration * 2.0f) { // slow down
        f32 targetSpeed = 1.0f - acceleration * framesElapsed / v;
        targetSpeed = targetSpeed < 0.0f ? 0.0f : targetSpeed;
        relativeVelocity *= targetSpeed;
    }
    else { // approach estimate
        f32 speedComponent = v > maxRelativeSpeed ? 
            -relativeVelocity.dot(relativePosition) / s 
            : 0.0f;

        f32 targetSpeed = maxRelativeSpeed > speedComponent ? 
            maxRelativeSpeed 
            : speedComponent;

        if(s / targetSpeed > timeout_frames) {
            implementation::Alignment::stop(*this);
            return;
        }

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
