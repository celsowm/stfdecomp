#include <stdint.h>
#include <string.h>

#include "attack_hit_guard_state.h"

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
    uint8_t attacker[STF_GUARD_STATE_ATTACKER_MIN_SIZE];
    uint8_t defender[STF_GUARD_STATE_DEFENDER_MIN_SIZE];
    uint8_t workspace[STF_GUARD_STATE_WORKSPACE_MIN_SIZE];
    stf_guard_state_result result;

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));

    write_le32(workspace + 0x26Cu, UINT32_C(1));
    write_le32(defender + 0xC70u, UINT32_C(2));
    write_le16(attacker + 0x6D0u, UINT16_C(0xFFFB));
    write_le16(attacker + 0x1224u, UINT16_C(1));
    write_le32(attacker + 0x1228u, UINT32_C(0x11223344));
    write_le16(attacker + 0x1226u, UINT16_C(0x5566));
    write_le16(attacker + 0x1248u, UINT16_C(2));
    write_le16(attacker + 0x124Au, UINT16_C(0x7788));
    write_le16(attacker + 0x1AAu, UINT16_C(50));
    write_le16(attacker + 0x80Cu, UINT16_C(10));

    if (!stf_attack_hit_guard_apply_model2(
            STF_GUARD_BLOCK_A,
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            0u,
            0u,
            &result
        )) {
        return 1;
    }

    if (result.workspace_26c != UINT32_C(9) ||
        result.sound != STF_GUARD_SOUND_KNOCK_9 ||
        result.defender_c70 != 1 ||
        result.attacker_194 != UINT32_C(0x10000002) ||
        result.defender_198 != UINT32_C(0x0A00013D) ||
        result.defender_6d8 != -5 ||
        !result.copy_122x ||
        !result.copy_124x ||
        result.requires_sub_2b94c ||
        result.defender_5de != 30 ||
        read_le32(attacker + 0x121Cu) != UINT32_C(0x11223344) ||
        read_le16(attacker + 0x1220u) != UINT16_C(0x5566) ||
        read_le16(attacker + 0x1244u) != UINT16_C(0x7788) ||
        read_le16(defender + 0x5DEu) != UINT16_C(30)) {
        return 2;
    }

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));

    write_le32(defender + 0xC70u, UINT32_C(3));
    write_le16(attacker + 0x1AAu, UINT16_C(70));
    write_le16(attacker + 0x80Cu, UINT16_C(20));

    if (!stf_attack_hit_guard_apply_model2(
            STF_GUARD_BLOCK_B,
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            UINT16_C(1) << 13u,
            UINT32_C(0x1234),
            &result
        )) {
        return 3;
    }

    if (result.sound != STF_GUARD_SOUND_KNOCK_3 ||
        result.defender_c70 != 2 ||
        result.defender_198 != UINT32_C(0x0B001234) ||
        !result.requires_sub_2b94c ||
        result.defender_5de != 56 ||
        read_le32(defender + 0x198u) != UINT32_C(0x0B001234)) {
        return 4;
    }

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    attacker[0x85Cu] = UINT8_C(12);
    write_le16(attacker + 0x1AAu, UINT16_C(100));
    write_le16(attacker + 0x80Cu, UINT16_C(20));

    if (!stf_attack_hit_guard_apply_model2(
            STF_GUARD_BLOCK_A,
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            0u,
            0u,
            &result
        ) ||
        result.defender_5de != 12) {
        return 5;
    }

    return 0;
}
