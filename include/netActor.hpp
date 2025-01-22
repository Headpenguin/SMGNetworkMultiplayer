#ifndef NETACTOR_HPP
#define NETACTOR_HPP

#include <Game/LiveActor/LiveActor.hpp>

class NetActor : public LiveActor {
public:
    enum {
        SENSOR_BODY = 0,
        NUM_SENSOR_CATEGORIES
    };
    inline NetActor(const char *name) : LiveActor(name), cooldown(0) {}
    inline NetActor() : LiveActor("dummyNetActor") {}
    virtual void init(const JMapInfoIter &);
    virtual bool receiveMsgPlayerAttack(u32, HitSensor*, HitSensor*);
    virtual bool receiveMsgPush(HitSensor*, HitSensor*);
    virtual void movement();
    virtual void updateHitSensor(HitSensor *);
    
    u32 initCounter;
    u8 cooldown;
    MarioActor *marioActor;
};

class NetPlayerActor : public NetActor {
public:
    inline NetPlayerActor(const char *name) : NetActor(name) {}
    inline NetPlayerActor() {}
};

#endif
