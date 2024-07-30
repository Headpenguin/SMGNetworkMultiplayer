#include "packets.h"
#include "net.h"
#include "errno.h"
#include <cstring>
#include <revolution/types.h>
#include <revolution/os.h>
#include "kamek/hooks.h"

#define ENOMEM 12
#define ENXIO 6
#define ENOBUFS 105


static void *readbuff;
static long sockfd = -1;
#define READBUFF_SIZE 1500
// In ms
#define CONNECT_RECV_TIMEOUT 10000
#define CONNECT_RETRIES 5

static const struct sockaddr_in server_addr = {
    8,
    2,
    5000,
    0x0A000060
};

void* __nw__FUli(unsigned int, int);

/*
    Transmission structure: PACKET_DISC, packet_*, [data1], ... 
*/

static const char *connect_msg = "connect";

enum connectState {
    INVALID = 0,
    SEND_INITIATED,
    POLL,
    READ,
    POSTREAD
};

struct connectCallbackInfo {
    long connectionAttemptsRemaining;
    IOSError connectionError;
    unsigned long dataSentReceived;
    enum connectState state;
    const void *connectPacket;
    unsigned long connectPacketLen;
    struct pollsd pollsd;
} connectCallbackInfo = { 
    .connectionAttemptsRemaining = CONNECT_RETRIES, 
    .pollsd = { .sd = -1, .events = POLLIN }
};

BOOL connected = 0;

static IOSError packet_mainloop(IOSError, void *);

static IOSError connectCallback(IOSError, void *);

static void retry(IOSError err, struct connectCallbackInfo *data) {
        data->connectionAttemptsRemaining--;
        data->connectionError = err;
        data->dataSentReceived = 0;
        data->state = SEND_INITIATED;
        memcpy(readbuff, data->connectPacket, data->connectPacketLen);
        netsendto_async(sockfd, readbuff, data->connectPacketLen, &server_addr, connectCallback, data);
}

static IOSError connectCallback(IOSError err, void *data_) {
    struct connectCallbackInfo *data = data_;
    
    if(data->connectionAttemptsRemaining <= 0) return 0;
    
    if(err < 0) {
        retry(err, data);
        return 0;
    }


    switch(data->state) {
        case SEND_INITIATED:
            data->dataSentReceived += err;
            if(data->dataSentReceived < data->connectPacketLen) {
                netsendto_async(sockfd, (char*)readbuff + data->dataSentReceived, data->connectPacketLen - data->dataSentReceived, &server_addr, connectCallback, data);
                return 0;
            }
            else {
                data->dataSentReceived = 0;
                data->state = POLL;
            }

            break;

        // Handle the data in the server's response packet and maybe enter the POLL state
        // No logic for checking server packets has been implemented yet, so we just return
        case POSTREAD:
            data->dataSentReceived += err;

            // Successful connection
            data->dataSentReceived = 0;
            connected = 1;
            return 0;
    }

    switch(data->state) {
        case POLL:
            data->pollsd.revents = 0;
            data->state = READ;
            netpoll_async(&data->pollsd, 1, CONNECT_RECV_TIMEOUT, connectCallback, data);
            return 0;

       case READ:
            if(err < 1 || !(data->pollsd.revents & POLLIN)) {
                retry(-ETIMEDOUT, data);
                return 0;
            }
            data->state = POSTREAD;
            netread_async(sockfd, (char*)readbuff + data->dataSentReceived, READBUFF_SIZE - data->dataSentReceived, connectCallback, data);
            return 0;
    }
 
    // unreachable
    return -EINVAL;    
    
}

static long connect_to_server_async(void) {
    connectCallbackInfo.pollsd.sd = sockfd;
    connectCallbackInfo.state = SEND_INITIATED;
    connectCallbackInfo.connectPacket = connect_msg;
    connectCallbackInfo.connectPacketLen = 8;
    connectCallbackInfo.connectionAttemptsRemaining = 5;
    memcpy(readbuff, connect_msg, 8);
    return netsendto_async(sockfd, readbuff, 8, &server_addr, connectCallback, &connectCallbackInfo);
}

inline static long packet_init() {
    long err = 0;
    
    err = netinit();
    if(err < 0) return err;
    sockfd = netsocket(2, 2, 0);
    if(sockfd < 0) return sockfd;
    readbuff = __nw__FUli(READBUFF_SIZE, 32);
    if(!readbuff) return -ENOMEM;
    err = connect_to_server_async();
    if(err < 0) return err;
    return 0;
    
}

extern void start__15DrawSyncManagerFUll(unsigned long, long);

static void packet_init_wrapper(unsigned long a, long b) {
    start__15DrawSyncManagerFUll(a, b);
    packet_init();
}

extern kmSymbol init__10GameSystemFv;
//extern kmSymbol __vt__10MarioActor;
//extern kmSymbol substitutionB;

kmCall(&init__10GameSystemFv + 0x94, packet_init_wrapper); // Replaces a call to `DrawSyncManager::start`
//kmWritePointer(&__vt__10MarioActor + 0x14, &substitutionB);
/*
enum packetMainloopState {
    REQ_READ = 0,
    REQ_SEND,
    SEND_INITIATED,
    READ_INITIATED
};

static struct packetMainloopInfo {
    enum packetMainloopState state;
    BOOL isLoopStarted;
    IOSError err;
    unsigned long transmissionLen;
    unsigned long dataSentReceived;
    void *transmission;
} packetMainloopInfo = {.transmission = readbuff};

static IOSError packetMainloop(IOSError err, void *data_) {
    long len = 0;
    unsigned int rwaddr = 0, rwlen = 0;
    struct packetMainloopState *data = data_;
    struct packet_transmission *transmissionStructured = data->transmission;

    if(data->isLoopStarted) {
        if(err < 0) {
            // if we get an error, just give up
            data->err = err;
            return 0;
        }
    }
    else {
        data->isLoopStarted = 1;
    }

    switch(data->state) {
        case SEND_INITIATED:
            data->dataSentReceived += err;
            if(data->dataSentReceived < data->transmissionLen) {
                netsendto_async(sockfd, (char *)data->transmission + data->dataSentReceived, data->transmissionLen - data->dataSentReceived, &serverAddr, packetMainloop, data);
                return 0;
            }
            data->state = READ_REQ;
            data->dataSentReceived = 0;
            break;
        case READ_INITIATED:
            data->dataSentReceived += err;

            if(data->dataSentReceived < sizeof transmission->disc) {
                data->state = READ_REQ;
                break;
            }

            // shouldn't happen
            if(len > READBUFF_SIZE) return -ENOBUFS;

            switch(transmission->disc) {
                case PACKET_DISC_STOP:
                    // done reading
                    transmission->disc = PACKET_DISC_ACK;
                    transmission->ack.len = 0;
                    return netsendto(sockfd, readbuff, sizeof transmission->disc + sizeof transmission->ack, &server_addr);
                case PACKET_DISC_ACK:
                    // done reading
                    break;
                case PACKET_DISC_READ:
                    if(len < sizeof transmission->disc + sizeof transmission->read) {
                        data->state = READ_REQ;
                        break;
                    }
                    rwaddr = transmission->read.addr;
                    rwlen = transmission->read.len;
                    // done reading
                    if(sizeof transmission->disc + sizeof transmission->ack + rwlen > READBUFF_SIZE) continue;
                    transmission->disc = PACKET_DISC_ACK;
                    transmission->ack.len = rwlen;
                    memcpy((u8*)readbuff + sizeof transmission->disc + sizeof transmission->ack, (const void *)rwaddr, rwlen);
                    err = netsendto(sockfd, readbuff, sizeof transmission->disc + sizeof transmission->ack + rwlen, &server_addr);
                    if(err < 0) return err;
                    break;
                case PACKET_DISC_WRITE:
                    if(len < sizeof transmission->disc + sizeof transmission->write) continue;
                    rwaddr = transmission->write.addr;
                    rwlen = transmission->write.len;
                    if(len < sizeof transmission->disc + sizeof transmission->write + rwlen) continue;
                    memcpy((void*)rwaddr, (u8*)readbuff + sizeof transmission->disc + sizeof transmission->write, rwlen);
                    // Fun cache invalidation instructions go here
                    
                    // done reading
                    transmission->disc = PACKET_DISC_ACK;
                    transmission->ack.len = 0;
                    err = netsendto(sockfd, readbuff, sizeof transmission->disc + sizeof transmission->ack, &server_addr);
                    if(err < 0) return err;
                    break;
            }

            break;

    }
    
    err = netread(sockfd, readbuff, READBUFF_SIZE);


}
*/
