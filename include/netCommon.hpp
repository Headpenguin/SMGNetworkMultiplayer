#ifndef NETCOMMON_HPP
#define NETCOMMON_HPP

enum ErrorCode {
    OK = 0,
    NOT_ENOUGH_SPACE,
    INVALID_DATA
};

struct NetReturn {
    enum ErrorCode {
        OK = 0,
        NOT_ENOUGH_SPACE,
        INVALID_DATA
    };
    // How much we have read/written
    unsigned bytes: 24;
    unsigned err: 8;

    NetReturn(u32 bytes, u8 err) : bytes(bytes), err(err) {}
    
    inline static NetReturn Ok(u32 bytes) {return NetReturn(bytes, NetReturn::OK);}
    inline static NetReturn NotEnoughSpace(u32 bytes) {return NetReturn(bytes, NetReturn::NOT_ENOUGH_SPACE);}
    inline static NetReturn InvalidData() {return NetReturn(0, NetReturn::INVALID_DATA);}
};

#endif
