#include "collision_combat_runtime.h"

#include <string.h>

static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) |
           ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}

static bool valid_order(const stf_collision_hit_order *order)
{
    uint8_t i;

    if (order == NULL || order->count > 2u) {
        return false;
    }
    for (i = 0u; i < order->count; ++i) {
        if (order->attacker[i] > 1u) {
            return false;
        }
    }
    if (order->count == 2u &&
        (order->attacker[0] != 1u || order->attacker[1] != 0u)) {
        return false;
    }
    return true;
}

bool stf_collision_combat_apply_model2(
    const stf_collision_combat_model2 *model,
    const stf_collision_hit_order *order,
    stf_collision_combat_result *result
)
{
    stf_collision_combat_result local;
    uint8_t i;

    if (model == NULL || result == NULL || !valid_order(order) ||
        model->fighters[0] == NULL || model->fighters[1] == NULL ||
        model->enemy_slots[0] == NULL || model->enemy_slots[1] == NULL ||
        model->workspace == NULL || model->effect == NULL ||
        model->attack_body == NULL ||
        (model->guard_tail != NULL && model->normal_profile_table == NULL) ||
        (model->guard_common_tail != NULL &&
         model->normal_profile_table == NULL) ||
        (model->normal_exit_tail != NULL &&
         model->normal_profile_table == NULL) ||
        (model->recover_normal_stance &&
         model->normal_exit_tail == NULL) ||
        (model->recover_guard_skill &&
         (model->guard_common_tail == NULL || model->total_skill == NULL)) ||
        (model->guard_vector_tgp != NULL && model->guard_tail == NULL) ||
        (model->normal_profile_table != NULL &&
         model->normal_profile_table_size < STF_ATTACK_HIT_PROFILE_RECORD_SIZE) ||
        model->workspace_size < STF_COLLISION_ATTACK_WORKSPACE_MODEL2_MIN_SIZE ||
        model->fighter_sizes[0] < STF_DAMAGE_UNIT_DEFENDER_MIN_SIZE ||
        model->fighter_sizes[1] < STF_DAMAGE_UNIT_DEFENDER_MIN_SIZE ||
        model->enemy_slot_sizes[0] < STF_ATTACK_HIT_ENEMY_MODEL2_MIN_SIZE ||
        model->enemy_slot_sizes[1] < STF_ATTACK_HIT_ENEMY_MODEL2_MIN_SIZE ||
        (model->guard_common_tail != NULL &&
         (model->enemy_slot_sizes[0] < STF_GUARD_COMMON_ENEMY_MIN_SIZE ||
          model->enemy_slot_sizes[1] < STF_GUARD_COMMON_ENEMY_MIN_SIZE)) ||
        model->fighters[0][4u] != 0u || model->fighters[1][4u] != 1u) {
        return false;
    }

    /* Validate both fighter bindings before the first non-rollback write.
     * The normal cleared-bit29 path always selects four kamae motions. */
    if (model->recover_normal_stance) {
        for (i = 0u; i < 2u; ++i) {
            const stf_collision_stance_binding *binding =
                &model->normal_stance[i];
            if (binding->selector_block == NULL ||
                binding->selector_block_size <
                    STF_SET_KAMAE_FULL_SELECTOR_MIN_SIZE ||
                binding->stance_ram == NULL ||
                binding->stance_ram_size <
                    0x5A0u + STF_GET_KAMAE_OUTPUT_BYTES ||
                binding->resolver == NULL) {
                return false;
            }
        }
    }

    memset(&local, 0, sizeof(local));
    for (i = 0u; i < order->count; ++i) {
        const uint8_t attacker_index = order->attacker[i];
        const uint8_t defender_index = (uint8_t)(1u - attacker_index);
        stf_collision_combat_pair_result *pair = &local.pairs[i];
        uint8_t *attacker = model->fighters[attacker_index];
        uint8_t *defender = model->fighters[defender_index];
        const size_t attacker_size = model->fighter_sizes[attacker_index];
        const size_t defender_size = model->fighter_sizes[defender_index];

        pair->attacker_index = attacker_index;
        pair->defender_index = defender_index;
        ++local.attempted;

        if (!stf_attack_hit_prefix_apply_model2(
                attacker, attacker_size,
                defender, defender_size,
                model->workspace, model->workspace_size,
                model->enemy_slots[attacker_index],
                model->enemy_slot_sizes[attacker_index],
                &pair->prefix)) {
            *result = local;
            return false;
        }
        pair->prefix_applied = true;

        if (pair->prefix.special_opponent_path &&
            model->special_record != NULL) {
            const uint8_t *record = NULL;
            size_t record_size = 0u;
            const uint16_t motion_id =
                (uint16_t)((uint16_t)defender[0x19Cu] |
                           ((uint16_t)defender[0x19Du] << 8u));
            if (!model->special_record(
                    model->special_record_user_data, motion_id,
                    &record, &record_size) ||
                !stf_attack_hit_special_apply_model2(
                    attacker, attacker_size, defender, defender_size,
                    record, record_size, model->special_tgp,
                    model->special_tgp_user_data, &pair->special)) {
                *result = local;
                return false;
            }
            pair->special_applied = true;
        } else {
            if (model->normal_profile_table != NULL) {
                if (!stf_attack_hit_normal_entry_apply_model2(
                        attacker, attacker_size, defender, defender_size,
                        model->workspace, model->workspace_size,
                        &pair->prefix, model->normal_profile_table,
                        model->normal_profile_table_size,
                        &pair->normal_entry)) {
                    *result = local;
                    return false;
                }
                pair->normal_entry_applied = true;
            }
            if (model->guard_tail != NULL &&
                pair->normal_entry_applied &&
                (pair->normal_entry.guard_branch ==
                     STF_ATTACK_GUARD_BLOCK_A_2AA70 ||
                 pair->normal_entry.guard_branch ==
                     STF_ATTACK_GUARD_BLOCK_B_2AB54)) {
                const stf_guard_block_kind kind =
                    pair->normal_entry.guard_branch ==
                        STF_ATTACK_GUARD_BLOCK_A_2AA70
                        ? STF_GUARD_BLOCK_A : STF_GUARD_BLOCK_B;
                if (!stf_attack_hit_guard_apply_resolved_model2(
                        kind, attacker, attacker_size,
                        defender, defender_size,
                        model->workspace, model->workspace_size,
                        (uint16_t)pair->prefix.hit_flags_828,
                        model->guard_motion_resolver,
                        model->guard_motion_resolver_user_data,
                        &pair->guard_state)) {
                    *result = local;
                    return false;
                }
                pair->guard_applied = true;
                if (model->guard_vector_tgp != NULL) {
                    if (!stf_attack_hit_guard_vector_apply_model2(
                            attacker, attacker_size,
                            defender, defender_size,
                            pair->normal_entry.strength.combined_82a_026,
                            pair->normal_entry.host_strength_scale_bits,
                            (uint8_t)pair->prefix.hit_kind_843,
                            model->guard_vector_boost_table,
                            model->guard_vector_boost_table_size,
                            model->guard_vector_tgp,
                            model->guard_vector_tgp_user_data,
                            &pair->guard_vector)) {
                        *result = local;
                        return false;
                    }
                    pair->guard_vector_applied = true;
                }
                if (!model->guard_tail(
                        model->guard_tail_user_data,
                        attacker_index, defender_index,
                        attacker, attacker_size, defender, defender_size,
                        model->workspace, model->workspace_size,
                        &pair->prefix, &pair->normal_entry,
                        &pair->guard_state)) {
                    *result = local;
                    return false;
                }
                pair->guard_tail_completed = true;
            } else if (model->guard_common_tail != NULL &&
                       pair->normal_entry_applied &&
                       pair->normal_entry.guard_branch ==
                           STF_ATTACK_GUARD_BRANCH_2AC74) {
                /* ROM loc_2AC74 tests for early abort BEFORE sub_2B94C
                 * resolves any animation. Do not query ROM in that case. */
                stf_attack_side_exit_result exit_result;
                if (!stf_attack_hit_side_exit_apply_model2(
                        STF_ATTACK_SIDE_EXIT_GUARD_2AC74,
                        attacker, attacker_size,
                        defender, defender_size, &exit_result)) {
                    *result = local;
                    return false;
                }
                if (exit_result.path ==
                        STF_ATTACK_SIDE_EXIT_ABORT_2B8C8) {
                    if (!stf_attack_hit_abort_cleanup_apply_model2(
                            attacker, attacker_size,
                            model->enemy_slots[0], model->enemy_slot_sizes[0],
                            model->enemy_slots[1], model->enemy_slot_sizes[1],
                            &pair->guard_common_cleanup)) {
                        *result = local;
                        return false;
                    }
                    pair->guard_common_aborted = true;
                } else {
                    if (!stf_attack_hit_guard_common_apply_resolved_model2(
                            attacker, attacker_size,
                            defender, defender_size,
                            model->workspace, model->workspace_size,
                            model->enemy_slots[attacker_index],
                            model->enemy_slot_sizes[attacker_index],
                            (uint16_t)pair->prefix.hit_flags_828,
                            (uint8_t)pair->prefix.hit_kind_83e,
                            model->guard_common_limit,
                            model->guard_motion_resolver,
                            model->guard_motion_resolver_user_data,
                            &pair->guard_common)) {
                        *result = local;
                        return false;
                    }
                    pair->guard_common_applied = true;
                    if (!pair->guard_common.guard.skipped &&
                        model->recover_guard_skill) {
                        if (!stf_total_skill_add(
                                model->skill_rank_mode,
                                model->skill_select_flags[0],
                                model->skill_select_flags[1],
                                attacker[4u],
                                pair->guard_common.guard.skill_amount,
                                *model->total_skill, &pair->guard_skill)) {
                            *result = local;
                            return false;
                        }
                        pair->guard_skill_processed = true;
                        if (pair->guard_skill.applied) {
                            *model->total_skill =
                                pair->guard_skill.total_skill_after;
                        }
                    }
                    if (pair->guard_common.guard.skipped) {
                        if (!stf_attack_hit_abort_cleanup_apply_model2(
                                attacker, attacker_size,
                                model->enemy_slots[0],
                                model->enemy_slot_sizes[0],
                                model->enemy_slots[1],
                                model->enemy_slot_sizes[1],
                                &pair->guard_common_cleanup)) {
                            *result = local;
                            return false;
                        }
                        pair->guard_common_aborted = true;
                    } else {
                        if (!model->guard_common_tail(
                                model->guard_common_tail_user_data,
                                attacker_index, defender_index,
                                attacker, attacker_size,
                                defender, defender_size,
                                model->workspace, model->workspace_size,
                                &pair->prefix, &pair->normal_entry,
                                &pair->guard_common)) {
                            *result = local;
                            return false;
                        }
                        pair->guard_common_tail_completed = true;
                    }
                }
            } else if (model->normal_exit_tail != NULL &&
                       pair->normal_entry_applied &&
                       pair->normal_entry.guard_branch ==
                           STF_ATTACK_GUARD_EXIT) {
                /* The former generic continuation now exposes the
                 * 0x2AE40 -> 0x2B8C8 abort and defender bit29 handling. */
                if (!stf_attack_hit_side_exit_apply_model2(
                        STF_ATTACK_SIDE_EXIT_NORMAL_2AE40,
                        attacker, attacker_size,
                        defender, defender_size, &pair->normal_exit)) {
                    *result = local;
                    return false;
                }
                pair->normal_exit_applied = true;
                if (pair->normal_exit.path ==
                        STF_ATTACK_SIDE_EXIT_ABORT_2B8C8) {
                    if (!stf_attack_hit_abort_cleanup_apply_model2(
                            attacker, attacker_size,
                            model->enemy_slots[0], model->enemy_slot_sizes[0],
                            model->enemy_slots[1], model->enemy_slot_sizes[1],
                            &pair->normal_exit_cleanup)) {
                        *result = local;
                        return false;
                    }
                    pair->normal_exit_aborted = true;
                } else {
                    /* Consume bit29 stance refresh in recovered C when
                     * explicitly enabled. An unsuccessful ROM resolver
                     * leaves the earlier side effects visible but no damage. */
                    if (model->recover_normal_stance &&
                        pair->normal_exit.request_set_kamae) {
                        const stf_collision_stance_binding *binding =
                            &model->normal_stance[defender_index];
                        if (!stf_attack_hit_apply_stance_event(
                                &pair->normal_exit,
                                read_le32(defender),
                                binding->selector_block,
                                binding->selector_block_size,
                                binding->arithmetic_control,
                                binding->stance_ram,
                                binding->stance_ram_size,
                                binding->resolver,
                                binding->resolver_user_data,
                                &pair->normal_stance)) {
                            *result = local;
                            return false;
                        }
                        pair->normal_stance_applied =
                            pair->normal_stance.executed;
                    }
                    /* With recovered stance disabled, the callback must
                     * handle request_set_kamae itself. In either mode the
                     * callback retains the other unported attack-hit work. */
                    if (!model->normal_exit_tail(
                            model->normal_exit_tail_user_data,
                            attacker_index, defender_index,
                            attacker, attacker_size,
                            defender, defender_size,
                            model->workspace, model->workspace_size,
                            &pair->prefix, &pair->normal_entry,
                            &pair->normal_exit)) {
                        *result = local;
                        return false;
                    }
                    pair->normal_exit_tail_completed = true;
                }
            } else if (!model->attack_body(
                    model->attack_body_user_data,
                    attacker_index, defender_index,
                    attacker, attacker_size, defender, defender_size,
                    model->workspace, model->workspace_size, &pair->prefix)) {
                *result = local;
                return false;
            }
        }
        pair->body_completed = true;

        if (!stf_damage_unit_runtime_apply_model2(
                attacker, attacker_size,
                defender, defender_size,
                model->workspace, model->workspace_size,
                model->num_rounds_to_win, model->effect,
                model->also_mode, model->also_sub_mode,
                &pair->damage)) {
            *result = local;
            return false;
        }
        pair->damage_completed = true;
        ++local.completed;
    }

    *result = local;
    return true;
}
