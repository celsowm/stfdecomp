#ifndef STF_RECOVERED_ATTACK_HIT_NORMAL_REACTION_RUNTIME_H
#define STF_RECOVERED_ATTACK_HIT_NORMAL_REACTION_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attack_hit_motion_select.h"
#include "attack_hit_normal_reaction.h"

typedef struct stf_normal_reaction_runtime_result {
    stf_hit_motion_select_result motion;
    stf_normal_reaction_result reaction;
} stf_normal_reaction_runtime_result;

/*
 * Compose loc_2B488: (hit_mode & 0x1f) -> sub_2B94C -> normal reaction.
 *
 * uk_hit_motions remains caller-supplied through table_resolver.
 */
bool stf_attack_hit_normal_reaction_apply_resolved_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint16_t hit_flags_50fe00,
    uint8_t hit_flags_50fe03,
    uint32_t damage,
    uint32_t hit_mode,
    stf_hit_motion_table_resolver table_resolver,
    void *resolver_user_data,
    stf_normal_reaction_runtime_result *result
);

#endif
