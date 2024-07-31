#ifndef PACKETS_H
#define PACKETS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int BOOL;

enum PACKET_DISC {
    PACKET_DISC_STOP, 
    PACKET_DISC_ACK, 
    PACKET_DISC_READ, 
    PACKET_DISC_WRITE
};

struct packet_ack {
    unsigned int len;
};

struct packet_read {
    unsigned int addr;
    unsigned int len;
};

struct packet_write {
    unsigned int addr;
    unsigned int len;
};

struct packet_transmission {
    unsigned int disc;
    union {
        struct packet_ack ack;
        struct packet_read read;
        struct packet_write write;
    };
};

BOOL connected;

#ifdef __cplusplus
}
#endif

#endif
