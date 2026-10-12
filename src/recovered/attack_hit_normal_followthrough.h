#ifndef STF_RECOVERED_ATTACK_HIT_NORMAL_FOLLOWTHROUGH_H
#define STF_RECOVERED_ATTACK_HIT_NORMAL_FOLLOWTHROUGH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attack_hit_combo.h"
#include "attack_hit_state_prelude.h"
#include "attack_hit_damage.h"
#include "attack_hit_finish.h"
#include "attack_hit_reaction.h"
#include "attack_hit_normal_reaction_runtime.h"

typedef struct stf_attack_hit_normal_followthrough_result {
    bool combo_applied;
    bool state_prelude_applied;
    bool damage_transform_applied;
    bool finish_applied;
    bool reaction_classified;
    bool normal_reaction_applied;
    stf_attack_hit_combo_result combo;
    stf_attack_hit_state_prelude_result state_prelude;
    stf_attack_damage_result damage;
    stf_attack_finish_result finish;
    stf_attack_reaction_path reaction_path;
    stf_normal_reaction_runtime_result normal_reaction;
} stf_attack_hit_normal_followthrough_result;

/* Apply the recovered CPU steps AFTER the normal 0x2AE40 exit test,
 * BEFORE the still-mandatory attack_hit continuation callback.
 * Initial damage and energy settings come from caller game context.
 * The normal motion resolver must access genuine ROM-backed tables.
 * Result flags describe irreversible partial work on failure.
 * Device/sound/damage_calculation work is intentionally not simulated.
 */
bool stf_attack_hit_normal_followthrough_apply_model2(
    uint8_t *attacker, size_t attacker_size,
    uint8_t *defender, size_t defender_size,
    uint8_t *workspace, size_t workspace_size,
    const stf_attack_side_exit_result *side_exit,
    uint8_t hit_kind_50fe02,
    uint16_t hit_flags_50fe00,
    uint8_t hit_flags_50fe03,
    uint32_t initial_damage,
    int16_t energy_max,
    bool disable_energy_scaling,
    stf_hit_motion_table_resolver motion_resolver,
    void *motion_resolver_user_data,
    stf_attack_hit_normal_followthrough_result *result
);

#endif
