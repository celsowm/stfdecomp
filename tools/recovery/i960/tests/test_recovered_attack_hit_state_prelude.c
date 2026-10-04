#include <stdint.h>
#include <string.h>

#include "attack_hit_state_prelude.h"

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
    uint8_t attacker[STF_ATTACK_HIT_STATE_PRELUDE_MIN_SIZE];
    stf_attack_hit_state_prelude_result result;

    memset(attacker, 0, sizeof(attacker));

    if (!stf_attack_hit_state_prelude_apply_model2(
            attacker, sizeof(attacker), &result
        ) ||
        result.attacker_194 != UINT32_C(0x10000001) ||
        result.copied_122x ||
        result.copied_124x ||
        read_le32(attacker + 0x194u) != UINT32_C(0x10000001)) {
        return 1;
    }

    memset(attacker, 0, sizeof(attacker));
    write_le16(attacker + 0x1224u, UINT16_C(1));
    write_le16(attacker + 0x1226u, UINT16_C(0xFF9C));
    write_le32(attacker + 0x1228u, UINT32_C(0x12345678));
    write_le16(attacker + 0x1248u, UINT16_C(1));
    write_le16(attacker + 0x124Au, UINT16_C(0xFF38));

    if (!stf_attack_hit_state_prelude_apply_model2(
            attacker, sizeof(attacker), &result
        ) ||
        !result.copied_122x ||
        !result.copied_124x ||
        result.attacker_121c != UINT32_C(0x12345678) ||
        result.attacker_1220 != INT16_C(-100) ||
        result.attacker_1244 != INT16_C(-200) ||
        read_le32(attacker + 0x121Cu) != UINT32_C(0x12345678) ||
        (int16_t)read_le16(attacker + 0x1220u) != INT16_C(-100) ||
        (int16_t)read_le16(attacker + 0x1244u) != INT16_C(-200)) {
        return 2;
    }

    if (stf_attack_hit_state_prelude_apply_model2(
            attacker,
            STF_ATTACK_HIT_STATE_PRELUDE_MIN_SIZE - 1u,
            &result
        )) {
        return 3;
    }

    return 0;
}
