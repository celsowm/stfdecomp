#ifndef STF_RECOVERED_COLLISION_KIND_H
#define STF_RECOVERED_COLLISION_KIND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_COLLISION_KIND_FIGHTER_MIN_SIZE = 0xC5Cu,
    STF_COLLISION_KIND_WORKSPACE_MIN_SIZE = 0x1E4u,
    STF_UNIT_TO_BALL_UNIT_COUNT = 32u,
    STF_UNIT_TO_BALL_ROW_BYTES = 8u,
    STF_COLLISION_KIND_APPLY_WORKSPACE_MIN_SIZE = 0x284u
};

typedef enum stf_collision_kind {
    STF_COLLISION_KIND_NORMAL,
    STF_COLLISION_KIND_FIX_0,
    STF_COLLISION_KIND_FIX_1,
    STF_COLLISION_KIND_BODY,
    STF_COLLISION_KIND_BODY_0,
    STF_COLLISION_KIND_KABE,
    STF_COLLISION_KIND_KABE_0,
    STF_COLLISION_KIND_RIDE_0,
    STF_COLLISION_KIND_RIDE_1,
    STF_COLLISION_KIND_JUMP_0,
    STF_COLLISION_KIND_JUMP_1,
    STF_COLLISION_KIND_RISE_0,
    STF_COLLISION_KIND_RISE_1,
    STF_COLLISION_KIND_TOBIKOE_0,
    STF_COLLISION_KIND_TOBIKOE_1,
    STF_COLLISION_KIND_COUNT
} stf_collision_kind;

typedef struct stf_collision_kind_result {
    stf_collision_kind kind;
    uint32_t flags_g6;
    uint8_t angle_index;
    uint32_t selected_plane_bits;
} stf_collision_kind_result;

/* decide_coli_kind 0x2CFD0..0x2D248; uses precomputed area_coli planes
 * and the current fighter state. All results are local, no RAM writes. */
bool stf_collision_kind_select_model2(
    const uint8_t *fighter0, size_t fighter0_size,
    const uint8_t *fighter1, size_t fighter1_size,
    const uint8_t *workspace, size_t workspace_size,
    stf_collision_kind_result *result
);

/* unit_to_ball 0x2D320: each selected unit points at a row of 8 bytes.
 * The first byte is always a ball (byte>>2); following zero entries
 * terminate the list. An unterminated row is rejected instead of
 * reading into the next unit's record. Bits are i960 setbit modulo 32. */
bool stf_collision_unit_to_ball_model2(
    uint32_t unit_mask, const uint8_t *unit_rows, size_t rows_size,
    uint32_t *ball_mask
);

/* Original NORMAL/FIX/BODY/etc table row, supplied by the caller.
 * Each row contains the eight i960 words at r12+0x00..+0x1C. */
typedef struct stf_collision_kind_profile {
    uint32_t units0, units1;
    uint32_t scalar_cc, scalar_d0;
    uint32_t scalar_c8, scalar_88;
    uint32_t units264, units268;
} stf_collision_kind_profile;

typedef bool (*stf_collision_kind_special_fn)(
    void *user_data, uint32_t input0, uint32_t input1,
    uint32_t *output0, uint32_t *output1
);

/* decide_coli_kind 0x2D248..0x2D318:
 * expand unit masks, complement the primary exclusion masks,
 * and write C0/C4, CC/D0, C8/88, 264/268.
 * The special 0x2D005A5A coprocessor request, when necessary,
 * must be provided explicitly; no fabricated output is permitted. */
bool stf_collision_kind_apply_model2(
    const stf_collision_kind_result *selection,
    const stf_collision_kind_profile *profiles, size_t profile_count,
    const uint8_t *unit_rows0, size_t rows0_size,
    const uint8_t *unit_rows1, size_t rows1_size,
    uint8_t *workspace, size_t workspace_size,
    stf_collision_kind_special_fn special, void *special_user_data
);
#endif
