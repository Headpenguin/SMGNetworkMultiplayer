#ifndef CONCURRENCYUTILS_HPP
#define CONCURRENCYUTILS_HPP

#include "atomic.h"
#include <revolution/types.h>
   

// Currently only supports single thread consumption
// but multithread production (could easily support
// multithread consumption if desired though)
template<typename T>
class ConcurrentQueue {
    void _write();
public:
    struct Block {
        inline Block() : isInit(false) {}
        
        bool isInit;
        T data;
    };

    inline void init(Block *_buffer, u32 _len) {
        buffer = _buffer;
        len = _len;

        readHead = _buffer;
        writeHead = _buffer;
        allocHead = _buffer;

        sem_init(&empty, len);
        sem_init(&full, 0);
    }

    inline const T* read() const {
        if(full > 0) return &readHead->data; // not an issue for now since single reader
        return nullptr;
    }

    // Only call this if read succeeded
    inline void advance() {
        readHead->isInit = false;
        if(readHead + 1 >= buffer + len) readHead = buffer;
        else readHead++;
        sem_down(&full);
        sem_up(&empty);
    }
    inline bool write(const T &data) {
        if(!sem_down(&empty)) return false;

        Block *block;
        do {
            block = allocHead;
        } while(!advanceAtomic(allocHead, block));

        volatile Block *vblock = block;

        block->data = data;
        vblock->isInit = true;

        if(block != writeHead) return true;

        do {
            if(!advanceAtomic(writeHead, block)) return true;
            sem_up(&full);
            block = writeHead;
        } while(block != allocHead && block->isInit);
        
        return true;
    }

private:
    inline bool advanceAtomic(Block *&ptr, Block *orig) {
        Block *dst;
        dst = orig + 1;
        if(dst >= buffer + len) dst = buffer;
        return atomicTryUpdate(&ptr, (u32)dst, (u32)orig);
    }
    Block *buffer;
    u32 len;

    Block *readHead, *writeHead, *allocHead;

    sem_t empty, full;
};

#endif
