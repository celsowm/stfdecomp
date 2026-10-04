#include "attack_hit_state_prelude.h"

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

bool stf_attack_hit_state_prelude_apply_model2(
    uint8_t *attacker,
    size_t attacker_size,
    stf_attack_hit_state_prelude_result *result
)
{
    stf_attack_hit_state_prelude_result local;

    if (attacker == NULL ||
        attacker_size < STF_ATTACK_HIT_STATE_PRELUDE_MIN_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));

    local.attacker_194 = UINT32_C(0x10000001);
    write_le32(attacker + 0x194u, local.attacker_194);

    if ((read_le16(attacker + 0x1224u) & UINT16_C(1)) != 0u) {
        local.copied_122x = true;
        local.attacker_121c = read_le32(attacker + 0x1228u);
        local.attacker_1220 = (int16_t)read_le16(attacker + 0x1226u);
        write_le32(attacker + 0x121Cu, local.attacker_121c);
        write_le16(attacker + 0x1220u, (uint16_t)local.attacker_1220);
    }

    if ((read_le16(attacker + 0x1248u) & UINT16_C(1)) != 0u) {
        local.copied_124x = true;
        local.attacker_1244 = (int16_t)read_le16(attacker + 0x124Au);
        write_le16(attacker + 0x1244u, (uint16_t)local.attacker_1244);
    }

    if (result != NULL) {
        *result = local;
    }
    return true;
}
