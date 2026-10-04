#include "attack_hit_combo.h"

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

bool stf_attack_hit_combo_apply_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint8_t *workspace,
    size_t workspace_size,
    uint8_t hit_kind_50fe02,
    stf_attack_hit_combo_result *result
)
{
    stf_attack_hit_combo_result local;
    const uint32_t attacker_70c =
        attacker != NULL && attacker_size >= STF_ATTACK_HIT_COMBO_ATTACKER_MIN_SIZE
            ? read_le32(attacker + 0x70Cu)
            : 0u;
    const uint32_t attacker_5b8 =
        attacker != NULL && attacker_size >= STF_ATTACK_HIT_COMBO_ATTACKER_MIN_SIZE
            ? read_le32(attacker + 0x5B8u)
            : 0u;
    const uint32_t attacker_860 =
        attacker != NULL && attacker_size >= STF_ATTACK_HIT_COMBO_ATTACKER_MIN_SIZE
            ? read_le32(attacker + 0x860u)
            : 0u;
    uint32_t defender_flags = 0u;
    uint32_t workspace_flags = 0u;
    uint32_t attacker_720 = 0u;
    uint32_t attacker_19c = 0u;
    uint32_t attacker_1a4 = 0u;
    uint8_t combo = 0u;

    if (attacker == NULL || defender == NULL || workspace == NULL ||
        attacker_size < STF_ATTACK_HIT_COMBO_ATTACKER_MIN_SIZE ||
        defender_size < STF_ATTACK_HIT_COMBO_DEFENDER_MIN_SIZE ||
        workspace_size < STF_ATTACK_HIT_COMBO_WORKSPACE_MIN_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));

    if (!bit32(attacker_70c, 20u) && bit32(attacker_5b8, 0u)) {
        local.skipped = true;
        if (result != NULL) {
            *result = local;
        }
        return true;
    }

    defender_flags = read_le32(defender + 0x000u);
    if (bit32(defender_flags, 29u)) {
        if (bit32(attacker_860, 19u)) {
            local.skipped = true;
            if (result != NULL) {
                *result = local;
            }
            return true;
        }

        defender_flags &= ~(UINT32_C(1) << 29u);
        write_le32(defender + 0x000u, defender_flags);
        local.requires_set_kamae = true;
    }

    if (bit32(attacker_860, 19u)) {
        attacker_720 = read_le32(attacker + 0x720u);
        attacker_19c = read_le32(attacker + 0x19Cu);

        if (bit32(attacker_720, 12u) ||
            (bit32(attacker_720, 6u) && bit32(attacker_19c, 16u)) ||
            (!bit32(attacker_720, 6u) &&
             bit32(attacker_720, 7u) &&
             bit32(attacker_19c, 17u)) ||
            attacker[0x822u] == 0u) {
            local.skipped = true;
            if (result != NULL) {
                *result = local;
            }
            return true;
        }
    }

    if (read_le16(defender + 0xA0Eu) >= UINT16_C(1000)) {
        local.bonus_skill = UINT32_C(2000);
    }
    write_le16(defender + 0xA0Eu, 0u);

    local.hit_skill = attacker[0x822u];

    workspace_flags = read_le32(workspace + 0x26Cu);
    attacker_1a4 = read_le32(attacker + 0x1A4u);
    if (bit32(attacker_1a4, 26u)) {
        workspace_flags |= UINT32_C(1) << 4u;
    } else {
        workspace_flags |= UINT32_C(1) << 1u;
    }
    if (hit_kind_50fe02 == UINT8_C(4)) {
        workspace_flags |= UINT32_C(1) << 5u;
    }
    write_le32(workspace + 0x26Cu, workspace_flags);
    local.workspace_26c = workspace_flags;

    combo = (uint8_t)(defender[0x6F4u] + UINT8_C(1));
    defender[0x6F4u] = combo;
    local.defender_combo_6f4 = combo;

    if (result != NULL) {
        *result = local;
    }
    return true;
}
