#ifndef CONCURRENCYUTILS_HPP
#define CONCURRENCYUTILS_HPP

class ConcurrentQueueInner {

};

// Currently only supports single thread consumption
// but multithread production (could easily support
// multithread consumption if desired though)
template<typename T>
class ConcurrentQueue : ConcurrentQueueInner{
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
    }

    inline const T* read() const {
        if(readHead != writeHead) return &readHead->data;
        return nullptr;
    }

    // Only call this if read succeeded
    inline void advance() {
        readHead->isInit = false;
        if(readHead + 1 >= buffer + len) readHead = buffer;
        else readHead++;
    }
    inline bool write(const T &data) {
        Block *volatile orig;
        do {
            orig = allocHead;
            if(allocHead == readHead) return false;
        } while(!advanceAtomic(allocHead, orig));
        volatile Block *block = orig;

        orig->data = data;
        block->isInit = true;

        orig = writeHead;
        if(block != orig) return true;

        do {
            if(!advanceAtomic(writeHead, orig)) return true;
            orig = writeHead;
        } while(orig != allocHead && orig->isInit);
        
        return true;
    }

private:
    inline bool advanceAtomic(Block *&ptr, Block *orig) {
        Block *dst;
        orig = ptr;
        dst = orig + 1;
        if(dst >= buffer + len) dst = buffer;
        return atomicTryUpdate(&ptr, (u32)dst, (u32)orig);
    }
    Block *buffer;
    u32 len;

    Block *readHead, *writeHead, *allocHead;
};

#endif
