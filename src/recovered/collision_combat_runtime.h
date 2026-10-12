#ifndef STF_RECOVERED_COLLISION_COMBAT_RUNTIME_H
#define STF_RECOVERED_COLLISION_COMBAT_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attack_hit.h"
#include "attack_hit_special.h"
#include "attack_hit_normal_entry.h"
#include "attack_hit_guard_state_runtime.h"
#include "attack_hit_guard_vector.h"
#include "attack_hit_guard_common_runtime.h"
#include "attack_hit_side_exit.h"
#include "attack_hit_stance.h"
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


/*
 * Continuation after the recovered BLOCK_A/B CPU branch (loc_2B738).
 * Must implement the remaining real coprocessor/vector and event work;
 * merely returning true without the continuation is NOT 1:1 gameplay.
 * The resolved BLOCK_B motion and guard sound request are in guard_state.
 */
typedef bool (*stf_collision_attack_hit_guard_tail_fn)(
    void *user_data,
    uint8_t attacker_index,
    uint8_t defender_index,
    uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint8_t *workspace,
    size_t workspace_size,
    const stf_attack_hit_prefix_result *prefix,
    const stf_attack_hit_normal_entry_result *normal_entry,
    const stf_guard_state_runtime_result *guard_state
);

/*
 * Continuation for the 0x2AC74 common guard path. The CPU-resolved
 * motion, counters, stun state, and audio/skill requests have already
 * been produced by guard_common. This callback remains responsible for
 * the original audio/skill/TGP tail (it MUST NOT be a success stub
 * in a real gameplay port).
 */
typedef bool (*stf_collision_attack_hit_guard_common_tail_fn)(
    void *user_data,
    uint8_t attacker_index,
    uint8_t defender_index,
    uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint8_t *workspace,
    size_t workspace_size,
    const stf_attack_hit_prefix_result *prefix,
    const stf_attack_hit_normal_entry_result *normal_entry,
    const stf_guard_common_runtime_result *guard_common
);

/*
 * Explicit opt-in continuation for non-guard attack_hit at loc_2AE40.
 * Classify original abort/bit29 consumption first; this callback must
 * consume request_set_kamae and implement the remaining original body.
 * A successful callback alone is NOT a recovered full attack_hit.
 */
typedef bool (*stf_collision_attack_hit_normal_exit_tail_fn)(
    void *user_data,
    uint8_t attacker_index,
    uint8_t defender_index,
    uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint8_t *workspace,
    size_t workspace_size,
    const stf_attack_hit_prefix_result *prefix,
    const stf_attack_hit_normal_entry_result *normal_entry,
    const stf_attack_side_exit_result *side_exit
);

/* Caller-owned, ROM-backed stance data for one defender. The two entries in
 * the combat model correspond to fighter slot 0 and fighter slot 1. */
typedef struct stf_collision_stance_binding {
    const uint8_t *selector_block;
    size_t selector_block_size;
    uint8_t *stance_ram;
    size_t stance_ram_size;
    uint32_t arithmetic_control;
    stf_kamae_motion_resolver resolver;
    void *resolver_user_data;
} stf_collision_stance_binding;

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
    /* Opt-in recovered BLOCK_A/B state: requires original profile table.
     * BLOCK_B additionally requires guard_motion_resolver. */
    stf_collision_attack_hit_guard_tail_fn guard_tail;
    void *guard_tail_user_data;
    stf_hit_motion_table_resolver guard_motion_resolver;
    void *guard_motion_resolver_user_data;
    /* Optional 5-command Model 2 guard-vector continuation 0x2B738.
     * Requires guard_tail and normal_profile_table. The guard_tail still
     * handles damage_calculation, events and other unrecovered work. */
    stf_attack_hit_guard_vector_tgp_fn guard_vector_tgp;
    void *guard_vector_tgp_user_data;
    const uint8_t *guard_vector_boost_table;
    size_t guard_vector_boost_table_size;
    /* Optional 0x2AC74 common-guard path, using guard_motion_resolver.
     * Strength/stun effects are computed in C; audio/skill/TGP need tail. */
    stf_collision_attack_hit_guard_common_tail_fn guard_common_tail;
    void *guard_common_tail_user_data;
    uint8_t guard_common_limit;
    /* Optional loc_2AE40 branch after STF_ATTACK_GUARD_EXIT. */
    stf_collision_attack_hit_normal_exit_tail_fn normal_exit_tail;
    void *normal_exit_tail_user_data;

    /* Opt in to set_kamae_ram on continuing 0x2AE40 bit29 paths.
     * Both fighter bindings must be valid before the first prefix write.
     * With this enabled, normal_exit_tail must NOT repeat the stance event.
     * Other attack-hit work still belongs to normal_exit_tail. */
    bool recover_normal_stance;
    stf_collision_stance_binding normal_stance[2];

    /* Optional real guard skill accounting for the 0x2AC74 route.
     * The rank/select flags and counter are supplied by the caller.
     * guard_common_tail must NOT add skill again when enabled. */
    bool recover_guard_skill;
    uint16_t skill_rank_mode;
    uint32_t skill_select_flags[2];
    uint32_t *total_skill;
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
    bool guard_applied;
    bool guard_tail_completed;
    bool guard_vector_applied;
    stf_attack_hit_guard_vector_result guard_vector;
    stf_guard_state_runtime_result guard_state;
    bool guard_common_applied;
    bool guard_common_aborted;
    bool guard_common_tail_completed;
    stf_guard_common_runtime_result guard_common;
    stf_attack_abort_cleanup_result guard_common_cleanup;
    bool guard_skill_processed;
    stf_skill_accounting_result guard_skill;
    bool normal_exit_applied;
    bool normal_exit_aborted;
    bool normal_exit_tail_completed;
    stf_attack_side_exit_result normal_exit;
    stf_attack_abort_cleanup_result normal_exit_cleanup;
    bool normal_stance_applied;
    stf_attack_hit_stance_result normal_stance;
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
 * mandatory normal attack_body continuation. When guard_tail is supplied,
 * BLOCK_A and BLOCK_B execute the recovered guard-state CPU writes in C
 * and invoke that branch-specific tail, not the generic attack_body.
 * BLOCK_B resolves sub_2B94C motion through guard_motion_resolver before
 * modifying defender/attacker state. Other paths retain the generic hook.
 * The optional guard_vector_tgp resolves the five explicit TGP commands
 * at loc_2B738..loc_2B898 before guard_tail. Without it, guard_tail still
 * owns these vector operations. In either case guard_tail MUST handle
 * the remaining damage_calculation/audio/event work for ROM equivalence.
 * The optional guard_common_tail handles the distinct 0x2AC74 branch;
 * its early abort at 0x2B8C8 is handled in C without a motion lookup.
 * The tail must still perform the unrecovered events and device work.
 * The optional recover_normal_stance handles the original set_kamae_ram
 * event before normal_exit_tail, using caller-supplied motion records.
 * The optional recover_guard_skill writes a caller-owned total_skill via
 * rank/select-gated, recovered arithmetic before guard_common_tail.
 * Host float math is semantic,
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
