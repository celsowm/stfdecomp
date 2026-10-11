#ifndef STF_RECOVERED_COLLISION_COMBAT_RUNTIME_H
#define STF_RECOVERED_COLLISION_COMBAT_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attack_hit.h"
#include "attack_hit_special.h"
#include "attack_hit_normal_entry.h"
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
    /* Optional genuine sub_19888 path for defender +0x19F==0x12. */
    stf_attack_hit_special_record_fn special_record;
    void *special_record_user_data;
    stf_attack_hit_special_tgp_fn special_tgp;
    void *special_tgp_user_data;
    /* Optional ROM-backed normal attack prelude, excluding sub_19888. */
    const uint8_t *normal_profile_table;
    size_t normal_profile_table_size;
} stf_collision_combat_model2;

typedef struct stf_collision_combat_pair_result {
    uint8_t attacker_index;
    uint8_t defender_index;
    stf_attack_hit_prefix_result prefix;
    stf_damage_unit_runtime_result damage;
    bool prefix_applied;
    bool special_applied;
    bool normal_entry_applied;
    stf_attack_hit_normal_entry_result normal_entry;
    stf_attack_hit_special_result special;
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
 * When special_record is provided and the defender is in state 0x12,
 * execute the recovered sub_19888/sub_1980C path INSTEAD of the generic
 * attack_body continuation, preserving the original early return. The
 * special tag-5 record is selected from ROM by the supplied resolver.
 * Optional normal_profile_table activates the CPU strength prelude,
 * 40-byte profile lookup and guard-route classification before the
 * mandatory normal attack_body continuation. Host float math is semantic,
 * not claimed bit-for-bit accurate to the original sqrt coprocessor.
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
