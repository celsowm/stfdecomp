#ifndef STF_RECOVERED_COLLISION_AREA_SIDE_H
#define STF_RECOVERED_COLLISION_AREA_SIDE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "collision_area_distance.h"

enum {
    STF_COLLISION_AREA_ANGLE_LUT_COUNT = 16u,
    STF_COLLISION_AREA_SIDE_FIGHTER_MIN_SIZE = 0xAE4u,
    STF_COLLISION_AREA_SIDE_WORKSPACE_MIN_SIZE = 0x240u
};

typedef struct stf_collision_area_side_result {
    stf_collision_area_plane plane;
    bool applied;
    uint8_t lut_index;
    uint8_t orientation_code;
    uint16_t heading_1f0;
    uint16_t selected_1f8;
    uint16_t min_plane_index;
    int32_t minimum_record_offset;
    uint32_t minimum_bits;
    uint32_t next_flags_a08;
} stf_collision_area_side_result;

/*
 * i960 area_coli 0x2DF38..0x2E074, for the X or Z branch.
 * angle_lut[16] is caller-provided byte_2E260: no ROM table is embedded.
 * Both branches store the 16-bit heading at task+0x1F0[slot*2],
 * OR the orientation sign into fighter+0xA08, set task+0x23C=2,
 * write task+0x1F4 and fighter+0xAE2 with the low orientation bits.
 * Scan the four +0x650..+0x65C floats in original order (ties select
 * the later candidate) and write the packed value at task+0x1F8.
 *
 * A no-plane branch is reported without changes. The subsequent opposing
 * fighter scan / 0x2E230 and 0x2E250 lookup remains outside this function.
 * The optional supplied table is a bounded view of program data, not a
 * guessed heading-to-sector approximation.
 */
bool stf_collision_area_side_apply_model2(
    uint8_t *fighter,
    size_t fighter_size,
    uint8_t *workspace,
    size_t workspace_size,
    const uint8_t *angle_lut,
    size_t angle_lut_size,
    stf_collision_area_side_result *result
);

#endif
