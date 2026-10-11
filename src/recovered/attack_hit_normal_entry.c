#include "attack_hit_normal_entry.h"

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

bool stf_attack_hit_normal_entry_apply_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    const uint8_t *defender,
    size_t defender_size,
    uint8_t *workspace,
    size_t workspace_size,
    const stf_attack_hit_prefix_result *prefix,
    const uint8_t *profile_table,
    size_t profile_table_size,
    stf_attack_hit_normal_entry_result *result
)
{
    stf_attack_hit_normal_entry_result local;
    stf_attack_guard_inputs inputs;

    if (attacker == NULL || defender == NULL || workspace == NULL ||
        prefix == NULL || profile_table == NULL || result == NULL ||
        attacker_size < STF_ATTACK_HIT_NORMAL_ENTRY_ATTACKER_MIN_SIZE ||
        defender_size < STF_ATTACK_HIT_NORMAL_ENTRY_DEFENDER_MIN_SIZE ||
        workspace_size < STF_ATTACK_HIT_NORMAL_ENTRY_WORKSPACE_MIN_SIZE ||
        prefix->special_opponent_path) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    if (!stf_attack_hit_profile_decode(
            profile_table, profile_table_size,
            (uint8_t)prefix->hit_kind_843, &local.profile)) {
        return false;
    }
    inputs.opponent_flags_1a4 = read32(defender + 0x1A4u);
    inputs.opponent_flags_5b8 = read32(defender + 0x5B8u);
    inputs.attacker_flags_000 = read32(attacker);
    inputs.attacker_field_c7c = (int16_t)read16(attacker + 0xC7Cu);
    inputs.opponent_field_c70 = (int32_t)read32(defender + 0xC70u);
    inputs.hit_flags_50fe00 = (uint16_t)prefix->hit_flags_828;
    if (!stf_attack_hit_guard_classify(&inputs, &local.guard_branch)) {
        return false;
    }
    /* strength_prepare is the only writer: workspace +0x26C bit 0.
       All ROM and guard predicate checks have already passed. */
    if (!stf_attack_hit_strength_prepare_model2(
            attacker, attacker_size, workspace, workspace_size,
            &local.strength)) {
        return false;
    }
    if (!stf_attack_hit_strength_scale_bits(
            local.strength.raw_822_float_bits,
            local.profile.strength_scale_bits,
            &local.host_strength_scale_bits)) {
        return false;
    }
    local.host_strength_is_approximate = true;
    *result = local;
    return true;
}
