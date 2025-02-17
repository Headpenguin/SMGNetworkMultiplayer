#include "uiIpFsTool.hpp"
#include <revolution/types.h>
#include <revolution/dvd.h>
#include "net.h"

static bool _isDigit(char c) {
    return '0' <= c && c <= '9';
}

static long _decode(u8 *buff, u32 len, sockaddr_in *outAddr) {
    union {
        u32 addr;
        long ret;
    };
    addr = 0;
    u32 i = 0;
    for(; i < len; i++) {
        if(_isDigit(buff[i])) break;
    }
    for(u32 j = 0; j < 4; j++) { // for each octet
        u16 octet = 0;
        for(u32 k = 0; k < 3; k++) {
            if(i >= len || !_isDigit(buff[i])) {
                if(k > 0) break;
                else return false;
            }
            u8 digit = buff[i] - '0';
            octet *= 10;
            octet += digit;
            i++;
        }

        if (
            j <= 2 && (i >= len || buff[i] != '.')
            || j == 3 && i < len && _isDigit(buff[i])
            || octet > 0xFF
        ) return false;
        if(j <= 2) i++;
        
        addr <<= 8;
        addr += octet;
    }

    u32 port = 0xFFFFFFFF;
    if(i < len && buff[i] == ':') {
        port = 0;
        i++;
        for(u32 j = 0; j < 5; j++) {
            if(i >= len || !_isDigit(buff[i])) {
                if(j == 0) return false;
                else break;
            }
            u8 digit = buff[i] - '0';
            port *= 10;
            port += digit;
            i++;
        }
        if(i < len && _isDigit(buff[i]) || port > 0xFFFF) return false;
    }
    
    outAddr->addr = addr;
    if(port != 0xFFFFFFFF) outAddr->port = port;
    return true;
}

bool readIpAddrFs(const char *path, sockaddr_in *addr) {
    if(!addr) return false;

    int pathId = DVDConvertPathToEntrynum(path);

    if(pathId < 0) return false;

    DVDFileInfo fileInfo;
    if(!DVDFastOpen(pathId, &fileInfo)) return false;

    u8 buff[40] __attribute__((aligned(32)));

    s32 len = sizeof buff;
    if(fileInfo.length < len) len = fileInfo.length;
    len = DVDReadPrio(&fileInfo, buff, len, 0, 2); // uses same prio as loader, maybe it shouldn't?
    DVDClose(&fileInfo);
    if(len < 0) return false;
    return _decode(buff, len, addr);
}
