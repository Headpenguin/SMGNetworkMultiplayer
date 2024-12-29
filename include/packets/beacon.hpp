#ifndef PACKETS_BEACON_HPP
#define PACKETS_BEACON_HPP

#include "packets.hpp"

namespace Packets {

class _TimeQuery {
    const static u32 implementationSize;
    static u32 seqNum; // Only modified+read from Beacon::update (only called from one thread)
public:

    // THIS TYPE INSTANTIATES A TEMPLATE USED IN AN UNTHREADED CONTEXT. PLEASE REFER TO
    // `Beacon::process` FOR MORE DETAILS BEFORE MODIFYING
    typedef ReliablePacketCode ReliablePacketCode;
    
    u32 timeMs;
    ReliablePacketCode check;

    inline void init(u32 _timeMs) {
        timeMs = _timeMs;
        check = ReliablePacketCode(seqNum++);
    }

    NetReturn netWriteToBuffer(void *, u32) const;
    static NetReturn netReadFromBuffer(Packet<_TimeQuery> *out, const void *buff, u32 len);
    inline static Tag getTag() {return TIME_QUERY;}
    inline u32 getSize() const {return implementationSize;}
};

class _TimeResponse {
    const static u32 implementationSize;
public:

    u32 timeMs;
    ReliablePacketCode check;


    // BE CAREFUL: THIS GETS CALLED FROM AN UNTHREADED CONTEXT
    // EXTRA CAUTION: T::verify MUST NOT REQUIRE A THREAD
    template<typename T>
    inline bool verify(const T &requestCheck) const {
        return requestCheck.verify(check);
    }

    NetReturn netWriteToBuffer(void *, u32) const;
    static NetReturn netReadFromBuffer(Packet<_TimeResponse> *out, const void *buff, u32 len);
    inline static Tag getTag() {return TIME_RESPONSE;}
    inline u32 getSize() const {return implementationSize;}
};

typedef Packet<_TimeQuery> TimeQuery;
typedef Packet<_TimeResponse> TimeResponse;

};

#endif
