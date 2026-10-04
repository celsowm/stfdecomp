#include "attack_hit_down_reaction.h"

#include <stddef.h>
#include <string.h>

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8u) |
           ((uint32_t)data[2] << 16u) |
           ((uint32_t)data[3] << 24u);
}

static void write_le16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
}

static void write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

static bool bit32(uint32_t value, unsigned bit)
{
    return (value & (UINT32_C(1) << bit)) != 0u;
}

static float bits_to_float(uint32_t bits)
{
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

bool stf_attack_hit_down_route(
    uint32_t attacker_flags_1a4,
    uint32_t defender_flags_1a4,
    stf_down_reaction_route *route
)
{
    if (route == NULL) {
        return false;
    }

    if (bit32(attacker_flags_1a4, 22u) &&
        !bit32(defender_flags_1a4, 4u) &&
        bit32(defender_flags_1a4, 11u)) {
        *route = STF_DOWN_REACTION_SPECIAL_BIT16;
    } else {
        *route = STF_DOWN_REACTION_GENERIC;
    }
    return true;
}

bool stf_attack_hit_special_bit16_reaction_apply_model2(
    uint8_t *defender,
    size_t defender_size,
    uint32_t damage,
    uint32_t motion_g0,
    stf_down_reaction_result *result
)
{
    stf_down_reaction_result local;
    int32_t stun = 0;

    if (defender == NULL || result == NULL ||
        defender_size < STF_DOWN_REACTION_DEFENDER_MIN_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    local.selected_motion = motion_g0;
    local.defender_198 = UINT32_C(0x08010000) + motion_g0;
    local.requires_sub_2b94c = true;
    write_le32(defender + 0x198u, local.defender_198);

    stun = ((int32_t)(damage >> 1u) - 7) * 2;
    local.defender_5de = (int16_t)stun;
    write_le16(defender + 0x5DEu, (uint16_t)local.defender_5de);

    *result = local;
    return true;
}

bool stf_attack_hit_generic_down_reaction_apply_model2(
    uint8_t *defender,
    size_t defender_size,
    uint32_t damage,
    uint8_t hit_kind_50fe02,
    uint32_t motion_g0,
    bool increment_down_combo,
    stf_down_reaction_result *result
)
{
    stf_down_reaction_result local;
    uint32_t motion = motion_g0;
    uint32_t defender_flags = 0u;
    uint32_t stun = 0u;

    if (defender == NULL || result == NULL ||
        defender_size < STF_DOWN_REACTION_DEFENDER_MIN_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));

    if (increment_down_combo) {
        defender[0x6F5u] = (uint8_t)(defender[0x6F5u] + UINT8_C(1));
    }
    local.defender_down_combo_6f5 = defender[0x6F5u];

    defender_flags = read_le32(defender + 0x1A4u);
    if (bit32(defender_flags, 4u)) {
        motion = UINT32_C(0xE1);
        if (hit_kind_50fe02 == UINT8_C(7)) {
            motion = UINT32_C(0x14E);
        } else if (hit_kind_50fe02 == UINT8_C(4)) {
            motion = UINT32_C(0x10E);
        }
        local.used_motion_override = true;
    } else {
        local.requires_sub_2b94c = true;
    }

    if (motion == UINT32_C(0xE1) &&
        bits_to_float(read_le32(defender + 0x1F8u)) <= 0.9f) {
        motion = UINT32_C(0x106);
        local.used_height_motion_fixup = true;
    }

    local.selected_motion = motion;
    local.defender_198 = UINT32_C(0x08000000) + motion;
    write_le32(defender + 0x198u, local.defender_198);

    stun = (damage >> 1u) * UINT32_C(2);
    local.defender_5de = (int16_t)stun;
    write_le16(defender + 0x5DEu, (uint16_t)local.defender_5de);

    local.requires_calc_mht = true;
    *result = local;
    return true;
}
