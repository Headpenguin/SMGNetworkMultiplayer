#ifndef PLAYERCOMMON_HPP
#define PLAYERCOMMON_HPP

#include <revolution/types.h>

namespace Multiplayer {

/*
 *
 * To allow for simpler game logic, all id's obtained from the server
 * are mapped such that the current console's id is always 0xFF and
 * all remaining id's are still contiguous.
 *
 * Note that not necessarily all players with the remaining id's will 
 * be active. Care must be taken to properly deactivate/clear all 
 * player structures once it is known that a particular player is inactive.
 *
 */ 
class Id {
    u8 id;
    u8 consoleId;
    inline Id(u8 a, u8 b) : id(a), consoleId(b) {}
public:
    inline Id() {}
    // Intended for use in game logic
    inline u8 toLocalId() const {
        if(id < consoleId) return id;
        if(id == consoleId) return 0xFF;
        if(id > consoleId) return id - 1;
        return id; // unreachable
    }
    inline u8 toGlobalId() const {return id;}
    // Intended for creating almost all `Id` structures
    inline Id fromLocalId(u8 localId) const {
        if(localId == 0xFF) localId = consoleId;
        else if(localId >= consoleId) localId += 1;
        Id ret(localId, consoleId);
        return ret;
    }
    inline Id fromGlobalId(u8 globalId) {
        Id ret(globalId, consoleId);
        return ret;
    }
    inline Id createSelfId() const {
        Id ret(consoleId, consoleId);
        return ret;
    }
    // Intended for use only when getting id from server
    static inline Id selfId(u8 consoleId) {
        Id ret(consoleId, consoleId);
        return ret;
    }
};

}

#endif
