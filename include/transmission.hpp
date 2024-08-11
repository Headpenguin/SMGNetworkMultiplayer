#ifndef TRANSMISSION_HPP
#define TRANSMISSION_HPP

#include "netCommon.hpp"
#include "packets.hpp"
#include "net.h"
#include <revolution/os.h>
#include "atomic.h"
#include "debug.hpp"

//extern u8 test[8];
namespace Transmission {

enum ConcludeResult {
    CONTINUE,
    COMPLETE,
    FAILURE
};

class Reader {
    u8 period_ms;
    OSTime periodEnd;
    
    u8 *readBuff;
    u8 *buffPosition;
    u32 buffSize;

    IOSIpcCb cb;
    void *cbData;

    enum State {
        READ = 0,
        POLL,
        RESET
    };
    State state;

    pollsd psd;
    bool psdReady;

public:
    inline Reader() {}
    // Align buffer to 32 bytes
    inline Reader(u8 period_ms, void *buffer, u32 bufferSize, int sockfd)
        : period_ms(period_ms),
        periodEnd(0),
        readBuff((u8 *)buffer),
        buffPosition(readBuff),
        buffSize(bufferSize),
        cb(nullptr),
        cbData(nullptr),
        state(READ),
        psdReady(false)
    {
        psd.sd = sockfd;
        psd.events = POLLIN;
        psd.revents = 0;    
    }

    inline void setCallback(IOSIpcCb _cb, void *data) {
        cb = _cb;
        cbData = data;
    }

    void setStartTime();
    NetReturn process(IOSError err, u8 *&buffer);
    inline void informError(NetReturn err) {}
    // Returns true to indicate that the timer has expired
    ConcludeResult conclude();
    inline void reset() {state = RESET;}
};
    
class Writer {

    u8 *buff;
    u8 *buffEnd;
    u8 *sp;

    IOSIpcCb cb;
    void *cbData;

    const sockaddr_in *addr;
    int sd;

    simplelock_t lock;

public:
    inline Writer() {}
    inline Writer(u8 *buffer, u32 bufferSize, int sd, const sockaddr_in *addr)
        : buff(buffer),
        buffEnd(buff + bufferSize),
        sp(buff - sizeof sp),
        cb(nullptr),
        cbData(nullptr),
        addr(addr),
        sd(sd),
        lock(0) {}

    inline void setCallback(IOSIpcCb _cb, void *data) {
        cb = _cb;
        cbData = data;
    }

    inline bool reset() {return simplelock_tryLock(&lock) == TRY_LOCK_RESULT_OK;}
    NetReturn createTransmissionBuffer(u8 *&buffer, u32 len);
    NetReturn process(IOSError err);
    ConcludeResult conclude();
};

template<typename T>
class Transmitter {
    Reader reader;
    Writer writer;

    enum State {
        WRITE = 0,
        READ,
        CONCLUDE
    };

    T packetProcessor;

    State state;
    bool requestedAgain;

    static IOSError callback(IOSError err, void *data) {
        Transmitter<T> *transmitter = (Transmitter<T> *)data;
        transmitter->process(err);
        return 0;
    }

    inline void setConclude() volatile {
        state = CONCLUDE;
    }
    inline void resetRequestedAgain() volatile {
        requestedAgain = false;
    }

    void process(IOSError err) {
//        test[4] = 1;
        setDebugMsg(4, state);
        switch(state) {
            case WRITE:
            {
//                test[5] = 1;
                writer.process(err);
                ConcludeResult cres = writer.conclude();
                if(cres == CONTINUE) break;
                else if(cres == COMPLETE) {
                    state = READ;
                    err = 0;
                    reader.reset();
                }
                else if(cres == FAILURE) {
                    setDebugMsg(5, 1);
                    setConclude();
                    resetRequestedAgain();
                }
                //proceed to the next state (no break is intentional)
            }
            case READ:
            {
//                test[5] = 2;
                u8 *transmissionBuffer = nullptr;
                NetReturn res = reader.process(err, transmissionBuffer);
                Packets::Tag tag;
                if(transmissionBuffer && res.bytes >= sizeof tag) {
                    tag = *(Packets::Tag *)transmissionBuffer;
                    res = packetProcessor.process(tag, transmissionBuffer + sizeof tag, res.bytes - sizeof tag);
                    reader.informError(res);
                }
                ConcludeResult cres = reader.conclude();
                if(cres == CONTINUE) break;
                else if(cres == COMPLETE) {
                    state = CONCLUDE;
                }
                else if(cres == FAILURE) {
                    setDebugMsg(5, 1);
                    setConclude();
                    resetRequestedAgain();
                }
                //proceed to the the next state (no break is intentional)
            }
            case CONCLUDE:
//                test[5] = 3;
                if(requestedAgain) update();
                break;
        }
    }

public:
    Transmitter() {}
    Transmitter(const Reader &_reader, const Writer &_writer, T packetProcessor) 
        : reader(_reader), 
        writer(_writer),
        packetProcessor(packetProcessor),
        state(CONCLUDE) {}

    void init() {
        reader.setCallback(callback, (void*)this);
        writer.setCallback(callback, (void*)this);
    }

    template<typename U>
    NetReturn addPacket(const Packets::Packet<U> &packet) {
        u8 *buffer;
        Packets::Tag tag = packet.getTag();
        u32 size = packet.getSize();
        NetReturn res = writer.createTransmissionBuffer(buffer, size + sizeof(tag));
        if(res.err != NetReturn::OK) return res;
        
        *(Packets::Tag*)buffer = tag;
        buffer += sizeof(tag);

        return packet.netWriteToBuffer(buffer, size);
    }

    bool update() {
        if(state == CONCLUDE) {
            requestedAgain = false;
            reader.setStartTime();
            setDebugMsg(1, 1);
            if(!writer.reset()) return false;
            state = WRITE;
            process(0);
            setDebugMsg(1, 0);
            return true;
        }
        else {
            requestedAgain = true;
            setDebugMsg(1, 2);
            return false;
        }
    }

};

}

#endif
