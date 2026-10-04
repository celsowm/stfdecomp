#ifndef STF_RECOVERED_ATTACK_HIT_MOTION_VECTOR_H
#define STF_RECOVERED_ATTACK_HIT_MOTION_VECTOR_H

#include <stdbool.h>
#include <stdint.h>

#include "attack_hit_motion_prefix.h"

typedef struct stf_motion_vector_inputs {
    uint32_t hit_mode;
    int32_t horizontal_angle_r10;
    uint32_t attacker_flags_0;
    uint8_t attacker_7d2;
    uint32_t attacker_70c;
    uint32_t defender_flags_1a4;
    uint32_t profile_horizontal_scale_bits;
    uint32_t profile_vertical_scale_bits;
    uint8_t attacker_843;
} stf_motion_vector_inputs;

typedef struct stf_motion_vector_result {
    uint32_t x_5e0_bits;
    uint32_t y_5e4_bits;
    uint32_t z_5e8_bits;
} stf_motion_vector_result;

/*
 * Recover loc_2B7E0..loc_2B898, including cpres1 commands 0x24/0x25.
 *
 * Starting from attack_hit_motion_prefix's angle_r6 and sqrt_r4 magnitude,
 * the original constructs a 3D knockback vector as:
 *
 *   y = sin(angle_r6) * magnitude
 *   horizontal = cos(angle_r6) * magnitude
 *   x = sin(-horizontal_angle_r10) * horizontal
 *   z = cos(-horizontal_angle_r10) * horizontal
 *
 * It then applies the same hit/profile and attacker-state multipliers observed
 * in the i960 corridor before storing x/y/z at defender +0x5E0/+0x5E4/+0x5E8.
 */
bool stf_attack_hit_motion_vector_compute(
    const stf_motion_prefix_result *prefix,
    const stf_motion_vector_inputs *inputs,
    stf_motion_vector_result *result
);

#endif
