
#include "net.h"
#include "revolution/ipc/ipcclt.h"
#include "cstring"

static const char *msg = "plppppppp";

void testNet(void) {
    long fd;
    struct sockaddr_in server_addr = {
        8,
        2,
        5000,
        0x0A000060
    };
    char buff[10] __attribute__((aligned(32)));

    memcpy(buff, msg, 10);

    netinit();
    fd = netsocket(2, 2, 0);
//    netconnect(fd, &server_addr);
    netsendto(fd, buff, 10, server_addr);
}

