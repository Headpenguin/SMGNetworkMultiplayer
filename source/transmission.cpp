#include "transmission.hpp"

namespace Transmission {

void Reader::setStartTime() {
    periodEnd = OSGetTime() + OSMillisecondsToTicks(period_ms);
}

NetReturn Reader::process(IOSError err, u8 *&buffer) {
    if(err < 0 && state != RESET) {
        state = POLL;
        return NetReturn::SystemError(-err);
    }

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
            buffPosition += err;
            buffer = readBuff;

            state = POLL;
            return NetReturn::Ok(err);
        }

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
    u8 *ret = alignUp32(sp);
    u8 *tmpSp = ret + len;
    if(tmpSp + sizeof *sp >= buffEnd) {
        simplelock_release(&lock);
        return NetReturn::NotEnoughSpace(tmpSp + sizeof *sp - buffEnd);
    }
    
    *(u8 **)tmpSp = sp;
    buffer = tmpSp;
    simplelock_release(&lock);
    return NetReturn::Ok();
}

NetReturn Writer::process(IOSError err) {
    if(err < 0) return NetReturn::SystemError(err);
    return NetReturn::Ok();
}

bool Writer::conclude() {
    if(sp == buff) {
        simplelock_release(&lock);
        return true;
    }
    u8 *tmpSp = *(u8 **)sp;
    u8 *packetBuff = alignUp32(tmpSp);
    u32 len = sp - packetBuff;
    netsendto_async(sd, packetBuff, len, addr, cb, cbData);
    sp = tmpSp;
    return false;
}

}

