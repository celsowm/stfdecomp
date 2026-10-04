#ifndef STF_RECOVERED_DAMAGE_UNIT_H
#define STF_RECOVERED_DAMAGE_UNIT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_DAMAGE_UNIT_ATTACKER_MIN_SIZE = 0x1F5Du,
    STF_DAMAGE_UNIT_DEFENDER_MIN_SIZE = 0x1F82u,
    STF_DAMAGE_UNIT_WORKSPACE_MIN_SIZE = 0x270u,
    STF_DAMAGE_UNIT_KIND_CATEGORY_COUNT = 12u
};

typedef struct stf_damage_unit_effect_state {
    uint32_t active_914;
} stf_damage_unit_effect_state;

typedef struct stf_damage_unit_result {
    bool skipped;
    bool matched_slot;
    uint8_t selected_slot;
    uint8_t selected_category;
    uint16_t accumulator_before;
    uint16_t accumulator_after;
    bool final_round_override;
    uint32_t up_total_1f74;
    uint32_t down_total_1f78;
    uint32_t flags_7f0;
} stf_damage_unit_result;

/*
 * Recover damage_unit entry through the return from calc_up_down_damage.
 *
 * This covers:
 * - workspace bit-0 gating;
 * - highest-set-bit scan of defender +0x6F0;
 * - byte_2CA3C attacker-kind category matching;
 * - +0x1F00[16] damage accumulation;
 * - final-round 0x1000 accumulator override/effect activation;
 * - calc_up_down_damage totals, history copies and threshold flags.
 *
 * Later crush-parts/audio/particle effects remain outside this CPU-state
 * contract.
 */
bool stf_damage_unit_apply_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    const uint8_t *workspace,
    size_t workspace_size,
    uint8_t num_rounds_to_win,
    stf_damage_unit_effect_state *effect,
    stf_damage_unit_result *result
);

const uint8_t *stf_damage_unit_kind_categories(size_t *count);

#endif
