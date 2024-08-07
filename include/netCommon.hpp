#ifndef NETCOMMON_HPP
#define NETCOMMON_HPP

#include <revolution/types.h>

struct NetReturn {
    enum ErrorCode {
        OK = 0,
        NOT_ENOUGH_SPACE,
        INVALID_DATA,
        SYSTEM_ERROR,
        BUSY
    };
    // How much we have read/written
    unsigned bytes: 24;
    unsigned err: 8;

    NetReturn(u32 bytes, u8 err) : bytes(bytes), err(err) {}
    
    inline static NetReturn Ok(u32 bytes) {return NetReturn(bytes, NetReturn::OK);}
    inline static NetReturn Ok() {return NetReturn(0, NetReturn::OK);}
    inline static NetReturn NotEnoughSpace(u32 bytes) {return NetReturn(bytes, NetReturn::NOT_ENOUGH_SPACE);}
    inline static NetReturn InvalidData() {return NetReturn(0, NetReturn::INVALID_DATA);}
    inline static NetReturn SystemError(u32 err) {return NetReturn(err, NetReturn::SYSTEM_ERROR);}
    inline static NetReturn Busy() {return NetReturn(0, NetReturn::BUSY);}
};

#endif
