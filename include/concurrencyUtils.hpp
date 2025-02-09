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

        frozen = 0;

        readHead = _buffer;
        writeHead = _buffer;
        allocHead = _buffer;
    }

    inline const T* read() const {
        if(readHead != writeHead || frozen) return &readHead->data;
        return nullptr;
    }

    // Only call this if read succeeded
    inline void advance() {
        readHead->isInit = false;
        if(readHead + 1 >= buffer + len) readHead = buffer;
        else readHead++;
        frozen = false;
    }
    inline bool write(const T &data) {
        Block *block;
        {
            BOOL mask = OSDisableInterrupts();
            if(frozen) {
                OSRestoreInterrupts(mask);
                return false;
            }
            block = allocHead++;
            if(allocHead >= buffer + len) allocHead = buffer;
            if(allocHead == readHead) frozen = true;
            OSRestoreInterrupts(mask);
        }
        volatile Block *vblock = block;

        block->data = data;
        vblock->isInit = true;

        if(block != writeHead) return true;

        do {
            if(!advanceAtomic(writeHead, block)) return true;
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

    volatile u32 frozen;

    Block *readHead, *writeHead, *allocHead;
};

#endif
