#include "transmission.hpp"

namespace Transmission {

void Reader::setStartTime() {
    periodEnd = OSGetTime() + OSMillisecondsToTicks(period_ms);
}

NetReturn Reader::process(IOSError err, u8 *&buffer) {
    switch(state) {

        case RESET:
            buffPosition = readBuff;
            state = POLL;
            break;

        case POLL:
        {
            psdReady = err >= 1 && (psd.revents & POLLIN);

            state = READ;
            break;
        }
        case READ:
        {
            state = POLL;
            if(err >= 0) {
                buffPosition += err;
                buffer = readBuff;

                return NetReturn::Ok(err);
            }
            break;
        }

    }
    if(err < 0 && state != RESET) {
        return NetReturn::SystemError(-err);
    }

    return NetReturn::Ok();

}

bool Reader::conclude() {
    switch(state) {
        case POLL:
        {
            s32 timeout = OSTicksToMilliseconds(periodEnd - OSGetTime()); // Round down
            if(timeout < 0) timeout = 0;

            psdReady = false;
            netpoll_async(&psd, 1, timeout, cb, cbData);
            break;
        }
        case READ:
        {
            if(!psdReady) return true;

            buffPosition = readBuff;

            netread_async(psd.sd, readBuff, buffSize, cb, cbData);

            break;
        }
    }

    return false;
}

static u8* alignUp32(u8 *p) {
    return (u8*)((u32)p + 31 & ~31);
}

NetReturn Writer::createTransmissionBuffer(u8 *&buffer, u32 len) {
    if(!simplelock_tryLock(&lock)) return NetReturn::Busy();
    u8 *ret = alignUp32(sp + sizeof sp);
    u8 *tmpSp = ret + len;
    if(tmpSp + sizeof sp >= buffEnd) {
        simplelock_release(&lock);
        return NetReturn::NotEnoughSpace(tmpSp + sizeof sp - buffEnd);
    }
    
    *(u8 **)tmpSp = sp;
    buffer = ret;
    sp = tmpSp;
    simplelock_release(&lock);
    return NetReturn::Ok();
}

NetReturn Writer::process(IOSError err) {
    if(err < 0) return NetReturn::SystemError(-err);
    return NetReturn::Ok();
}


bool Writer::conclude() {
    if(sp + sizeof(sp) == buff) {
        simplelock_release(&lock);
     //   test[4] = 1;
        return true;
    }
    u8 *tmpSp = *(u8 **)sp;
    u8 *packetBuff = alignUp32(tmpSp + sizeof(sp));
    u32 len = sp - packetBuff;
    long res = netsendto_async(sd, packetBuff, len, addr, cb, cbData);
    //test[3] = res < 0 ? -res : 0;
    sp = tmpSp;
    return false;
}

}

