#ifndef ATOMIC_H
#define ATOMIC_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long simplelock_t;
typedef int BOOL;

typedef enum {
    TRY_LOCK_RESULT_OK = 0,
    TRY_LOCK_RESULT_INT = 1,
    TRY_LOCK_RESULT_FAIL = 2
} tryLockResult_t;
// This works because of the updated the exception vectors
inline tryLockResult_t simplelock_tryLock(register volatile simplelock_t *pLock) {
    register tryLockResult_t val;
    register simplelock_t one = 2;
    asm {
        lwarx val, 0, pLock
        stwcx. one, 0, pLock
        beq+ success //we expect to succeed (an interrupt is very unlikely)
        addi val, val, 1 // 0->INT, 1->FAIL
        success:
    };
    return val;
}

inline tryLockResult_t atomicSet(register volatile void *dst, register unsigned int value, register unsigned int orig) {
    register unsigned int currValue;
    register tryLockResult_t ret;
    asm {
        lwarx currValue, 0, dst
        cmp currValue, orig
        bne- failure
        stwcx. value, 0, dst
        beq+ success
        
        li ret, 1 // INT
        b end
        
        failure:
        li ret, 2 // failure
        b end

        success:
        li ret, 0 // OK
        
        end:
    };
    return ret;
}

inline tryLockResult_t simplelock_tryLockLoop(volatile simplelock_t *pLock) {
    tryLockResult_t ret;
    while((ret = simplelock_tryLock(pLock)) == TRY_LOCK_RESULT_INT);
    return ret;
}

inline void simplelock_release(volatile simplelock_t *pLock) {
    *pLock = 0;
}

inline BOOL atomicTryUpdate(volatile void *ptr, unsigned int value, unsigned int origValue) {
    tryLockResult_t ret;
    while((ret = atomicSet(ptr, value, origValue)) == TRY_LOCK_RESULT_INT);
    return ret == TRY_LOCK_RESULT_OK;
}

typedef unsigned long sem_t;

inline void sem_init(sem_t *sem, unsigned long val) {*sem = val;}

inline BOOL sem_down(volatile sem_t *sem) {
    BOOL succ = 0;
    while(!succ) {
        sem_t val = *sem;
        if(val == 0) break;
        succ = atomicTryUpdate(sem, val - 1, val);
    }
    return succ;
}

inline void sem_up(volatile sem_t *sem) {
    BOOL succ = 0;
    while(!succ) {
        sem_t val = *sem;
        succ = atomicTryUpdate(sem, val + 1, val);
    }
}

#define LWARX(label, value, ptr) label: asm {lwarx value, 0, ptr};
#define STWCX(label, value, ptr) asm {\
    stwcx. value, 0, ptr; \
    bne label; \
}
#define CLEAR(value, ptr) asm {stwcx. value, 0, ptr}

#ifdef __cplusplus
}
#endif

#endif
