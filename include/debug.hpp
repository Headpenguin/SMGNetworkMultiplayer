#ifndef DEBUG_HPP
#define DEBUG_HPP

#include <revolution/types.h>
#include "net.h"

void setDebugMsg(u32 i, u8 value);
void setPtrDebugMsg(u32 i, void* value);
void setDebugMsgFloat(u32 i, float value);
void setDebugCounter(u32 i);
void setupDebug(int sock, const sockaddr_in *addr);
void sendDebugMsg();


#endif
