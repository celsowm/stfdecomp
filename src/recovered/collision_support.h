#ifndef STF_RECOVERED_COLLISION_SUPPORT_H
#define STF_RECOVERED_COLLISION_SUPPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_COLLISION_SUPPORT_MODEL2_MIN_SIZE = 0x240u
};

typedef struct stf_collision_support_prelude {
    uint32_t flags_1b0;
    uint16_t counter_220;
    uint16_t counter_23c;
    uint16_t counter_23e;
} stf_collision_support_prelude;

/*
 * Recover the hardware-independent prefix of support_rob_position.
 *
 * When debug_flag bit 5 is set the original routine returns before making any
 * of these writes.
 */
void stf_collision_support_prelude_step(
    stf_collision_support_prelude *state,
    uint32_t debug_flag
);

bool stf_collision_support_prelude_apply_model2(
    uint8_t *workspace,
    size_t workspace_size,
    uint32_t debug_flag
);

#endif
