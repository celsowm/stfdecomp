#include <stdint.h>
#include <string.h>

#include "attack_hit_normal_reaction.h"

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

static void seed_common(uint8_t *attacker)
{
    write_le16(attacker + 0x80Cu, UINT16_C(50));
    write_le16(attacker + 0x1AAu, UINT16_C(20));
    write_le16(attacker + 0x808u, UINT16_C(10));
}

int main(void)
{
    uint8_t attacker[STF_NORMAL_REACTION_ATTACKER_MIN_SIZE];
    uint8_t defender[STF_NORMAL_REACTION_DEFENDER_MIN_SIZE];
    stf_normal_reaction_result result;

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    seed_common(attacker);

    if (!stf_attack_hit_normal_reaction_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            UINT32_C(40), UINT32_C(2), 0u, UINT32_C(0x123), &result
        ) ||
        result.defender_198 != UINT32_C(0x0B000123) ||
        result.defender_5de != 36 ||
        result.reaction_argument != 15 ||
        result.used_mode_specific_85e ||
        result.used_hit_flag_floor ||
        result.used_defender_bit16_override ||
        read_le16(defender + 0x5DEu) != UINT16_C(36) ||
        read_le32(defender + 0x198u) != UINT32_C(0x0B000123)) {
        return 1;
    }

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    seed_common(attacker);

    if (!stf_attack_hit_normal_reaction_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            UINT32_C(20), UINT32_C(2), UINT8_C(1) << 3u,
            UINT32_C(0x20), &result
        ) ||
        !result.used_hit_flag_floor ||
        result.defender_5de != 36) {
        return 2;
    }

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    seed_common(attacker);
    attacker[0x85Eu] = UINT8_C(12);

    if (!stf_attack_hit_normal_reaction_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            UINT32_C(40), UINT32_C(3), 0u,
            UINT32_C(0x44), &result
        ) ||
        !result.used_mode_specific_85e ||
        result.defender_5de != 12 ||
        result.reaction_argument != 9) {
        return 3;
    }

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    seed_common(attacker);
    write_le32(defender + 0x1A4u, UINT32_C(1) << 16u);

    if (!stf_attack_hit_normal_reaction_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            UINT32_C(41), UINT32_C(2), 0u,
            UINT32_C(0x55), &result
        ) ||
        !result.used_defender_bit16_override ||
        result.defender_5de != 26 ||
        result.reaction_argument != 15) {
        return 4;
    }

    return 0;
}
