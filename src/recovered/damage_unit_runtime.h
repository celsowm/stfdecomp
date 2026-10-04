#ifndef STF_RECOVERED_DAMAGE_UNIT_RUNTIME_H
#define STF_RECOVERED_DAMAGE_UNIT_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "damage_unit.h"
#include "damage_unit_post.h"

typedef struct stf_damage_unit_runtime_result {
    stf_damage_unit_result damage;
    bool post_applied;
    stf_damage_unit_post_result post;
} stf_damage_unit_runtime_result;

/*
 * Compose the recovered CPU-visible damage_unit transaction.
 *
 * The workspace bit-0 gate belongs to the original entry path. When it
 * rejects the event, damage_unit returns before the post-calc tail. Otherwise
 * the recovered accumulation/calc_up_down_damage prefix is followed by the
 * recovered post-state/effect-flag tail.
 */
bool stf_damage_unit_runtime_apply_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    const uint8_t *workspace,
    size_t workspace_size,
    uint8_t num_rounds_to_win,
    stf_damage_unit_effect_state *effect,
    uint8_t also_mode,
    uint8_t also_sub_mode,
    stf_damage_unit_runtime_result *result
);

#endif
