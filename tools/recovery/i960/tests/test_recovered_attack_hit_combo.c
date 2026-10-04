#include <stdint.h>
#include <string.h>

#include "attack_hit_combo.h"

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
    uint8_t attacker[STF_ATTACK_HIT_COMBO_ATTACKER_MIN_SIZE];
    uint8_t defender[STF_ATTACK_HIT_COMBO_DEFENDER_MIN_SIZE];
    uint8_t workspace[STF_ATTACK_HIT_COMBO_WORKSPACE_MIN_SIZE];
    stf_attack_hit_combo_result result;

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));

    write_le32(attacker + 0x5B8u, UINT32_C(1));

    if (!stf_attack_hit_combo_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            0u, &result
        ) ||
        !result.skipped ||
        defender[0x6F4u] != 0u) {
        return 1;
    }

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));

    write_le32(defender + 0x000u, UINT32_C(1) << 29u);
    write_le16(defender + 0xA0Eu, UINT16_C(1000));
    defender[0x6F4u] = 2u;
    attacker[0x822u] = 12u;
    write_le32(workspace + 0x26Cu, UINT32_C(1));

    if (!stf_attack_hit_combo_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            UINT8_C(4), &result
        )) {
        return 2;
    }

    if (result.skipped ||
        !result.requires_set_kamae ||
        result.bonus_skill != UINT32_C(2000) ||
        result.hit_skill != UINT32_C(12) ||
        result.workspace_26c != UINT32_C(0x23) ||
        result.defender_combo_6f4 != UINT8_C(3) ||
        (read_le32(defender + 0x000u) & (UINT32_C(1) << 29u)) != 0u ||
        read_le16(defender + 0xA0Eu) != 0u ||
        defender[0x6F4u] != UINT8_C(3)) {
        return 3;
    }

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));

    attacker[0x822u] = 8u;
    write_le32(attacker + 0x1A4u, UINT32_C(1) << 26u);

    if (!stf_attack_hit_combo_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            0u, &result
        ) ||
        result.workspace_26c != UINT32_C(1) << 4u ||
        result.hit_skill != UINT32_C(8)) {
        return 4;
    }

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));

    write_le32(attacker + 0x860u, UINT32_C(1) << 19u);
    write_le32(attacker + 0x720u, UINT32_C(1) << 12u);
    attacker[0x822u] = 9u;

    if (!stf_attack_hit_combo_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            0u, &result
        ) ||
        !result.skipped ||
        defender[0x6F4u] != 0u) {
        return 5;
    }

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));

    write_le32(defender + 0x000u, UINT32_C(1) << 29u);
    write_le32(attacker + 0x860u, UINT32_C(1) << 19u);

    if (!stf_attack_hit_combo_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            0u, &result
        ) ||
        !result.skipped ||
        !((read_le32(defender + 0x000u) & (UINT32_C(1) << 29u)) != 0u)) {
        return 6;
    }

    return 0;
}
