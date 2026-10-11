#include "collision_combat_runtime.h"

#include <string.h>

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
        model->workspace_size < STF_COLLISION_ATTACK_WORKSPACE_MODEL2_MIN_SIZE ||
        model->fighter_sizes[0] < STF_DAMAGE_UNIT_DEFENDER_MIN_SIZE ||
        model->fighter_sizes[1] < STF_DAMAGE_UNIT_DEFENDER_MIN_SIZE ||
        model->enemy_slot_sizes[0] < STF_ATTACK_HIT_ENEMY_MODEL2_MIN_SIZE ||
        model->enemy_slot_sizes[1] < STF_ATTACK_HIT_ENEMY_MODEL2_MIN_SIZE ||
        model->fighters[0][4u] != 0u || model->fighters[1][4u] != 1u) {
        return false;
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
        } else if (!model->attack_body(
                model->attack_body_user_data,
                attacker_index, defender_index,
                attacker, attacker_size, defender, defender_size,
                model->workspace, model->workspace_size, &pair->prefix)) {
            *result = local;
            return false;
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
