#include "attack_hit_guard_state.h"

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

static bool bit16(uint16_t value, unsigned bit)
{
    return (value & (uint16_t)(UINT16_C(1) << bit)) != 0u;
}

static int16_t min_i16(int16_t a, int16_t b)
{
    return a < b ? a : b;
}

bool stf_attack_hit_guard_apply_model2(
    stf_guard_block_kind kind,
    uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint8_t *workspace,
    size_t workspace_size,
    uint16_t hit_flags_50fe00,
    uint32_t block_b_motion_g0,
    stf_guard_state_result *result
)
{
    stf_guard_state_result local;
    uint32_t flags = 0u;
    int32_t c70 = 0;
    uint16_t flags_1224 = 0u;
    uint16_t flags_1248 = 0u;
    int16_t candidate = 0;
    int16_t delta = 0;

    if ((kind != STF_GUARD_BLOCK_A && kind != STF_GUARD_BLOCK_B) ||
        attacker == NULL || defender == NULL || workspace == NULL ||
        attacker_size < STF_GUARD_STATE_ATTACKER_MIN_SIZE ||
        defender_size < STF_GUARD_STATE_DEFENDER_MIN_SIZE ||
        workspace_size < STF_GUARD_STATE_WORKSPACE_MIN_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));

    flags = read_le32(workspace + 0x26Cu) | (UINT32_C(1) << 3u);
    write_le32(workspace + 0x26Cu, flags);
    local.workspace_26c = flags;

    write_le32(defender + 0xC6Cu, UINT32_C(0x13));

    c70 = (int32_t)read_le32(defender + 0xC70u);
    if (c70 != 0) {
        --c70;
        local.sound =
            c70 == 0 ? STF_GUARD_SOUND_KNOCK_9 : STF_GUARD_SOUND_KNOCK_3;
    }
    write_le32(defender + 0xC70u, (uint32_t)c70);
    local.defender_c70 = c70;

    local.attacker_194 = UINT32_C(0x10000002);
    write_le32(attacker + 0x194u, local.attacker_194);

    if (kind == STF_GUARD_BLOCK_A) {
        local.defender_198 = UINT32_C(0x0A00013D);
    } else {
        local.defender_198 = UINT32_C(0x0B000000) + block_b_motion_g0;
        local.requires_sub_2b94c = true;
    }
    write_le32(defender + 0x198u, local.defender_198);

    local.defender_6d8 = (int16_t)read_le16(attacker + 0x6D0u);
    write_le16(defender + 0x6D8u, (uint16_t)local.defender_6d8);

    flags_1224 = read_le16(attacker + 0x1224u);
    if (bit16(flags_1224, 0u) || bit16(flags_1224, 1u)) {
        write_le32(attacker + 0x121Cu, read_le32(attacker + 0x1228u));
        write_le16(attacker + 0x1220u, read_le16(attacker + 0x1226u));
        local.copy_122x = true;
    }

    flags_1248 = read_le16(attacker + 0x1248u);
    if (bit16(flags_1248, 0u) || bit16(flags_1248, 1u)) {
        write_le16(attacker + 0x1244u, read_le16(attacker + 0x124Au));
        local.copy_124x = true;
    }

    if (attacker[0x85Cu] != 0u) {
        candidate = (int16_t)attacker[0x85Cu];
    } else {
        delta = (int16_t)(
            (int16_t)read_le16(attacker + 0x1AAu) -
            (int16_t)read_le16(attacker + 0x80Cu)
        );
        candidate = delta;

        if (kind == STF_GUARD_BLOCK_A) {
            candidate = min_i16(candidate, INT16_C(30));
        } else if (bit16(hit_flags_50fe00, 13u) && candidate > 40) {
            candidate = (int16_t)(candidate + 40);
        }
    }

    delta = (int16_t)(
        (int16_t)read_le16(attacker + 0x1AAu) -
        (int16_t)read_le16(attacker + 0x80Cu)
    );
    local.defender_5de = min_i16(candidate, (int16_t)(delta + 6));
    write_le16(defender + 0x5DEu, (uint16_t)local.defender_5de);

    if (result != NULL) {
        *result = local;
    }
    return true;
}
