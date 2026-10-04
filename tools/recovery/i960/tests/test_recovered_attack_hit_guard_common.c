#include <stdint.h>
#include <string.h>

#include "attack_hit_guard_common.h"

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
    uint8_t attacker[STF_GUARD_COMMON_ATTACKER_MIN_SIZE];
    uint8_t defender[STF_GUARD_COMMON_DEFENDER_MIN_SIZE];
    uint8_t workspace[STF_GUARD_COMMON_WORKSPACE_MIN_SIZE];
    uint8_t enemy[STF_GUARD_COMMON_ENEMY_MIN_SIZE];
    stf_guard_common_result result;

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    memset(enemy, 0, sizeof(enemy));

    write_le32(attacker + 0x860u, UINT32_C(1) << 19u);
    attacker[0x822u] = 0u;

    if (!stf_attack_hit_guard_common_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            enemy, sizeof(enemy),
            0u, 30u, UINT32_C(0x123), &result
        ) ||
        !result.skipped ||
        read_le32(attacker + 0x1238u) != 0u ||
        enemy[0x109u] != 0u) {
        return 1;
    }

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    memset(enemy, 0, sizeof(enemy));

    attacker[0x822u] = 15u;
    write_le32(attacker + 0x1234u, UINT32_C(5));
    write_le32(attacker + 0x1238u, UINT32_C(9));
    write_le16(attacker + 0x6D0u, UINT16_C(0xFFFE));
    write_le16(attacker + 0x1224u, UINT16_C(2));
    write_le32(attacker + 0x1228u, UINT32_C(0xAABBCCDD));
    write_le16(attacker + 0x1226u, UINT16_C(0x3344));
    write_le16(attacker + 0x1248u, UINT16_C(2));
    write_le16(attacker + 0x124Au, UINT16_C(0x7788));
    write_le16(attacker + 0x1AAu, UINT16_C(10));
    write_le16(attacker + 0x80Cu, UINT16_C(40));
    write_le32(workspace + 0x26Cu, UINT32_C(1));

    if (!stf_attack_hit_guard_common_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            enemy, sizeof(enemy),
            0u, 20u, UINT32_C(0x222), &result
        )) {
        return 2;
    }

    if (result.skipped ||
        result.attacker_counter_1234 != UINT32_C(4) ||
        result.attacker_counter_1238 != UINT32_C(10) ||
        result.enemy_109 != UINT8_C(1) ||
        result.skill_amount != UINT32_C(7) ||
        result.attacker_194 != UINT32_C(0x10000002) ||
        result.defender_198 != UINT32_C(0x0A000222) ||
        result.defender_6d8 != -2 ||
        !result.copy_122x ||
        !result.copy_124x ||
        result.defender_5de != 14 ||
        result.workspace_26c != UINT32_C(5) ||
        read_le32(attacker + 0x121Cu) != UINT32_C(0xAABBCCDD) ||
        read_le16(attacker + 0x1220u) != UINT16_C(0x3344) ||
        read_le16(attacker + 0x1244u) != UINT16_C(0x7788) ||
        read_le16(defender + 0x5DEu) != UINT16_C(14)) {
        return 3;
    }

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    memset(enemy, 0, sizeof(enemy));

    attacker[0x822u] = 9u;
    write_le16(attacker + 0x808u, UINT16_C(18));
    write_le16(attacker + 0x80Cu, UINT16_C(15));

    if (!stf_attack_hit_guard_common_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            enemy, sizeof(enemy),
            UINT8_C(1), 20u, UINT32_C(0x11), &result
        ) ||
        result.defender_5de != 8 ||
        read_le16(defender + 0x5DEu) != UINT16_C(8)) {
        return 4;
    }

    write_le16(attacker + 0x808u, UINT16_C(10));
    write_le16(attacker + 0x80Cu, UINT16_C(80));

    if (!stf_attack_hit_guard_common_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            enemy, sizeof(enemy),
            UINT8_C(1), 20u, UINT32_C(0x11), &result
        ) ||
        result.defender_5de != 20) {
        return 5;
    }

    return 0;
}
