typedef unsigned long size_t;

#include "net.h"
#include "errno.h"
#include "atomic.h"

extern void setDebugMsg__FUlUc(unsigned long, unsigned char);

#include <revolution/os/OSCache.h>
#include <revolution/os.h>
#include <cstring>
#include <mem.h>

static const char *kd_fs = "/dev/net/kd/request";
static const char *iptop_fs = "/dev/net/ip/top";

static long iptop_fd = -1;
static long ipaddr = 0;
static long attempt = 0;

#define MAX_IP_ADDR_RETRY 30
#define IP_ADDR_RETRY_PERIOD_U 1000000 

#define IOCTL_NWC24iStartupSocket 0x06

#define IOCTL_SOConnect 0x04
#define IOCTL_SOPoll 0xB
#define IOCTL_SORecvFrom 0x0C
#define IOCTL_SOSendTo 0x0D
#define IOCTL_SOSocket 0x0F
#define IOCTL_SOGetHostID 0x10
#define IOCTL_SOStartup 0x1F
//#define IOCTL_SOShutown 0x0E

long netinit() {
    long kd_fd = -1, err = 0, i = 0;
    unsigned char kd_out[32] __attribute__((aligned(32))); //unused

    // NWC24 socket initialization

    kd_fd = IOS_Open(kd_fs, 0);

    if(kd_fd < 0) return kd_fd;

    err = IOS_Ioctl(kd_fd, IOCTL_NWC24iStartupSocket, NULL, 0, kd_out, 32);

    if(err < 0) {
        IOS_Close(kd_fd);
        kd_fd = -1;
        return err;
    }

    IOS_Close(kd_fd);
    kd_fd = -1;

    // SOStartup

    iptop_fd = IOS_Open(iptop_fs, 0);

    if(iptop_fd < 0) return iptop_fd;

    err = IOS_Ioctl(iptop_fd, IOCTL_SOStartup, NULL, 0, NULL, 0);

    if(err < 0) {
        IOS_Close(iptop_fd);
        iptop_fd = -1;
        return err;
    }

    // IP
    err = 0;

    for(i = 0; i < MAX_IP_ADDR_RETRY && err == 0; i++) {

        err = IOS_Ioctl(iptop_fd, IOCTL_SOGetHostID, NULL, 0, NULL, 0);
        
        if(err == 0) OSSleepTicks(OSMicrosecondsToTicks(IP_ADDR_RETRY_PERIOD_U));

    }

    if(err == 0) {
        IOS_Close(iptop_fd);
        iptop_fd = -1;
        return -ETIMEDOUT;
    }

    ipaddr = err;
    
    // Done

    return 0;

}

/*long netshutdown(void) {
    long err = 0, ret = 0;

    if(iptop_fd >= 0) {
        err = IOS_Ioctl(iptop_fd, IOCTL_SOShutdown, NULL, 0, NULL, 0);
        ret = IOS_Close(iptop_fd);
        if(err < 0) ret = err;
    }

    return ret;
}*/

long netsocket(long domain, long type, long protocol) {
    long buff[3] __attribute__((aligned(32)));

    if(iptop_fd < 0) return -ENXIO;

    buff[0] = domain;
    buff[1] = type;
    buff[2] = protocol;

    return IOS_Ioctl(iptop_fd, IOCTL_SOSocket, buff, 12, NULL, 0);
}

struct connect_data {
    long fd;
    unsigned long has_addr;
    unsigned char addr[28];
};

long netconnect(long fd, const struct sockaddr_in *addr) {
    struct connect_data buff __attribute__((aligned(32)));

    if(iptop_fd < 0) return -ENXIO;
    
    memset(&buff, 0, sizeof(buff));

    buff.fd = fd;
    buff.has_addr = 1;

    memcpy(&buff.addr, addr, sizeof(*addr));
    
    return IOS_Ioctl(iptop_fd, IOCTL_SOConnect, &buff, sizeof(buff), NULL, 0);
}

struct sendto_data {
    long fd;
    unsigned long flags;
    unsigned long has_addr;
    unsigned char addr[28];
};

static long netsendto_prep(long fd, const void *data, size_t len, const struct sockaddr_in *addr, IOSIoVector *v, struct sendto_data *params) {
    
    if(iptop_fd < 0) return -ENXIO;

    if((unsigned int)data % 32) return -EINVAL; // not aligned
                        
    memset(params, 0, sizeof *params);
    params->fd = fd;
    params->has_addr = addr ? 1u : 0u;

    if(addr) memcpy(&params->addr, addr, sizeof(*addr));

//    DCFlushRange(data, len);
//    DCFlushRange(&params, sizeof params);

    v[0].base = (u8*)data;
    v[0].length = len;
    v[1].base = (u8*)params;
    v[1].length = sizeof *params;

    return 0;
}

long netsendto(long fd, const void *data, size_t len, const struct sockaddr_in *addr) {
    struct sendto_data params __attribute__((aligned(32)));
    IOSIoVector v[2] __attribute__((aligned(32)));
    long err;
    
    err = netsendto_prep(fd, data, len, addr, v, &params);
    if(err < 0) return err;

    return IOS_Ioctlv(iptop_fd, IOCTL_SOSendTo, 2, 0, v);
}

static struct sendto_data netsendto_async_params __attribute__((aligned(32)));
static IOSIoVector netsendto_async_v[2] __attribute__((aligned(32)));
static struct {
    IOSIpcCb cb;
    void *data;
} netsendto_async_cbInfo;
// Perhaps add in a timeout failsafe in case callback is not called?
static simplelock_t netsendto_async_buflock;

static IOSError netsendto_async_cb(IOSError err, void *) {

    IOSIpcCb cb = netsendto_async_cbInfo.cb;
    void *data = netsendto_async_cbInfo.data;
   
    simplelock_release(&netsendto_async_buflock);
    return cb ? cb(err, data) : 0;
}

// All async callbacks execute in the context of the NULL thread. Functions that assume a non-null thread (for example, anything that blocks) MUST NEVER be called during a callback
long netsendto_async(long fd, const void *data, size_t len, const struct sockaddr_in *addr, IOSIpcCb cb, void *cb_data) {
    long err;
    
    switch(simplelock_tryLock(&netsendto_async_buflock)) {
        case TRY_LOCK_RESULT_OK:
            break;
        case TRY_LOCK_RESULT_FAIL:
            //Failed to acquire lock
            return -EWOULDBLOCK;
        case TRY_LOCK_RESULT_INT:
            return -EINTR;
    }

    err = netsendto_prep(fd, data, len, addr, netsendto_async_v, &netsendto_async_params);
    if(err < 0) return err;

    netsendto_async_cbInfo.cb = cb;
    netsendto_async_cbInfo.data = cb_data;

    return IOS_IoctlvAsync(iptop_fd, IOCTL_SOSendTo, 2, 0, netsendto_async_v, netsendto_async_cb, NULL);
}

static long netread_prep(long fd, void *data, size_t len, IOSIoVector *v, unsigned int *params) {
    if(iptop_fd < 0) return -ENXIO;

    if((unsigned int)data % 32) return -EINVAL; // not aligned

    params[0] = fd; // fd
    params[1] = 0; // flags
    
    // params    
    v[0].base = (u8*)params; 
    v[0].length = 8;

    // data
    v[1].base = (u8*)data;
    v[1].length = len;
    
    // sockaddr
    v[2].base = NULL;
    v[2].length = 0;
                                                //
    return 0;

//    DCFlushRange(params, 8);
}

long netread(long fd, void *data, size_t len) {
    unsigned int params[2] __attribute__((aligned(32)));
    IOSIoVector v[3] __attribute__((aligned(32)));
    long err;

    err = netread_prep(fd, data, len, v, params);
    if(err < 0) return err;
                                               
    return IOS_Ioctlv(iptop_fd, IOCTL_SORecvFrom, 1, 2, v);
}

static IOSIoVector netread_async_v[3] __attribute__((aligned(32)));
static unsigned int netread_async_params[2] __attribute__((aligned(32)));
static struct {
    IOSIpcCb cb;
    void *data;
} netread_async_cbInfo;
static simplelock_t netread_async_bufLock;

static IOSError netread_async_cb(IOSError err, void *) {
    simplelock_release(&netread_async_bufLock);
    return netread_async_cbInfo.cb(err, netread_async_cbInfo.data);
}

long netread_async(long fd, void *data, size_t len, IOSIpcCb cb, void *cb_data) {
    long err;

    switch(simplelock_tryLock(&netread_async_bufLock)) {
        case TRY_LOCK_RESULT_OK:
            break;
        case TRY_LOCK_RESULT_FAIL:
            //Failed to acquire lock
            return -EWOULDBLOCK;
        case TRY_LOCK_RESULT_INT:
            return -EINTR;
    }

    err = netread_prep(fd, data, len, netread_async_v, netread_async_params);
    if(err < 0) return err;

    netread_async_cbInfo.cb = cb;
    netread_async_cbInfo.data = cb_data;
                                               
    return IOS_IoctlvAsync(iptop_fd, IOCTL_SORecvFrom, 1, 2, netread_async_v, netread_async_cb, NULL);
}

static long netpoll_prep(struct pollsd *sd, unsigned long nsd, long timeout, long *pParams, struct pollsd *alignedSD) {
    if(iptop_fd < 0) return -ENXIO;
    // no need to implement this yet
    if(nsd != 1 || sd == NULL) return -EINVAL;
    pParams[1] = timeout;
    memcpy(alignedSD, sd, sizeof *alignedSD);
    return 0;
}

static struct pollsd netpoll_async_pollsd __attribute__((aligned(32)));
static long netpoll_async_params[2] __attribute__((aligned(32)));
static struct netpoll_async_cbInfo {
    void *data;
    IOSIpcCb cb;
    struct pollsd *sd;
    unsigned long nsd;
} netpoll_async_cbInfo;
simplelock_t netpoll_async_lock;

static IOSError netpoll_async_cb(IOSError err, void *data_) {
    struct netpoll_async_cbInfo *data = data_;
    simplelock_release(&netpoll_async_lock);
    memcpy(data->sd, &netpoll_async_pollsd, data->nsd * sizeof netpoll_async_pollsd);
    return data->cb ? data->cb(err, data->data) : 0;
}

long netpoll_async(struct pollsd *sd, unsigned long nsd, long timeout, IOSIpcCb cb, void *cb_data) {
    long err;

    switch(simplelock_tryLock(&netpoll_async_lock)) {
        case TRY_LOCK_RESULT_OK:
            break;
        case TRY_LOCK_RESULT_FAIL:
            //Failed to acquire lock
            return -EWOULDBLOCK;
        case TRY_LOCK_RESULT_INT:
            return -EINTR;
    }
    
    err = netpoll_prep(sd, nsd, timeout, netpoll_async_params, &netpoll_async_pollsd);
    if(err < 0) return err;

    netpoll_async_cbInfo.cb = cb;
    netpoll_async_cbInfo.data = cb_data;
    netpoll_async_cbInfo.sd = sd;
    netpoll_async_cbInfo.nsd = nsd;

    return IOS_IoctlAsync(iptop_fd, IOCTL_SOPoll, netpoll_async_params, sizeof netpoll_async_params, &netpoll_async_pollsd, nsd * sizeof netpoll_async_pollsd, netpoll_async_cb, &netpoll_async_cbInfo);

}

