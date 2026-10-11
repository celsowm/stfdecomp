#ifndef STF_RECOVERED_COLLISION_ATTACK_EFFECT_H
#define STF_RECOVERED_COLLISION_ATTACK_EFFECT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "collision_attack.h"

enum {
    STF_COLLISION_ATTACK_EFFECT_XYZ_SIZE = 12u,
    STF_COLLISION_ATTACK_EFFECT_MODEL2_SIZE = 0x904u
};

/*
 * The accepted-hit tail of coli_attack_chk at 0x2A780..0x2A7C4.
 *
 * defender +0x1F4+12*selected_unit: three position words -> effect +0x7C,
 * effect +0x8EC and 0x50A3FC. effect +0x78 receives countdown 10.
 *
 * Original (r5)[r6*1] -> effect+0x8F8 requires an independently established
 * r5 pointer, so the 12 source bytes are passed explicitly. Nothing here
 * guesses that base pointer or emulates the Model 2 device.
 *
 * No hit is a no-op; malformed hit/output buffers fail without writes.
 */
bool stf_collision_attack_effect_apply_model2(
    const stf_collision_attack_result *hit,
    const uint8_t *defender,
    size_t defender_size,
    const uint8_t *r5_selected_xyz,
    size_t r5_selected_xyz_size,
    uint8_t *effect,
    size_t effect_size,
    uint8_t *copro_hit_xyz,
    size_t copro_hit_xyz_size
);

#endif
