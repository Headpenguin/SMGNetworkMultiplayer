#ifndef ATOMIC_H
#define ATOMIC_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long simplelock_t;
typedef int BOOL;

// This works because of the updated the exception vectors
inline BOOL simplelock_tryLock(register simplelock_t *pLock) {
    register simplelock_t val;
    register simplelock_t one = 1;
    asm {
        lwarx val, 0, pLock
        stwcx. one, 0, pLock
        beq+ success //we expect to succeed (an interrupt is very unlikely)
        li val, 1
        success:
    };
    return !val;
}

inline void simplelock_release(simplelock_t *pLock) {
    *pLock = 0;
}

#ifdef __cplusplus
}
#endif

#endif
