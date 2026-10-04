#ifndef STF_RECOVERED_COLLISION_NO_UNIT_H
#define STF_RECOVERED_COLLISION_NO_UNIT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_COLLISION_NO_UNIT_SELF_MIN_SIZE = 0x1E14u,
    STF_COLLISION_NO_UNIT_OTHER_MIN_SIZE = 0xA2Au
};

/*
 * Portable recovery of no_coli_unit_set.
 *
 * The original function computes a 16-bit suppression mask and stores it at
 * fighter + 0x6F8. Inputs stay offset-based until their gameplay names are
 * independently proven.
 */
bool stf_collision_no_unit_mask_model2(
    const uint8_t *fighter,
    size_t fighter_size,
    const uint8_t *opponent,
    size_t opponent_size,
    uint32_t stage_floor_bits,
    uint32_t stage_extent_bits,
    uint32_t no_coli_low_bits,
    uint16_t *mask
);

bool stf_collision_no_unit_apply_model2(
    uint8_t *fighter,
    size_t fighter_size,
    const uint8_t *opponent,
    size_t opponent_size,
    uint32_t stage_floor_bits,
    uint32_t stage_extent_bits,
    uint32_t no_coli_low_bits
);

#endif
