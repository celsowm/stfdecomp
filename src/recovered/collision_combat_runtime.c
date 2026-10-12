#include "collision_combat_runtime.h"

#include <math.h>
#include <string.h>

static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) |
           ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}

static void write_le32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
    p[2] = (uint8_t)(value >> 16u);
    p[3] = (uint8_t)(value >> 24u);
}

typedef struct stf_normal_sqrt_tgp_context {
    stf_attack_hit_guard_vector_tgp_fn handler;
    void *handler_user_data;
    uint8_t *completed;
} stf_normal_sqrt_tgp_context;

static bool query_normal_sqrt_tgp(
    uint32_t input_bits, uint32_t *reply_bits, void *user_data
)
{
    stf_normal_sqrt_tgp_context *ctx =
        (stf_normal_sqrt_tgp_context *)user_data;
    stf_attack_hit_guard_vector_query query;

    if (ctx == NULL || ctx->handler == NULL ||
        ctx->completed == NULL || reply_bits == NULL) {
        return false;
    }
    memset(&query, 0, sizeof(query));
    query.opcode = UINT32_C(0x0D001A1A);
    query.argument_count = 1u;
    query.words[0] = input_bits;
    if (!ctx->handler(ctx->handler_user_data, &query, reply_bits)) {
        return false;
    }
    /* Count only validated replies. In particular a NaN/Inf or negative
     * sqrt response is not a completed Model 2 command result. */
    {
        float value = 0.0f;
        memcpy(&value, reply_bits, sizeof(value));
        if (!isfinite(value) || value < 0.0f) {
            return false;
        }
    }
    ++*ctx->completed;
    return true;
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
        (model->recover_normal_hit &&
         (model->normal_exit_tail == NULL ||
          model->normal_profile_table == NULL ||
          model->normal_hit_motion_resolver == NULL)) ||
        (model->recover_normal_knockback &&
         (!model->recover_normal_hit ||
          model->normal_knockback_rom == NULL ||
          model->normal_knockback_rom->image == NULL ||
          model->normal_knockback_rom->animation_count == 0u ||
          model->normal_knockback_rom->animation_related_address <
              model->normal_knockback_rom->base_address)) ||
        (model->normal_knockback_tgp != NULL &&
         !model->recover_normal_knockback) ||
        (model->recover_normal_damage &&
         (!model->recover_normal_hit ||
          model->normal_damage_event == NULL)) ||
        (model->recover_normal_ring_scatter &&
         (!model->recover_normal_damage ||
          model->normal_ring_pool == NULL ||
          model->normal_ring_slots == NULL)) ||
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
                    if (model->recover_normal_hit) {
                        pair->normal_hit_started = true;
                        if (!stf_attack_hit_normal_followthrough_apply_model2(
                                attacker, attacker_size,
                                defender, defender_size,
                                model->workspace, model->workspace_size,
                                &pair->normal_exit,
                                (uint8_t)pair->prefix.hit_kind_843,
                                (uint16_t)pair->prefix.hit_flags_828,
                                (uint8_t)pair->prefix.hit_kind_83e,
                                model->normal_initial_damage[attacker_index],
                                model->normal_energy_max,
                                model->normal_disable_energy_scaling,
                                model->normal_hit_motion_resolver,
                                model->normal_hit_motion_resolver_user_data,
                                &pair->normal_hit)) {
                            *result = local;
                            return false;
                        }
                    }
                    /* The normal followthrough already selected the
                     * down motion and updated defender reaction state.
                     * Do NOT replay that reaction: only resolve tag 0x11
                     * and apply the remaining shared knockback corridor.
                     * Other branches retain their existing tail behavior. */
                    if (model->recover_normal_knockback &&
                        pair->normal_hit.down_reaction_applied &&
                        pair->normal_hit.down_route ==
                            STF_DOWN_REACTION_GENERIC &&
                        pair->normal_hit.down_reaction.reaction.requires_calc_mht) {
                        stf_motion_prefix_inputs motion_inputs =
                            model->normal_knockback_context[attacker_index];
                        stf_motion_vector_inputs vector_inputs;
                        uint32_t selected_motion =
                            pair->normal_hit.down_reaction.reaction.selected_motion;

                        pair->normal_knockback_started = true;
                        motion_inputs.defender_flags_1a4 =
                            read_le32(defender + 0x1A4u);
                        motion_inputs.hit_mode =
                            pair->normal_hit.damage.hit_mode;
                        motion_inputs.defender_combo_6f5 =
                            defender[0x6F5u];
                        motion_inputs.defender_5d8_bits =
                            read_le32(defender + 0x5D8u);

                        if (model->normal_knockback_tgp != NULL) {
                            stf_normal_sqrt_tgp_context sqrt_context;
                            sqrt_context.handler = model->normal_knockback_tgp;
                            sqrt_context.handler_user_data =
                                model->normal_knockback_tgp_user_data;
                            sqrt_context.completed =
                                &pair->normal_knockback_tgp_completed;
                            if (!stf_attack_hit_motion_prefix_resolve_rom_with_sqrt(
                                    selected_motion, &motion_inputs,
                                    model->normal_knockback_rom,
                                    &pair->normal_entry.profile.fallback,
                                    query_normal_sqrt_tgp, &sqrt_context,
                                    &pair->normal_knockback_motion)) {
                                *result = local;
                                return false;
                            }
                        } else if (!stf_attack_hit_motion_prefix_resolve_rom(
                                selected_motion, &motion_inputs,
                                model->normal_knockback_rom,
                                &pair->normal_entry.profile.fallback,
                                &pair->normal_knockback_motion)) {
                            *result = local;
                            return false;
                        }
                        pair->normal_knockback_motion_resolved = true;

                        memset(&vector_inputs, 0, sizeof(vector_inputs));
                        vector_inputs.hit_mode = pair->normal_hit.damage.hit_mode;
                        vector_inputs.horizontal_angle_r10 =
                            model->normal_knockback_heading[attacker_index];
                        vector_inputs.attacker_flags_0 = read_le32(attacker);
                        vector_inputs.attacker_7d2 = attacker[0x7D2u];
                        vector_inputs.attacker_70c =
                            read_le32(attacker + 0x70Cu);
                        vector_inputs.defender_flags_1a4 =
                            read_le32(defender + 0x1A4u);
                        vector_inputs.profile_horizontal_scale_bits =
                            pair->normal_entry.profile.horizontal_scale_bits;
                        vector_inputs.profile_vertical_scale_bits =
                            pair->normal_entry.profile.vertical_scale_bits;
                        vector_inputs.attacker_843 =
                            (uint8_t)pair->prefix.hit_kind_843;

                        if (model->normal_knockback_tgp != NULL) {
                            uint8_t completed = 0u;
                            const bool computed =
                                stf_attack_hit_motion_vector_compute_with_tgp(
                                    &pair->normal_knockback_motion.prefix,
                                    &vector_inputs,
                                    model->normal_knockback_tgp,
                                    model->normal_knockback_tgp_user_data,
                                    &completed,
                                    &pair->normal_knockback_vector);
                            pair->normal_knockback_tgp_completed =
                                (uint8_t)(pair->normal_knockback_tgp_completed +
                                          completed);
                            if (!computed) {
                                *result = local;
                                return false;
                            }
                        } else if (!stf_attack_hit_motion_vector_compute(
                                &pair->normal_knockback_motion.prefix,
                                &vector_inputs,
                                &pair->normal_knockback_vector)) {
                            *result = local;
                            return false;
                        }
                        /* Atomic vector commit: a failed ROM lookup or
                         * invalid float must not partially overwrite axes. */
                        write_le32(defender + 0x5E0u,
                            pair->normal_knockback_vector.x_5e0_bits);
                        write_le32(defender + 0x5E4u,
                            pair->normal_knockback_vector.y_5e4_bits);
                        write_le32(defender + 0x5E8u,
                            pair->normal_knockback_vector.z_5e8_bits);
                        pair->normal_knockback_vector_applied = true;
                    }

                    /* The callback retains unrecovered sound, hardware
                     * events and KO work, but must not repeat an opted-in
                     * vector application or its down-reaction dispatch. */
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

                    /* damage_calculation belongs after the normal reaction
                     * and external coprocessor/event work, but before
                     * damage_unit and its post-hit bookkeeping. */
                    if (model->recover_normal_damage) {
                        if (!stf_damage_calculation_apply_model2(
                                defender, defender_size,
                                attacker, attacker_size,
                                model->normal_energy_max,
                                model->normal_disable_energy_scaling,
                                model->normal_game_timer,
                                pair->normal_hit.damage.damage,
                                &pair->normal_damage)) {
                            *result = local;
                            return false;
                        }
                        pair->normal_damage_applied = true;

                        if (model->recover_normal_ring_scatter) {
                            if (!stf_ring_scatter_apply_damage_event_model2(
                                    &pair->normal_damage,
                                    defender, defender_size,
                                    attacker, attacker_size,
                                    &model->normal_ring_inputs[defender_index],
                                    model->normal_ring_pool,
                                    model->normal_ring_slots,
                                    &pair->normal_rings)) {
                                *result = local;
                                return false;
                            }
                            pair->normal_rings_processed = true;
                        }
                        /* Do not fake audio or discard unhandled ring events:
                         * the caller must finish the required event work. */
                        if (!model->normal_damage_event(
                                model->normal_damage_event_user_data,
                                attacker_index, defender_index,
                                &pair->normal_damage,
                                pair->normal_rings_processed,
                                &pair->normal_rings)) {
                            *result = local;
                            return false;
                        }
                        pair->normal_damage_event_completed = true;
                    }
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
