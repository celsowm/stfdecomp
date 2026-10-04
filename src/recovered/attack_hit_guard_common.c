#include "attack_hit_guard_common.h"

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

static bool bit16(uint16_t value, unsigned bit)
{
    return (value & (uint16_t)(UINT16_C(1) << bit)) != 0u;
}

static int16_t min_i16(int16_t a, int16_t b)
{
    return a < b ? a : b;
}

static int16_t max_i16(int16_t a, int16_t b)
{
    return a > b ? a : b;
}

bool stf_attack_hit_guard_common_apply_model2(
    uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint8_t *workspace,
    size_t workspace_size,
    uint8_t *enemy_slot,
    size_t enemy_slot_size,
    uint8_t hit_flags_50fe03,
    uint8_t guard_limit,
    uint32_t motion_g0,
    stf_guard_common_result *result
)
{
    stf_guard_common_result local;
    uint8_t raw_strength = 0u;
    uint16_t flags_1224 = 0u;
    uint16_t flags_1248 = 0u;
    uint32_t flags_26c = 0u;
    int16_t candidate = 0;
    int16_t delta = 0;

    if (attacker == NULL || defender == NULL || workspace == NULL ||
        enemy_slot == NULL ||
        attacker_size < STF_GUARD_COMMON_ATTACKER_MIN_SIZE ||
        defender_size < STF_GUARD_COMMON_DEFENDER_MIN_SIZE ||
        workspace_size < STF_GUARD_COMMON_WORKSPACE_MIN_SIZE ||
        enemy_slot_size < STF_GUARD_COMMON_ENEMY_MIN_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    raw_strength = attacker[0x822u];

    if (bit32(read_le32(attacker + 0x860u), 19u) && raw_strength == 0u) {
        local.skipped = true;
        if (result != NULL) {
            *result = local;
        }
        return true;
    }

    local.attacker_counter_1234 = read_le32(attacker + 0x1234u) - UINT32_C(1);
    local.attacker_counter_1238 = read_le32(attacker + 0x1238u) + UINT32_C(1);
    write_le32(attacker + 0x1234u, local.attacker_counter_1234);
    write_le32(attacker + 0x1238u, local.attacker_counter_1238);

    enemy_slot[0x109u] = UINT8_C(1);
    local.enemy_109 = UINT8_C(1);
    local.skill_amount = (uint32_t)raw_strength >> 1u;

    local.attacker_194 = UINT32_C(0x10000002);
    write_le32(attacker + 0x194u, local.attacker_194);

    local.defender_198 = UINT32_C(0x0A000000) + motion_g0;
    write_le32(defender + 0x198u, local.defender_198);
    local.requires_sub_2b94c = true;

    local.defender_6d8 = (int16_t)read_le16(attacker + 0x6D0u);
    write_le16(defender + 0x6D8u, (uint16_t)local.defender_6d8);

    flags_1224 = read_le16(attacker + 0x1224u);
    if (bit16(flags_1224, 1u)) {
        write_le32(attacker + 0x121Cu, read_le32(attacker + 0x1228u));
        write_le16(attacker + 0x1220u, read_le16(attacker + 0x1226u));
        local.copy_122x = true;
    }

    flags_1248 = read_le16(attacker + 0x1248u);
    if (bit16(flags_1248, 1u)) {
        write_le16(attacker + 0x1244u, read_le16(attacker + 0x124Au));
        local.copy_124x = true;
    }

    if (attacker[0x85Cu] != 0u) {
        candidate = (int16_t)attacker[0x85Cu];
        delta = (int16_t)(
            (int16_t)read_le16(attacker + 0x80Cu) -
            (int16_t)read_le16(attacker + 0x1AAu)
        );
        candidate = min_i16(candidate, (int16_t)(delta + 6));
    } else if ((hit_flags_50fe03 & UINT8_C(1)) == 0u) {
        const uint32_t derived =
            ((uint32_t)raw_strength / UINT32_C(3)) * UINT32_C(2) +
            UINT32_C(4);
        candidate = (int16_t)derived;
        candidate = min_i16(candidate, (int16_t)guard_limit);

        delta = (int16_t)(
            (int16_t)read_le16(attacker + 0x80Cu) -
            (int16_t)read_le16(attacker + 0x1AAu)
        );
        candidate = min_i16(candidate, (int16_t)(delta + 6));
    } else {
        candidate = (int16_t)(
            (int16_t)read_le16(attacker + 0x80Cu) -
            (int16_t)read_le16(attacker + 0x808u) -
            INT16_C(7)
        );
        candidate = min_i16(candidate, (int16_t)guard_limit);
        candidate = max_i16(candidate, INT16_C(8));
    }

    local.defender_5de = candidate;
    write_le16(defender + 0x5DEu, (uint16_t)candidate);

    flags_26c = read_le32(workspace + 0x26Cu) | (UINT32_C(1) << 2u);
    write_le32(workspace + 0x26Cu, flags_26c);
    local.workspace_26c = flags_26c;

    if (result != NULL) {
        *result = local;
    }
    return true;
}
