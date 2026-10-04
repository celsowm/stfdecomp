#include <stdint.h>
#include <string.h>

#include "damage_unit.h"

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

int main(void)
{
    uint8_t attacker[STF_DAMAGE_UNIT_ATTACKER_MIN_SIZE];
    uint8_t defender[STF_DAMAGE_UNIT_DEFENDER_MIN_SIZE];
    uint8_t workspace[STF_DAMAGE_UNIT_WORKSPACE_MIN_SIZE];
    stf_damage_unit_effect_state effect;
    stf_damage_unit_result result;
    size_t count = 0u;
    const uint8_t *categories = stf_damage_unit_kind_categories(&count);

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    memset(&effect, 0, sizeof(effect));

    if (categories == NULL ||
        count != STF_DAMAGE_UNIT_KIND_CATEGORY_COUNT ||
        categories[0] != UINT8_C(0) ||
        categories[2] != UINT8_C(1) ||
        categories[11] != UINT8_C(0)) {
        return 1;
    }

    if (!stf_damage_unit_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            UINT8_C(2), &effect, &result
        ) ||
        !result.skipped) {
        return 2;
    }

    write_le32(workspace + 0x26Cu, UINT32_C(1));
    attacker[0x821u] = UINT8_C(0); /* category 0 */
    attacker[0x822u] = UINT8_C(20);
    write_le16(defender + 0x6F0u, (UINT16_C(1) << 12u) | (UINT16_C(1) << 4u));
    write_le16(defender + 0x1F08u, UINT16_C(10));
    write_le32(defender + 0xAF0u, UINT32_C(25));
    write_le32(defender + 0xAF4u, UINT32_C(1000));

    if (!stf_damage_unit_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            UINT8_C(2), &effect, &result
        ) ||
        !result.matched_slot ||
        result.selected_slot != UINT8_C(4) ||
        result.selected_category != UINT8_C(0) ||
        result.accumulator_before != UINT16_C(10) ||
        result.accumulator_after != UINT16_C(30) ||
        read_le16(defender + 0x1F08u) != UINT16_C(30) ||
        result.up_total_1f74 != UINT32_C(30) ||
        result.down_total_1f78 != UINT32_C(0) ||
        (result.flags_7f0 & UINT32_C(1)) == 0u) {
        return 3;
    }

    memset(defender, 0, sizeof(defender));
    attacker[0x821u] = UINT8_C(2); /* category 1 */
    attacker[0x822u] = UINT8_C(8);
    write_le16(defender + 0x6F0u, UINT16_C(1) << 12u);
    write_le16(defender + 0x1F18u, UINT16_C(5));
    write_le32(defender + 0x1A4u, UINT32_C(1) << 13u);
    write_le32(defender + 0xAF0u, UINT32_C(1000));
    write_le32(defender + 0xAF4u, UINT32_C(16));

    if (!stf_damage_unit_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            UINT8_C(2), &effect, &result
        ) ||
        result.selected_slot != UINT8_C(12) ||
        result.accumulator_before != UINT16_C(5) ||
        result.accumulator_after != UINT16_C(17) ||
        result.down_total_1f78 != UINT32_C(17) ||
        (result.flags_7f0 & (UINT32_C(1) << 1u)) == 0u) {
        return 4;
    }

    memset(defender, 0, sizeof(defender));
    memset(&effect, 0, sizeof(effect));
    attacker[0x821u] = UINT8_C(0);
    attacker[0x822u] = UINT8_C(3);
    attacker[0x1F5Cu] = UINT8_C(1);
    attacker[0x7D2u] = UINT8_C(1);
    write_le16(defender + 0x6F0u, UINT16_C(1));
    write_le32(defender + 0xAF0u, UINT32_C(0x1000u * 9u));
    write_le32(defender + 0xAF4u, UINT32_C(0x1000u * 7u));
    write_le32(defender + 0x1F74u, UINT32_C(111));
    write_le32(defender + 0x1F78u, UINT32_C(222));

    if (!stf_damage_unit_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            UINT8_C(2), &effect, &result
        ) ||
        !result.final_round_override ||
        effect.active_914 != UINT32_C(1) ||
        read_le16(defender + 0x1F00u) != UINT16_C(0x1000) ||
        read_le16(defender + 0x1F1Eu) != UINT16_C(0x1000) ||
        result.up_total_1f74 != UINT32_C(0x1000u * 9u) ||
        result.down_total_1f78 != UINT32_C(0x1000u * 7u) ||
        read_le32(defender + 0x1F7Cu) != UINT32_C(111) ||
        read_le32(defender + 0x1F80u) != UINT32_C(222)) {
        return 5;
    }

    return 0;
}
