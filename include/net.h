#ifndef NET_H
#define NET_H

#ifdef __cplusplus
extern "C" {
#endif

#include <revolution/ipc/ipcclt.h>
typedef unsigned long size_t;

struct sockaddr_in {
    unsigned char addrlen;
    unsigned char addrfamily;
    short port;
    long addr;
};

struct pollsd {
    long sd;
    unsigned long events;
    unsigned long revents;
};

#define POLLRDNORM 1
#define POLLRDBAND 2
#define POLLIN (POLLRDNORM | POLLRDBAND)

long netinit(void);

long netsocket(long domain, long type, long protocol);

long netbind(long fd, const struct sockaddr_in *addr);
long netconnect(long fd, const struct sockaddr_in *addr);

// Please align `data` to 32 bytes
long netsendto(long fd, const void *data, size_t len, const struct sockaddr_in *);
long netsendto_async(long fd, const void *data, size_t len, const struct sockaddr_in *, IOSIpcCb cb, void *cb_data);

// Please align `data` to 32 bytes
long netread(long fd, void *data, size_t len);
long netread_async(long fd, void *data, size_t len, IOSIpcCb cb, void *cb_data);

// `timeout` in ms
long netpoll_async(struct pollsd *sd, unsigned long nsd, long timeout, IOSIpcCb cb, void *cb_data);

//long netshutdown(void);

#ifdef __cplusplus
}
#endif

#endif
