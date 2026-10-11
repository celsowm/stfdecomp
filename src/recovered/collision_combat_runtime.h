#ifndef STF_RECOVERED_COLLISION_COMBAT_RUNTIME_H
#define STF_RECOVERED_COLLISION_COMBAT_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attack_hit.h"
#include "collision_attack.h"
#include "collision_hit_order.h"
#include "damage_unit_runtime.h"

/*
 * Portable seam between collision's selected attack_hit/damage_unit pairs.
 * The caller supplies the body of attack_hit after its recovered prefix.
 * This MUST run the real recovered continuation (including its branch
 * predicates and potential workspace+0x26C changes), not blindly accept
 * the hit. A hook returning false aborts the sequence; state is NOT rolled
 * back. special_opponent_path marks the sub_19888 early-return corridor.
 */
typedef bool (*stf_collision_attack_hit_body_fn)(
    void *user_data,
    uint8_t attacker_index,
    uint8_t defender_index,
    uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint8_t *workspace,
    size_t workspace_size,
    const stf_attack_hit_prefix_result *prefix
);

typedef struct stf_collision_combat_model2 {
    uint8_t *fighters[2];
    size_t fighter_sizes[2];
    uint8_t *enemy_slots[2];
    size_t enemy_slot_sizes[2];
    uint8_t *workspace;
    size_t workspace_size;
    stf_damage_unit_effect_state *effect;
    uint8_t num_rounds_to_win;
    uint8_t also_mode;
    uint8_t also_sub_mode;
    stf_collision_attack_hit_body_fn attack_body;
    void *attack_body_user_data;
} stf_collision_combat_model2;

typedef struct stf_collision_combat_pair_result {
    uint8_t attacker_index;
    uint8_t defender_index;
    stf_attack_hit_prefix_result prefix;
    stf_damage_unit_runtime_result damage;
    bool prefix_applied;
    bool body_completed;
    bool damage_completed;
} stf_collision_combat_pair_result;

typedef struct stf_collision_combat_result {
    uint8_t completed;
    uint8_t attempted;
    stf_collision_combat_pair_result pairs[2];
} stf_collision_combat_result;

/*
 * Uses the frozen hit order produced by stf_collision_frame_check_model2:
 * for every pair, apply attack_hit's actual CPU prefix, call the explicit
 * continuation, then apply recovered damage_unit and post-damage state.
 *
 * Validate all shared buffers and the dispatch list before mutating anything.
 * A failed callback or downstream step may leave partial state: attempted
 * and completed identify that boundary for differential replay.
 */
bool stf_collision_combat_apply_model2(
    const stf_collision_combat_model2 *model,
    const stf_collision_hit_order *order,
    stf_collision_combat_result *result
);

#endif
