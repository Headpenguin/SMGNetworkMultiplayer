#include "uiIpFsTool.hpp"
#include <revolution/types.h>

static bool _isDigit(char c) {
    return '0' <= c && c <= '9';
}

static long _decode(u8 *buff, u32 len) {
    union {
        u32 addr;
        long ret;
    };
    u32 i = 0;
    for(; i < len; i++) {
        if(_isDigit(buff[i])) break;
    }
    for(u32 j = 0; j < 4; j++) { // for each octet
        u16 octet = 0;
        for(u32 k = 0; k < 3; k++) {
            if(i >= len || !_isDigit(buff[i])) return -1;
            u8 digit = buff[i] - '0';
            octet *= 10;
            octet += digit;
            i++;
        }

        if (
            j <= 3 && (i >= len || buff[i] != '.') 
            || octet > 0xFF
        ) return -1;
        
        addr *= 8;
        addr += octet;
    }

    return ret;
}

long readIpAddrFs(const char *path) {
    int pathId = DVDConvertPathToEntryNum(path);

    if(pathId < 0) return -1;

    DVDFileInfo fileInfo;
    if(!DVDFastOpen(pathId, &fileInfo)) return -1;

    u8 buff[40] __attribute__((aligned(32)));
    s32 len = DVDReadPrio(fileInfo, buff, sizeof buff, 0, 2); // uses same prio as loader, maybe it shouldn't?
    if(len < 0) return -1;
    return _decode(buff, len);
}
