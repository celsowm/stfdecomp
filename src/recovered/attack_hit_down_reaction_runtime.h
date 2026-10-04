#ifndef STF_RECOVERED_ATTACK_HIT_DOWN_REACTION_RUNTIME_H
#define STF_RECOVERED_ATTACK_HIT_DOWN_REACTION_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attack_hit_down_reaction.h"
#include "attack_hit_motion_select.h"
#include "attack_hit_motion_runtime.h"
#include "attack_hit_motion_vector.h"

typedef struct stf_down_reaction_runtime_result {
    stf_hit_motion_select_result motion;
    stf_down_reaction_result reaction;
} stf_down_reaction_runtime_result;

typedef struct stf_down_motion_runtime_result {
    stf_down_reaction_runtime_result down;
    stf_attack_hit_motion_runtime_result prefix;
    stf_motion_vector_result vector;
} stf_down_motion_runtime_result;

bool stf_attack_hit_special_bit16_reaction_apply_resolved_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint16_t hit_flags_50fe00,
    uint32_t damage,
    stf_hit_motion_table_resolver resolver,
    void *resolver_user_data,
    stf_down_reaction_runtime_result *result
);

bool stf_attack_hit_generic_down_motion_apply_resolved_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint16_t hit_flags_50fe00,
    uint8_t hit_kind_50fe02,
    uint32_t damage,
    bool increment_down_combo,
    stf_hit_motion_table_resolver resolver,
    void *resolver_user_data,
    const stf_motion_prefix_inputs *prefix_inputs,
    const stf_motion_hit_rom_view *rom_view,
    const stf_motion_fallback_profile *fallback_profile,
    const stf_motion_vector_inputs *vector_inputs,
    stf_down_motion_runtime_result *result
);

bool stf_attack_hit_generic_down_reaction_apply_resolved_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint16_t hit_flags_50fe00,
    uint8_t hit_kind_50fe02,
    uint32_t damage,
    bool increment_down_combo,
    stf_hit_motion_table_resolver resolver,
    void *resolver_user_data,
    stf_down_reaction_runtime_result *result
);

#endif
