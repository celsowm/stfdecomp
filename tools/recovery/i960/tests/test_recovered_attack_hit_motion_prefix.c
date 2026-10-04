#include <stdint.h>
#include <string.h>

#include "attack_hit_motion_prefix.h"

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
    stf_motion_prefix_inputs in;
    stf_motion_prefix_result out;
    stf_motion_fallback_profile profile;
    uint8_t record[7];

    memset(&in, 0, sizeof(in));
    memset(&profile, 0, sizeof(profile));
    memset(record, 0, sizeof(record));

    in.initial_r9_bits = UINT32_C(0x41200000); /* 10.0 */
    in.limit_xang = 200;
    in.defender_5d8_bits = UINT32_C(0x42700000); /* 60.0 */

    write_le16(record + 1u, UINT16_C(100));
    write_le32(record + 3u, UINT32_C(0x40400000)); /* 3.0 */

    if (!stf_attack_hit_motion_prefix_compute(
            &in, record, sizeof(record), NULL, &out
        ) ||
        out.source != STF_MOTION_PREFIX_RECORD ||
        out.angle_r6 != 100 ||
        out.scaled_r9_bits != UINT32_C(0x42700000) || /* 60.0 */
        out.sqrt_r4_bits != UINT32_C(0x40000000)) { /* 2.0 */
        return 1;
    }

    memset(&in, 0, sizeof(in));
    memset(&profile, 0, sizeof(profile));

    in.initial_r9_bits = UINT32_C(0x41F00000); /* 30.0 */
    in.defender_combo_6f5 = UINT8_C(5);
    in.combo_start = UINT8_C(2);
    in.combo_sub = UINT16_C(10);
    in.combo_limit = UINT16_C(20);
    in.limit_xang = 80;
    in.defender_5d8_bits = UINT32_C(0x42700000); /* 60.0 */

    profile.scale_normal_bits = UINT32_C(0x40000000); /* 2.0 */
    profile.angle_normal = 100;

    if (!stf_attack_hit_motion_prefix_compute(
            &in, NULL, 0u, &profile, &out
        ) ||
        out.source != STF_MOTION_PREFIX_PROFILE ||
        out.angle_r6 != 70 ||
        out.scaled_r9_bits != UINT32_C(0x42700000) ||
        out.sqrt_r4_bits != UINT32_C(0x40000000)) {
        return 2;
    }

    memset(&in, 0, sizeof(in));
    memset(&profile, 0, sizeof(profile));

    in.initial_r9_bits = UINT32_C(0x41700000); /* 15.0 */
    in.defender_flags_1a4 = UINT32_C(1) << 4u;
    in.hit_mode = UINT32_C(3);
    in.limit_xang = 100;
    in.defender_5d8_bits = UINT32_C(0x42700000);

    profile.scale_down_mode3_bits = UINT32_C(0x3F800000); /* 1.0 */
    profile.angle_down_mode3 = 40;

    if (!stf_attack_hit_motion_prefix_compute(
            &in, NULL, 0u, &profile, &out
        ) ||
        out.angle_r6 != 40 ||
        out.scaled_r9_bits != UINT32_C(0x41700000) ||
        out.sqrt_r4_bits != UINT32_C(0x3F800000)) {
        return 3;
    }

    in.defender_5d8_bits = UINT32_C(0x42700000);
    in.limit_xang = 30;
    if (!stf_attack_hit_motion_prefix_compute(
            &in, NULL, 0u, &profile, &out
        ) ||
        out.angle_r6 != 30) {
        return 4;
    }

    return 0;
}
