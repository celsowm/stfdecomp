#ifndef STF_RECOVERED_COLLISION_ATTACK_H
#define STF_RECOVERED_COLLISION_ATTACK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_COLLISION_ATTACK_MAPPING_COUNT = 16u
};

typedef struct stf_collision_attack_inputs {
    uint8_t fighter_index;
    uint16_t previous_motion;
    uint16_t current_motion;
    uint32_t flags_1a4;
    uint32_t flags_720;
    uint32_t flags_860;
    uint16_t field_1aa;
    uint16_t field_808;
    uint16_t hit_history_090;
    uint32_t lockout_2ac;
    uint32_t attack_profile_bits;
    uint16_t opponent_suppression_6f8;
} stf_collision_attack_inputs;

typedef struct stf_collision_attack_result {
    uint16_t next_previous_motion;
    uint16_t next_hit_history_090;
    uint16_t overlap_mask;
    uint32_t selected_unit;
    uint16_t hit_latch;
    uint32_t next_lockout_2ac;
    bool previous_motion_written;
    bool hit;
} stf_collision_attack_result;

/*
 * Recover the hardware-independent core of coli_attack_chk.
 *
 * mapping[0..15] corresponds to the original 0x90F600 table. For fighter 0,
 * attack_profile_bits selects table entries whose values are ORed together.
 * For fighter 1 the relationship is transposed: each selected profile bit is
 * tested against every mapping entry and matching entry indexes are ORed.
 *
 * The effect/impact position side effects after a successful hit are not part
 * of this contract yet.
 */
bool stf_collision_attack_resolve(
    const stf_collision_attack_inputs *inputs,
    const uint16_t mapping[STF_COLLISION_ATTACK_MAPPING_COUNT],
    stf_collision_attack_result *result
);

#endif
