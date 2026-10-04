#include "attack_hit_normal_reaction.h"

#include <stddef.h>
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

static int32_t min_i32(int32_t a, int32_t b)
{
    return a < b ? a : b;
}

static int32_t max_i32(int32_t a, int32_t b)
{
    return a > b ? a : b;
}

bool stf_attack_hit_normal_reaction_apply_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint32_t damage,
    uint32_t hit_mode,
    uint8_t hit_flags_50fe03,
    uint32_t motion_g0,
    stf_normal_reaction_result *result
)
{
    stf_normal_reaction_result local;
    int32_t stun = 0;
    int32_t cap = 0;
    int32_t reaction = 0;
    uint8_t explicit_stun = 0u;

    if (attacker == NULL || defender == NULL || result == NULL ||
        attacker_size < STF_NORMAL_REACTION_ATTACKER_MIN_SIZE ||
        defender_size < STF_NORMAL_REACTION_DEFENDER_MIN_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));

    local.defender_198 = UINT32_C(0x0B000000) + motion_g0;
    local.requires_sub_2b94c = true;
    write_le32(defender + 0x198u, local.defender_198);

    explicit_stun =
        hit_mode == UINT32_C(3) ? attacker[0x85Eu] : attacker[0x85Du];
    local.used_mode_specific_85e = hit_mode == UINT32_C(3);

    if (explicit_stun != 0u) {
        stun = explicit_stun;
    } else {
        stun = (int32_t)((damage * UINT32_C(4)) / UINT32_C(5)) + 6;

        if ((hit_flags_50fe03 & (UINT8_C(1) << 3u)) != 0u) {
            const int32_t floor_value =
                (int16_t)read_le16(attacker + 0x80Cu) -
                (int16_t)read_le16(attacker + 0x808u);
            stun = max_i32(stun, floor_value);
            local.used_hit_flag_floor = true;
        }
    }

    cap =
        (int16_t)read_le16(attacker + 0x80Cu) -
        (int16_t)read_le16(attacker + 0x1AAu) +
        6;
    stun = min_i32(stun, cap);

    if (bit32(read_le32(defender + 0x1A4u), 16u)) {
        stun = ((int32_t)(damage >> 1u) - 7) * 2;
        local.used_defender_bit16_override = true;
    }

    local.defender_5de = (int16_t)stun;
    write_le16(defender + 0x5DEu, (uint16_t)local.defender_5de);

    if (hit_mode == UINT32_C(34) || hit_mode == UINT32_C(3)) {
        reaction = stun - 4;
    } else {
        reaction = max_i32(
            (int16_t)read_le16(attacker + 0x808u),
            (int16_t)read_le16(attacker + 0x1AAu) - 6
        );
        reaction = min_i32(reaction, 50);
    }

    local.reaction_argument = reaction + 1;
    *result = local;
    return true;
}
