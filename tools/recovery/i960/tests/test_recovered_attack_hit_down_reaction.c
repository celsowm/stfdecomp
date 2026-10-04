#include <stdint.h>
#include <string.h>

#include "attack_hit_down_reaction.h"

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

static void write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

int main(void)
{
    uint8_t defender[STF_DOWN_REACTION_DEFENDER_MIN_SIZE];
    stf_down_reaction_result result;
    stf_down_reaction_route route;

    if (!stf_attack_hit_down_route(
            UINT32_C(1) << 22u,
            UINT32_C(1) << 11u,
            &route
        ) ||
        route != STF_DOWN_REACTION_SPECIAL_BIT16) {
        return 1;
    }

    if (!stf_attack_hit_down_route(
            UINT32_C(1) << 22u,
            (UINT32_C(1) << 11u) | (UINT32_C(1) << 4u),
            &route
        ) ||
        route != STF_DOWN_REACTION_GENERIC) {
        return 2;
    }

    memset(defender, 0, sizeof(defender));

    if (!stf_attack_hit_special_bit16_reaction_apply_model2(
            defender, sizeof(defender),
            UINT32_C(40), UINT32_C(0x123), &result
        ) ||
        result.selected_motion != UINT32_C(0x123) ||
        result.defender_198 != UINT32_C(0x08010123) ||
        result.defender_5de != 26 ||
        !result.requires_sub_2b94c ||
        read_le16(defender + 0x5DEu) != UINT16_C(26)) {
        return 3;
    }

    memset(defender, 0, sizeof(defender));
    defender[0x6F5u] = UINT8_C(2);

    if (!stf_attack_hit_generic_down_reaction_apply_model2(
            defender, sizeof(defender),
            UINT32_C(41), UINT8_C(0), UINT32_C(0x44), true, &result
        ) ||
        result.selected_motion != UINT32_C(0x44) ||
        result.defender_198 != UINT32_C(0x08000044) ||
        result.defender_5de != 40 ||
        result.defender_down_combo_6f5 != UINT8_C(3) ||
        !result.requires_sub_2b94c ||
        !result.requires_calc_mht) {
        return 4;
    }

    memset(defender, 0, sizeof(defender));
    write_le32(defender + 0x1A4u, UINT32_C(1) << 4u);

    if (!stf_attack_hit_generic_down_reaction_apply_model2(
            defender, sizeof(defender),
            UINT32_C(30), UINT8_C(7), UINT32_C(0x99), false, &result
        ) ||
        result.selected_motion != UINT32_C(0x14E) ||
        !result.used_motion_override ||
        result.used_height_motion_fixup ||
        result.requires_sub_2b94c) {
        return 5;
    }

    memset(defender, 0, sizeof(defender));
    write_le32(defender + 0x1A4u, UINT32_C(1) << 4u);
    write_le32(defender + 0x1F8u, UINT32_C(0x3F000000)); /* 0.5f */

    if (!stf_attack_hit_generic_down_reaction_apply_model2(
            defender, sizeof(defender),
            UINT32_C(30), UINT8_C(0), UINT32_C(0x99), false, &result
        ) ||
        result.selected_motion != UINT32_C(0x106) ||
        !result.used_motion_override ||
        !result.used_height_motion_fixup ||
        read_le32(defender + 0x198u) != UINT32_C(0x08000106)) {
        return 6;
    }

    return 0;
}
