#ifndef ATOMIC_H
#define ATOMIC_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long simplelock_t;

typedef enum {
    TRY_LOCK_RESULT_OK = 0,
    TRY_LOCK_RESULT_FAIL = 1,
    TRY_LOCK_RESULT_INT = 2
} tryLockResult_t;
// This works because of the updated the exception vectors
inline tryLockResult_t simplelock_tryLock(register simplelock_t *pLock) {
    register tryLockResult_t val;
    register simplelock_t one = 1;
    asm {
        lwarx val, 0, pLock
        stwcx. one, 0, pLock
        beq+ success //we expect to succeed (an interrupt is very unlikely)
        li val, 2
        success:
    };
    return val;
}

inline tryLockResult_t simplelock_tryLockLoop(simplelock_t *pLock) {
    tryLockResult_t ret;
    while((ret = simplelock_tryLock(pLock)) == TRY_LOCK_RESULT_INT);
    return ret;
}

inline void simplelock_release(simplelock_t *pLock) {
    *pLock = 0;
}

#ifdef __cplusplus
}
#endif

#endif
