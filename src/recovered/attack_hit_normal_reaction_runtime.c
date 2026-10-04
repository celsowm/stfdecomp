#include "attack_hit_normal_reaction_runtime.h"

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

bool stf_attack_hit_normal_reaction_apply_resolved_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint16_t hit_flags_50fe00,
    uint8_t hit_flags_50fe03,
    uint32_t damage,
    uint32_t hit_mode,
    stf_hit_motion_table_resolver table_resolver,
    void *resolver_user_data,
    stf_normal_reaction_runtime_result *result
)
{
    stf_normal_reaction_runtime_result local;
    stf_hit_motion_select_inputs motion_inputs;

    if (attacker == NULL || defender == NULL || result == NULL ||
        table_resolver == NULL ||
        attacker_size < STF_NORMAL_REACTION_ATTACKER_MIN_SIZE ||
        defender_size < STF_NORMAL_REACTION_DEFENDER_MIN_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    memset(&motion_inputs, 0, sizeof(motion_inputs));

    motion_inputs.hit_flags_50fe00 = hit_flags_50fe00;
    motion_inputs.motion_group = hit_mode & UINT32_C(0x1F);
    motion_inputs.attacker_flags_0 = read_le32(attacker + 0x000u);
    motion_inputs.defender_flags_0 = read_le32(defender + 0x000u);
    motion_inputs.defender_flags_1a4 = read_le32(defender + 0x1A4u);
    motion_inputs.attacker_82a = (int16_t)read_le16(attacker + 0x82Au);
    motion_inputs.attacker_26 = (int16_t)read_le16(attacker + 0x026u);
    motion_inputs.defender_5b4 = (int16_t)read_le16(defender + 0x5B4u);
    motion_inputs.defender_1f8_bits = read_le32(defender + 0x1F8u);

    if (!stf_attack_hit_motion_select(
            &motion_inputs,
            table_resolver,
            resolver_user_data,
            &local.motion
        ) ||
        !stf_attack_hit_normal_reaction_apply_model2(
            attacker,
            attacker_size,
            defender,
            defender_size,
            damage,
            hit_mode,
            hit_flags_50fe03,
            local.motion.motion,
            &local.reaction
        )) {
        return false;
    }

    local.reaction.requires_sub_2b94c = false;
    *result = local;
    return true;
}
