#ifndef STF_RECOVERED_COLLISION_RAM_INIT_H
#define STF_RECOVERED_COLLISION_RAM_INIT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_COLLISION_RAM_INIT_WORKSPACE_MIN_SIZE = 0x1B0u,
    STF_COLLISION_RAM_INIT_FIGHTER_MIN_SIZE = 0xA70u
};

/*
 * CPU writes of coli_ram_init (0x2D530..), called by coli_cont_cop.
 * It resets the task's 0x170..0x1AF and 0x98..0xAB ranges and each
 * fighter's +0x75C/+0x75E halfwords and +0xA68/+0xA6C dwords.
 * Caller must run it at the original pre-coprocessor boundary; it does
 * not replace a Model 2 coprocessor command or the full collision task.
 */
bool stf_collision_ram_init_apply_model2(
    uint8_t *workspace,
    size_t workspace_size,
    uint8_t *fighter0,
    size_t fighter0_size,
    uint8_t *fighter1,
    size_t fighter1_size
);

#endif
