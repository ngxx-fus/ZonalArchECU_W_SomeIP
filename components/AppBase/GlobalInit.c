#include "GlobalInit.h"

/* Define and initialize the atomic variable */
atomic_uint_fast8_t GlobalInit_Level = ATOMIC_VAR_INIT(0);

uint8_t GlobalInit_GetLevel(void) {
    uint8_t current_level;

    /* Control flow: atomically read the variable to prevent race conditions */
    current_level = (uint8_t)atomic_load(&GlobalInit_Level);
    
    /* Return the safely loaded level */
    return current_level;
}

uint8_t GlobalInit_MoveNextLevel(void) {
    uint_fast8_t current_level;
    uint_fast8_t next_level;

    /* Control flow: load the current value before attempting to modify it */
    current_level = atomic_load(&GlobalInit_Level);

    /* Control flow: safely increment using Compare-And-Swap (CAS) to prevent overflow */
    do {
        /* Control flow: cap the value at maximum level (255) */
        if (current_level >= GLOBAL_INIT_LEVEL_DONE) {
            return (uint8_t)GLOBAL_INIT_LEVEL_DONE;
        }
        
        next_level = current_level + 1;
        
    /* Control flow: attempt to swap. If another task modified GlobalInit_Level 
     * in the background, this fails and automatically updates current_level */
    } while (!atomic_compare_exchange_weak(&GlobalInit_Level, &current_level, next_level));

    /* Return the newly incremented value */
    return (uint8_t)next_level;
}

uint8_t GlobalInit_Done(void) {
    /* Control flow: atomically overwrite the variable with the final done state */
    atomic_store(&GlobalInit_Level, GLOBAL_INIT_LEVEL_DONE);
    
    /* Return the done magic number */
    return (uint8_t)GLOBAL_INIT_LEVEL_DONE;
}