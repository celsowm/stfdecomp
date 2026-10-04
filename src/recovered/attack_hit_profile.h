#ifndef STF_RECOVERED_ATTACK_HIT_PROFILE_H
#define STF_RECOVERED_ATTACK_HIT_PROFILE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attack_hit_motion_prefix.h"

enum {
    STF_ATTACK_HIT_PROFILE_RECORD_SIZE = 40u
};

typedef struct stf_attack_hit_profile {
    uint32_t horizontal_scale_bits;      /* +0x00 */
    uint32_t vertical_scale_bits;        /* +0x04 */
    uint32_t strength_scale_bits;        /* +0x08 */
    uint32_t unknown_0c;                 /* +0x0C */
    stf_motion_fallback_profile fallback;/* +0x10..+0x27 */
} stf_attack_hit_profile;

/*
 * Decode the 40-byte per-hit-kind record addressed by:
 *   0x50A800 + hit_kind * 40
 * in the original runtime.
 */
bool stf_attack_hit_profile_decode(
    const uint8_t *table_bytes,
    size_t table_size,
    uint8_t hit_kind,
    stf_attack_hit_profile *result
);

#endif
