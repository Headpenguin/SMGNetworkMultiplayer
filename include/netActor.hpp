#ifndef NETACTOR_HPP
#define NETACTOR_HPP

#include <Game/LiveActor/LiveActor.hpp>

class NetActor : public LiveActor {
public:
    static const u32 NUM_SENSOR_CATEGORIES = 1;
    inline NetActor(const char *name) : LiveActor(name), cooldown(0) {}
    inline NetActor() : LiveActor("dummy") {}
    virtual void init(const JMapInfoIter &);
    virtual bool receiveMsgPlayerAttack(u32, HitSensor*, HitSensor*);
    virtual void movement();
    
    bool isInit;
    u32 initCounter;
    u8 cooldown;
};

class NetPlayerActor : public NetActor {
public:
    inline NetPlayerActor(const char *name) : NetActor(name) {}
    inline NetPlayerActor() {}
};

#endif
