#include "beacon.hpp"

namespace Timestamps {

const static u32 TIMEOUT_FRAMES = 2 * 60; // 500ms is better, maybe go even lower if possible

struct {
    u32 startTime;
    u32 queryTimer; // lock exempt (only access from Beacon::_update)
    enum {
        INIT = 0,
        RESPONSE
    } state;
    Packets::TimeQuery::ReliablePacketCode check; // THIS TYPE INSTANTIATES A TEMPLATE
                                               // WITH VERY SPECIFIC REQUIREMENTS.
                                               // PLEASE REFER TO `Beacon::process`
                                               // FOR MORE DETAILS BEFORE MODIFYING
    simplelock_t lock;
} _beaconInitData = {0};

void Beacon::_update(Transmission::Transmitter<Packets::PacketProcessor> &transmitter) {
    if(init) {
        if(timeSinceUpdateFrames > MAX_UPDATE_INTERVAL_FRAMES) {
            timeSinceUpdateFrames = 0; // Resync time with server (unimplemented)
        }
    }
    else if(_beaconInitData.state == _beaconInitData.INIT || _beaconInitData.queryTimer == 0) {
        // obtain a lock for init, startTime, state, and check; then check init
        if(simplelock_tryLockLoop(&_beaconInitData.lock) != TRY_LOCK_RESULT_OK) return;
        
        if(!init) {
            _beaconInitData.startTime = OSTicksToMilliseconds(OSGetTime()); // could improve by setting start time in transmitter/writer

            Packets::TimeQuery tqp;
            tqp.init(_beaconInitData.startTime);
            _beaconInitData.check = tqp.check;

            transmitter.addPacket(tqp);

            _beaconInitData.queryTimer = TIMEOUT_FRAMES;
            _beaconInitData.state = _beaconInitData.RESPONSE;
        }

        simplelock_release(&_beaconInitData.lock);
    }
    else _beaconInitData.queryTimer--;
}

// BE CAREFUL: THIS GETS CALLED FROM AN UNTHREADED CONTEXT
// EXTRA CAUTION: TEMPLATE FUNCTION `trp.verify` MUST NOT REQUIRE THREAD
// PLEASE MAKE SURE THAT `trp.verify` IS INSTANTIATED WITH PARAMETERS
// THAT FULFILL THIS REQUIREMENT BEFORE CHANGING THIS FUNCTION
void Beacon::process(const Packets::TimeResponse &trp) {
    if(simplelock_tryLockLoop(&_beaconInitData.lock) != TRY_LOCK_RESULT_OK) return;
    if(init || _beaconInitData.state == _beaconInitData.INIT) goto end;
    if(!trp.verify(_beaconInitData.check)) { // 
        //_beaconInitData.state = _beaconInitData.INIT;
        goto end;
    }
    
    u32 time = OSTicksToMilliseconds(OSGetTime()); // Does not need thread
    commDelayMs = time - _beaconInitData.startTime;
    offsetClientToServerMs = trp.timeMs - _beaconInitData.startTime;
    timeSinceUpdateFrames = 0;

    *(volatile bool *)(&init) = true;

end:
    simplelock_release(&_beaconInitData.lock);
}

};

