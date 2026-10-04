#include "attack_hit_down_reaction_runtime.h"

#include <string.h>

static uint16_t read_le16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8u));
}

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8u) |
           ((uint32_t)data[2] << 16u) |
           ((uint32_t)data[3] << 24u);
}

static bool select_motion(
    const uint8_t *attacker,
    uint8_t *defender,
    uint16_t hit_flags_50fe00,
    uint32_t motion_group,
    stf_hit_motion_table_resolver resolver,
    void *resolver_user_data,
    stf_hit_motion_select_result *motion
)
{
    stf_hit_motion_select_inputs in;

    memset(&in, 0, sizeof(in));
    in.hit_flags_50fe00 = hit_flags_50fe00;
    in.motion_group = motion_group;
    in.attacker_flags_0 = read_le32(attacker + 0x000u);
    in.defender_flags_0 = read_le32(defender + 0x000u);
    in.defender_flags_1a4 = read_le32(defender + 0x1A4u);
    in.attacker_82a = (int16_t)read_le16(attacker + 0x82Au);
    in.attacker_26 = (int16_t)read_le16(attacker + 0x026u);
    in.defender_5b4 = (int16_t)read_le16(defender + 0x5B4u);
    in.defender_1f8_bits = read_le32(defender + 0x1F8u);

    return stf_attack_hit_motion_select(
        &in, resolver, resolver_user_data, motion
    );
}

bool stf_attack_hit_special_bit16_reaction_apply_resolved_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint16_t hit_flags_50fe00,
    uint32_t damage,
    stf_hit_motion_table_resolver resolver,
    void *resolver_user_data,
    stf_down_reaction_runtime_result *result
)
{
    stf_down_reaction_runtime_result local;

    if (attacker == NULL || defender == NULL || resolver == NULL ||
        result == NULL ||
        attacker_size < UINT32_C(0x82C) ||
        defender_size < STF_DOWN_REACTION_DEFENDER_MIN_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    if (!select_motion(
            attacker, defender, hit_flags_50fe00, UINT32_C(5),
            resolver, resolver_user_data, &local.motion
        ) ||
        !stf_attack_hit_special_bit16_reaction_apply_model2(
            defender, defender_size, damage, local.motion.motion,
            &local.reaction
        )) {
        return false;
    }

    local.reaction.requires_sub_2b94c = false;
    *result = local;
    return true;
}


bool stf_attack_hit_generic_down_motion_apply_resolved_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint16_t hit_flags_50fe00,
    uint8_t hit_kind_50fe02,
    uint32_t damage,
    bool increment_down_combo,
    stf_hit_motion_table_resolver resolver,
    void *resolver_user_data,
    const stf_motion_prefix_inputs *prefix_inputs,
    const stf_motion_hit_rom_view *rom_view,
    const stf_motion_fallback_profile *fallback_profile,
    const stf_motion_vector_inputs *vector_inputs,
    stf_down_motion_runtime_result *result
)
{
    stf_down_motion_runtime_result local;

    if (prefix_inputs == NULL || rom_view == NULL ||
        fallback_profile == NULL || vector_inputs == NULL ||
        result == NULL) {
        return false;
    }

    memset(&local, 0, sizeof(local));

    if (!stf_attack_hit_generic_down_reaction_apply_resolved_model2(
            attacker,
            attacker_size,
            defender,
            defender_size,
            hit_flags_50fe00,
            hit_kind_50fe02,
            damage,
            increment_down_combo,
            resolver,
            resolver_user_data,
            &local.down
        ) ||
        !local.down.reaction.requires_calc_mht ||
        !stf_attack_hit_motion_prefix_resolve_rom(
            local.down.reaction.selected_motion,
            prefix_inputs,
            rom_view,
            fallback_profile,
            &local.prefix
        ) ||
        !stf_attack_hit_motion_vector_compute(
            &local.prefix.prefix,
            vector_inputs,
            &local.vector
        )) {
        return false;
    }

    local.down.reaction.requires_calc_mht = false;
    *result = local;
    return true;
}

bool stf_attack_hit_generic_down_reaction_apply_resolved_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint16_t hit_flags_50fe00,
    uint8_t hit_kind_50fe02,
    uint32_t damage,
    bool increment_down_combo,
    stf_hit_motion_table_resolver resolver,
    void *resolver_user_data,
    stf_down_reaction_runtime_result *result
)
{
    stf_down_reaction_runtime_result local;
    uint32_t selected = 0u;
    uint32_t flags = 0u;

    if (attacker == NULL || defender == NULL || resolver == NULL ||
        result == NULL ||
        attacker_size < UINT32_C(0x82C) ||
        defender_size < STF_DOWN_REACTION_DEFENDER_MIN_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    flags = read_le32(defender + 0x1A4u);

    if ((flags & (UINT32_C(1) << 4u)) == 0u) {
        if (!select_motion(
                attacker, defender, hit_flags_50fe00, UINT32_C(4),
                resolver, resolver_user_data, &local.motion
            )) {
            return false;
        }
        selected = local.motion.motion;
    }

    if (!stf_attack_hit_generic_down_reaction_apply_model2(
            defender, defender_size, damage, hit_kind_50fe02,
            selected, increment_down_combo, &local.reaction
        )) {
        return false;
    }

    if ((flags & (UINT32_C(1) << 4u)) == 0u) {
        local.reaction.requires_sub_2b94c = false;
    }

    *result = local;
    return true;
}
