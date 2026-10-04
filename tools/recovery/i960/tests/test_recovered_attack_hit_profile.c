#include <stdint.h>
#include <string.h>

#include "attack_hit_profile.h"

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
    uint8_t table[STF_ATTACK_HIT_PROFILE_RECORD_SIZE * 2u];
    stf_attack_hit_profile profile;
    uint8_t *record = NULL;

    memset(table, 0, sizeof(table));
    record = table + STF_ATTACK_HIT_PROFILE_RECORD_SIZE;

    write_le32(record + 0x00u, UINT32_C(0x3F800000));
    write_le32(record + 0x04u, UINT32_C(0x40000000));
    write_le32(record + 0x08u, UINT32_C(0x40400000));
    write_le32(record + 0x0Cu, UINT32_C(0x12345678));
    write_le32(record + 0x10u, UINT32_C(0x3F000000));
    write_le32(record + 0x14u, UINT32_C(0x3F19999A));
    write_le32(record + 0x18u, UINT32_C(0x3F333333));
    write_le32(record + 0x1Cu, UINT32_C(0x3F4CCCCD));
    write_le16(record + 0x20u, UINT16_C(10));
    write_le16(record + 0x22u, UINT16_C(20));
    write_le16(record + 0x24u, UINT16_C(30));
    write_le16(record + 0x26u, UINT16_C(40));

    if (!stf_attack_hit_profile_decode(
            table, sizeof(table), UINT8_C(1), &profile
        ) ||
        profile.horizontal_scale_bits != UINT32_C(0x3F800000) ||
        profile.vertical_scale_bits != UINT32_C(0x40000000) ||
        profile.strength_scale_bits != UINT32_C(0x40400000) ||
        profile.unknown_0c != UINT32_C(0x12345678) ||
        profile.fallback.scale_normal_bits != UINT32_C(0x3F000000) ||
        profile.fallback.scale_mode3_bits != UINT32_C(0x3F19999A) ||
        profile.fallback.scale_down_bits != UINT32_C(0x3F333333) ||
        profile.fallback.scale_down_mode3_bits != UINT32_C(0x3F4CCCCD) ||
        profile.fallback.angle_normal != INT16_C(10) ||
        profile.fallback.angle_mode3 != INT16_C(20) ||
        profile.fallback.angle_down != INT16_C(30) ||
        profile.fallback.angle_down_mode3 != INT16_C(40)) {
        return 1;
    }

    if (stf_attack_hit_profile_decode(
            table,
            STF_ATTACK_HIT_PROFILE_RECORD_SIZE,
            UINT8_C(1),
            &profile
        )) {
        return 2;
    }

    return 0;
}
