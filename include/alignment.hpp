#ifndef ALIGNMENT_HPP
#define ALIGNMENT_HPP

#include <revolution/types.h>
#include <JSystem/JGeometry/TVec.hpp>

#include "timestamps.hpp"
#include "packets/playerPosition.hpp"

namespace implementation {
    class Alignment;
};

// Persists after receiving new positions
class AlignmentState {
    friend class ::implementation::Alignment;
public:

    // Does not persist after receiving new positions
    class HomingAlignmentPlan {
        friend class ::implementation::Alignment;
        f32 acceleration;
        f32 maxRelativeSpeed;
        f32 timeout_frames;
        
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
        HomingAlignmentPlan(const Packets::PlayerPosition &truePos, f32 acceleration, f32 maxRelativeSpeed, f32 timeout_frames, f32 epsilon);
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

#endif
