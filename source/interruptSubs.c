#include "kamek/hooks.h"
#include <revolution/os.h>

/*
We need all interrupt handlers to invoke `stwcx` so that interrupts clear the RESERVE bit
To achieve this, we modify `OSExceptionVector`. However, `OSExceptionVector` is copied into
the corresponding addresses for each interrupt vector by `OSExceptionInit`, so the new length
of `OSExceptionVector`, along with the new locations of any impacted labels, must
be updated in `OSExceptionInit`.

***************************************

Also note that this entire file must be included as 
a static patch since loader cannot be used until after 
each of the interrupt vectors have already been written
*/

void __OSEVSubStart(void);
void __SUBDBVECTOR(void);
void __OSEVSubSetNumber(void);
void __OSEVSubEnd(void);
void OSDefaultExceptionHandler(__OSException, OSContext *);
static asm void OSExceptionVectorSub(void) {
    
    nofralloc
entry __OSEVSubStart
    mtsprg 0, r4
    lwz r4, 0xC0
    stw r3, 12(r4)
    mfsprg  r3, 0
    stw r3, 16(r4)
    stw r5, 20(r4)
    lhz r3, 418(r4)
    ori r3, r3, 2
    sth r3, 418(r4)

    mfcr r3
    stw r3, 128(r4)
    mflr r3
    stw r3, 132(r4)
    mfctr r3
    stw r3, 136(r4)
    mfxer r3
    stw r3, 140(r4)
    mfsrr0 r3
    stw r3, 408(r4)
    mfsrr1 r3
    stw r3, 412(r4)
    
    // Added `li` + `stwcx`
    li r5, 412
    stwcx. r3, r5, r4 
    
    mr r5, r3

entry __SUBDBVECTOR

    // Deleted `nop`

    mfmsr r3
    ori r3, r3, 0x30
    mtsrr1 r3

entry __OSEVSubSetNumber
    addi r3, 0, 0

    lwz r4, 0xD4
    rlwinm. r5, r5, 0, 30, 30
    bne recover
    addis r5, 0, OSDefaultExceptionHandler@ha
    addi r5, r5, OSDefaultExceptionHandler@l
    mtsrr0 r5
    rfi

recover:
    rlwinm r5, r3, 2, 22, 29
    lwz r5, 0x3000(r5)
    mtsrr0 r5

    rfi

entry __OSEVSubEnd
// Deleted `nop`
}

extern kmSymbol OSExceptionInit;

// Avoid writing `nop` to `__DBVECTOR`
kmBranch(&OSExceptionInit + 0xFC, &OSExceptionInit + 0x18C);

// Use the correct address for `__OSEVEnd`
kmWriteInstruction(&OSExceptionInit + 0x34, addi r3, r3, __OSEVSubEnd@l);

// Use the correct address for `__OSEVSetNumber`
kmWriteInstruction(&OSExceptionInit + 0x38, lwzu r27, __OSEVSubSetNumber@l(r28));

