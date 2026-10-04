#include "attack_hit_guard_common_runtime.h"

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

bool stf_attack_hit_guard_common_apply_resolved_model2(
    uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint8_t *workspace,
    size_t workspace_size,
    uint8_t *enemy_slot,
    size_t enemy_slot_size,
    uint16_t hit_flags_50fe00,
    uint8_t hit_flags_50fe03,
    uint8_t guard_limit,
    stf_hit_motion_table_resolver resolver,
    void *resolver_user_data,
    stf_guard_common_runtime_result *result
)
{
    stf_guard_common_runtime_result local;
    stf_hit_motion_select_inputs in;

    if (attacker == NULL || defender == NULL || workspace == NULL ||
        enemy_slot == NULL || resolver == NULL || result == NULL ||
        attacker_size < STF_GUARD_COMMON_ATTACKER_MIN_SIZE ||
        defender_size < STF_GUARD_COMMON_DEFENDER_MIN_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    memset(&in, 0, sizeof(in));

    in.hit_flags_50fe00 = hit_flags_50fe00;
    in.motion_group = UINT32_C(0);
    in.attacker_flags_0 = read_le32(attacker + 0x000u);
    in.defender_flags_0 = read_le32(defender + 0x000u);
    in.defender_flags_1a4 = read_le32(defender + 0x1A4u);
    in.attacker_82a = (int16_t)read_le16(attacker + 0x82Au);
    in.attacker_26 = (int16_t)read_le16(attacker + 0x026u);
    in.defender_5b4 = (int16_t)read_le16(defender + 0x5B4u);
    in.defender_1f8_bits = read_le32(defender + 0x1F8u);

    if (!stf_attack_hit_motion_select(
            &in, resolver, resolver_user_data, &local.motion
        ) ||
        !stf_attack_hit_guard_common_apply_model2(
            attacker, attacker_size,
            defender, defender_size,
            workspace, workspace_size,
            enemy_slot, enemy_slot_size,
            hit_flags_50fe03,
            guard_limit,
            local.motion.motion,
            &local.guard
        )) {
        return false;
    }

    local.guard.requires_sub_2b94c = false;
    *result = local;
    return true;
}
