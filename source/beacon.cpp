#include "beacon.hpp"

namespace Timestamps {

static struct {
    u32 baseOSTimeMs;
#ifdef DOLPHIN
    u32 baseDolTimeMs;
#endif
} beaconTimebase = {0};

#ifdef DOLPHIN

#define IOCTL_DOLPHIN_GET_SYSTEM_TIME 0x01

static int fdDolphinDevice = -1;
static const char *fsDolphinDevice = "/dev/dolphin";

static struct {
    IOSIoVector v[1] __attribute__((aligned(32)));
    u32 timeMs __attribute__((aligned(32)));
} dolphinGetTimeInfo = {0};

static struct {

    // all ms
    u32 prevOSTime;
    u32 prevDolTime;
    u32 currOSTime;
    u32 currDolTime;
    u32 nextOSTime;
    
    volatile bool waiting;
    simplelock_t lock; // we really dont need this lock

    u32 currOSTimeSafe; // not guarded by lock
    u32 currDolTimeSafe; // not guarded by lock
    f32 conversionFactor; // not guarded by lock
    bool isConversionAccurate; // not guarded by lock

} dolphinTimeConversions = {0};

static long dolphinDeviceInit() {
    int ret = IOS_Open(fsDolphinDevice, 0);
    if(ret < 0) return ret;
    fdDolphinDevice = ret;
    dolphinGetTimeInfo.v[0].base = (u8*)&dolphinGetTimeInfo.timeMs;
    dolphinGetTimeInfo.v[0].length = sizeof dolphinGetTimeInfo.timeMs;
    return 0;
}

static u32 dolphinGetTime() {
    if(fdDolphinDevice < 0) return 0xFFFFFFFF;
    IOS_Ioctlv(fdDolphinDevice, IOCTL_DOLPHIN_GET_SYSTEM_TIME, 0, 1, dolphinGetTimeInfo.v);
    return dolphinGetTimeInfo.timeMs;
}

static long dolphinGetTimeAsync(IOSError (*cb)(IOSError, void *), void *data) {
    if(fdDolphinDevice < 0);
    return IOS_IoctlvAsync(fdDolphinDevice, IOCTL_DOLPHIN_GET_SYSTEM_TIME, 0, 1, dolphinGetTimeInfo.v, cb, data);
}

static IOSError dolphinResyncCb(IOSError err, void *) {
/*
    // if this is not called from interrupt handler, needs to be a semaphore instead (must increment by one more when lock is not obtained) -- but this is fine bc interrupt handler should not be interrupted by other mutual thread
    if(simplelock_tryLockLoop(&dolphinTimeConversions.lock) != TRY_LOCK_RESULT_OK && !dolphinTimeConversions.waiting) {
        return; // this should not happen
    } // if we don't lock but are waiting, that technically needs a semaphore, but this is called from an interrupt handler, so does not matter
*/
    //if(err) setDebugMsg(2);
    
    dolphinTimeConversions.prevOSTime = dolphinTimeConversions.currOSTime;
    dolphinTimeConversions.prevDolTime = dolphinTimeConversions.currDolTime;
    dolphinTimeConversions.currOSTime = dolphinTimeConversions.nextOSTime;
    dolphinTimeConversions.currDolTime = dolphinGetTimeInfo.timeMs;
    
    dolphinTimeConversions.waiting = false;
    //simplelock_release(&dolphinTimeConversions.lock);
    dolphinTimeConversions.isConversionAccurate = false;

    return 0;
}

static void dolphinResync() {
    //if(dolphinTimeConversions.waiting || simplelock_tryLockLoop(&dolphinTimeConversions.lock) != TRY_LOCK_RESULT_OK) return;
    if(dolphinTimeConversions.waiting) return;
    //if(dolphinTimeConversions.waiting) goto fail;
    
    dolphinTimeConversions.nextOSTime = OSTicksToMilliseconds(OSGetTime());
    dolphinTimeConversions.waiting = true;
    //simplelock_release(&dolphinTimeConversions.lock);
    dolphinGetTimeAsync(dolphinResyncCb, nullptr);
/*    return;
fail:
    simplelock_release(&dolphinTimeConversions.lock);*/
}

static bool calcConversion() {
    if(dolphinTimeConversions.waiting) return false;
    if(!dolphinTimeConversions.prevOSTime) return false; // this check intends to make sure all variables used are initialized
    //if(simplelock_tryLockLoop(&dolphinTimeConversions.lock) != TRY_LOCK_RESULT_OK) return false;
    //simplelock_release(&dolphinTimeConversions.lock);
    
    f32 osDelta = dolphinTimeConversions.currOSTime - dolphinTimeConversions.prevOSTime;
    f32 dolDelta = dolphinTimeConversions.currDolTime - dolphinTimeConversions.prevDolTime;
    
    // We could check dolphinTimeConversions.waiting again if we wanted to allow
    // for accesses before the callback

    dolphinTimeConversions.conversionFactor = dolDelta / osDelta;
    dolphinTimeConversions.currDolTimeSafe = dolphinTimeConversions.currDolTime - beaconTimebase.baseDolTimeMs;
    dolphinTimeConversions.currOSTimeSafe = dolphinTimeConversions.currOSTime - beaconTimebase.baseOSTimeMs;
    dolphinTimeConversions.isConversionAccurate = true;
    //setPtrDebugMsg(12, (void*)dolphinTimeConversions.currDolTimeSafe);
    return true;
}
#endif

static void initConversions() {
    beaconTimebase.baseOSTimeMs = OSTicksToMilliseconds(OSGetTime());
#ifdef DOLPHIN
    beaconTimebase.baseDolTimeMs = dolphinGetTime();
    dolphinTimeConversions.conversionFactor = 1.0f;
    dolphinTimeConversions.currDolTime = beaconTimebase.baseDolTimeMs;
    dolphinTimeConversions.currOSTime = beaconTimebase.baseOSTimeMs;
    dolphinTimeConversions.currDolTimeSafe = 0;
    dolphinTimeConversions.currOSTimeSafe = 0;

#endif
}
   
// In the dolphin version, we need to also periodically update the average clockspeed
#ifdef DOLPHIN 
const static u16 MAX_UPDATE_INTERVAL_FRAMES = 20; // Remember to match size with timeSinceUpdate
#else
const static u16 MAX_UPDATE_INTERVAL_FRAMES = 20 * 60 * 60; // Remember to match size with timeSinceUpdate
#endif

void Beacon::init1() {
    init = false;
    offsetClientToServerMs = 0;
    commDelayMs = 0;
    timeSinceUpdateFrames = 1;
#ifdef DOLPHIN
    dolphinDeviceInit();
#endif
    initConversions();
}

const static u32 TIMEOUT_FRAMES = 2 * 60; // 500ms is better, maybe go even lower if possible

static struct {
    u32 startTime;
    u32 queryTimer; // lock exempt (only access from Beacon::_update)
    enum {
        INIT = 0,
        RESPONSE
    } state;

   /* THIS TYPE INSTANTIATES A TEMPLATE WITH VERY SPECIFIC REQUIREMENTS.
      PLEASE REFER TO `Beacon::process` FOR MORE DETAILS BEFORE MODIFYING */
    Packets::TimeQuery::ReliablePacketCode check;    
    
    simplelock_t lock; // This lock guards _beaconInitData **AND** /dev/dolphin before beacon is initialized
} _beaconInitData = {0};

namespace implementation {

class _Beacon {
public:
    static IOSError process2Callback(IOSError, void *data);

    static void process2(Beacon &self);
};

}

static IOSError _update2Callback(IOSError, void *data) {


    Transmission::Transmitter<Packets::PacketProcessor> &transmitter =
        *(Transmission::Transmitter<Packets::PacketProcessor> *)data;
    
    _beaconInitData.startTime = 
#ifdef DOLPHIN
        dolphinGetTimeInfo.timeMs - beaconTimebase.baseDolTimeMs; // could improve by setting start time in transmitter/writer
#else
        OSTicksToMilliseconds(OSTime()) - beaconTimebase.baseOSTimeMs;
#endif
    
    Packets::TimeQuery tqp;
    tqp.init(_beaconInitData.startTime); // CANNOT BLOCK
    _beaconInitData.check = tqp.check;

    transmitter.addPacket(tqp); // CANNOT BLOCK

    _beaconInitData.queryTimer = TIMEOUT_FRAMES;
    _beaconInitData.state = _beaconInitData.RESPONSE;

    simplelock_release(&_beaconInitData.lock);
    return 0;
}

static void _update2(Transmission::Transmitter<Packets::PacketProcessor> &transmitter) {
#ifdef DOLPHIN
    dolphinGetTimeAsync(_update2Callback, &transmitter);
#else
    _update2Callback(0, &transmitter);
#endif
}

void Beacon::_update(Transmission::Transmitter<Packets::PacketProcessor> &transmitter) {
    if(init) {
        if(timeSinceUpdateFrames <= 0) {
            timeSinceUpdateFrames = MAX_UPDATE_INTERVAL_FRAMES;
            //setDebugMsg(12, 1);
#ifdef DOLPHIN
            dolphinResync();
#endif
        }
    }
    else if(_beaconInitData.state == _beaconInitData.INIT || _beaconInitData.queryTimer == 0) {
        // obtain a lock for init, startTime, state, and check; then check init
        if(simplelock_tryLockLoop(&_beaconInitData.lock) != TRY_LOCK_RESULT_OK) return;
       
        if(!init) {
            _update2(transmitter); // this function releases the lock
            return; 
        }

        simplelock_release(&_beaconInitData.lock);
    }
    else _beaconInitData.queryTimer--;
}

IOSError implementation::_Beacon::process2Callback(IOSError, void *data) {

    Beacon &self = *(Beacon *)data;
    
    u32 time = 
#ifdef DOLPHIN
        dolphinGetTimeInfo.timeMs - beaconTimebase.baseDolTimeMs;
#else
        OSTicksToMilliseconds(OSGetTime()) - beaconTimebase.baseOSTimeMs; // Does not need thread
#endif

    self.commDelayMs = time - _beaconInitData.startTime;

    self.timeSinceUpdateFrames = 1;
    self.init = true;
    simplelock_release(&_beaconInitData.lock);
    return 0;
}

void implementation::_Beacon::process2(Beacon &self) {
#ifdef DOLPHIN
    dolphinGetTimeAsync(_Beacon::process2Callback, &self);
#else
    process2Callback(0, &self);
#endif
}

// BE CAREFUL: THIS GETS CALLED FROM AN UNTHREADED CONTEXT
// EXTRA CAUTION: TEMPLATE FUNCTION `trp.verify` MUST NOT REQUIRE THREAD
// PLEASE MAKE SURE THAT `trp.verify` IS INSTANTIATED WITH PARAMETERS
// THAT FULFILL THIS REQUIREMENT BEFORE CHANGING THIS FUNCTION
void Beacon::process(const Packets::TimeResponse &trp) {
    //setDebugMsg(12, 1);
    if(init) return;
    if(simplelock_tryLockLoop(&_beaconInitData.lock) != TRY_LOCK_RESULT_OK) return;
    if(init 
        || _beaconInitData.state == _beaconInitData.INIT 
        || !trp.verify(_beaconInitData.check)
    ) {
        simplelock_release(&_beaconInitData.lock);
        return;
    }
    //setDebugMsg(13, 1);
    
    offsetClientToServerMs = trp.timeMs - _beaconInitData.startTime;

    implementation::_Beacon::process2(*this);
}

LocalTimestamp Beacon::now() {
#ifdef DOLPHIN
    LocalTimestamp ret = {0xFFFFFFFF};
    if(!dolphinTimeConversions.isConversionAccurate) calcConversion();
    
    ret.t.timeMs = 
        (u32)(
            OSTicksToMilliseconds(OSGetTime()) 
            - dolphinTimeConversions.currOSTimeSafe
        ) 
        * dolphinTimeConversions.conversionFactor 
    + dolphinTimeConversions.currDolTimeSafe;

    return ret;
#else
    LocalTimestamp t = {OSTicksToMilliseconds(OSGetTime()) - beaconTimebase.baseOSTimeMs};
    return t;
#endif
}

};

