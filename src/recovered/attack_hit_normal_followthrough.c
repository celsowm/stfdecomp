#include "attack_hit_normal_followthrough.h"

#include <string.h>

static uint16_t read16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8u));
}

static uint32_t read32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) |
           ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}

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
)
{
    stf_attack_hit_normal_followthrough_result local;
    stf_attack_reaction_inputs reaction;

    if (attacker == NULL || defender == NULL || workspace == NULL ||
        side_exit == NULL || result == NULL || motion_resolver == NULL ||
        side_exit->path != STF_ATTACK_SIDE_EXIT_CONTINUE ||
        attacker_size < STF_ATTACK_HIT_STATE_PRELUDE_MIN_SIZE ||
        defender_size < STF_ATTACK_HIT_COMBO_DEFENDER_MIN_SIZE ||
        workspace_size < STF_ATTACK_HIT_COMBO_WORKSPACE_MIN_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    if (!stf_attack_hit_combo_after_side_exit_apply_model2(
            attacker, attacker_size, defender, defender_size,
            workspace, workspace_size, side_exit, hit_kind_50fe02,
            &local.combo)) {
        *result = local;
        return false;
    }
    local.combo_applied = true;

    if (!stf_attack_hit_state_prelude_apply_model2(
            attacker, attacker_size, &local.state_prelude)) {
        *result = local;
        return false;
    }
    local.state_prelude_applied = true;

    if (!stf_attack_hit_damage_apply_model2(
            attacker, attacker_size, defender, defender_size,
            initial_damage, &local.damage)) {
        *result = local;
        return false;
    }
    local.damage_transform_applied = true;

    if (!stf_attack_hit_finish_apply_model2(
            attacker, attacker_size, defender, defender_size,
            energy_max, disable_energy_scaling,
            local.damage.damage, &local.finish)) {
        *result = local;
        return false;
    }
    local.finish_applied = true;

    /* The ah_not_finish_blow reaction classifier is not used on KOs. */
    if (!local.finish.finish_blow) {
        memset(&reaction, 0, sizeof(reaction));
        reaction.defender_flags_1a4 = read32(defender + 0x1A4u);
        reaction.defender_flags_70c = read32(defender + 0x70Cu);
        reaction.hit_flags_50fe00 = hit_flags_50fe00;
        reaction.damage = local.damage.damage;
        reaction.attacker_kind_821 = attacker[0x821u];
        reaction.scaled_damage = local.finish.scaled_damage;
        reaction.defender_energy_1ac =
            (int16_t)read16(defender + 0x1ACu);

        if (!stf_attack_hit_reaction_classify(
                &reaction, &local.reaction_path)) {
            *result = local;
            return false;
        }
        local.reaction_classified = true;

        if (local.reaction_path == STF_ATTACK_REACTION_NORMAL) {
            if (!stf_attack_hit_normal_reaction_apply_resolved_model2(
                    attacker, attacker_size, defender, defender_size,
                    hit_flags_50fe00, hit_flags_50fe03,
                    local.damage.damage, local.damage.hit_mode,
                    motion_resolver, motion_resolver_user_data,
                    &local.normal_reaction)) {
                *result = local;
                return false;
            }
            local.normal_reaction_applied = true;
        } else {
            bool use_special = local.reaction_path ==
                STF_ATTACK_REACTION_SPECIAL_BIT16;
            bool increment_combo = local.reaction_path ==
                STF_ATTACK_REACTION_DOWN_COMBO;

            if (increment_combo) {
                if (!stf_attack_hit_down_route(
                        read32(attacker + 0x1A4u),
                        read32(defender + 0x1A4u),
                        &local.down_route)) {
                    *result = local;
                    return false;
                }
                use_special = local.down_route ==
                    STF_DOWN_REACTION_SPECIAL_BIT16;
            } else {
                local.down_route = use_special
                    ? STF_DOWN_REACTION_SPECIAL_BIT16
                    : STF_DOWN_REACTION_GENERIC;
            }

            if (use_special) {
                if (!stf_attack_hit_special_bit16_reaction_apply_resolved_model2(
                        attacker, attacker_size, defender, defender_size,
                        hit_flags_50fe00, local.damage.damage,
                        motion_resolver, motion_resolver_user_data,
                        &local.down_reaction)) {
                    *result = local;
                    return false;
                }
            } else {
                if (!stf_attack_hit_generic_down_reaction_apply_resolved_model2(
                        attacker, attacker_size, defender, defender_size,
                        hit_flags_50fe00, hit_kind_50fe02,
                        local.damage.damage, increment_combo,
                        motion_resolver, motion_resolver_user_data,
                        &local.down_reaction)) {
                    *result = local;
                    return false;
                }
            }
            local.down_reaction_applied = true;
        }
    }

    *result = local;
    return true;
}
