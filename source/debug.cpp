#include "debug.hpp"

static u8 debugMsg[32] __attribute__((aligned(32)));

void setDebugMsg(u32 i, u8 value) {
    if(i < 32) debugMsg[i] = value;
}
void setPtrDebugMsg(u32 i, void* value) {
    if(i < 32) *(void**)(debugMsg + i) = value;
}
void setDebugMsgFloat(u32 i, float value) {
    if(i < 32) *(float*)(debugMsg + i) = value;
}

static int sock;
const static sockaddr_in *addr;

void setupDebug(int _sock, const sockaddr_in *_addr) {
    sock = _sock;
    addr = _addr;
}

void setDebugCounter(u32 i) {
    if(i < 32) debugMsg[i]++;
}

void sendDebugMsg() {
    netsendto(sock, debugMsg, 32, addr);
}

