#include <stdint.h>
#include <string.h>

#include "attack_hit_motion_runtime.h"

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
    uint32_t offsets[64];
    uint8_t blob[128];
    uint8_t strides[32];
    stf_motion_prefix_inputs inputs;
    stf_motion_fallback_profile fallback;
    stf_attack_hit_motion_runtime_result result;

    memset(offsets, 0, sizeof(offsets));
    memset(blob, 0, sizeof(blob));
    memset(strides, 0, sizeof(strides));
    memset(&inputs, 0, sizeof(inputs));
    memset(&fallback, 0, sizeof(fallback));

    inputs.initial_r9_bits = UINT32_C(0x41200000); /* 10.0 */
    inputs.limit_xang = INT16_C(200);
    inputs.defender_5d8_bits = UINT32_C(0x42700000); /* 60.0 */

    offsets[16] = 20u;
    strides[5] = 4u;
    blob[33] = UINT8_C(5); /* 20 + 0x0d */
    blob[37] = UINT8_C(0x11);
    write_le16(blob + 38u, UINT16_C(100));
    write_le32(blob + 40u, UINT32_C(0x40400000)); /* 3.0 */

    if (!stf_attack_hit_motion_prefix_resolve(
            UINT32_C(16),
            &inputs,
            offsets,
            sizeof(offsets) / sizeof(offsets[0]),
            blob,
            sizeof(blob),
            strides,
            sizeof(strides),
            &fallback,
            &result
        ) ||
        result.lookup_status != STF_MOTION_HIT_FOUND ||
        !result.used_mht_record ||
        result.record_offset != UINT32_C(37) ||
        result.prefix.source != STF_MOTION_PREFIX_RECORD ||
        result.prefix.angle_r6 != 100 ||
        result.prefix.scaled_r9_bits != UINT32_C(0x42700000) ||
        result.prefix.sqrt_r4_bits != UINT32_C(0x40000000)) {
        return 1;
    }

    blob[37] = UINT8_C(8);
    fallback.scale_normal_bits = UINT32_C(0x40000000); /* 2.0 */
    fallback.angle_normal = INT16_C(80);

    if (!stf_attack_hit_motion_prefix_resolve(
            UINT32_C(16),
            &inputs,
            offsets,
            sizeof(offsets) / sizeof(offsets[0]),
            blob,
            sizeof(blob),
            strides,
            sizeof(strides),
            &fallback,
            &result
        ) ||
        result.lookup_status != STF_MOTION_HIT_NOT_FOUND ||
        result.used_mht_record ||
        result.prefix.source != STF_MOTION_PREFIX_PROFILE ||
        result.prefix.angle_r6 != 80 ||
        result.prefix.scaled_r9_bits != UINT32_C(0x41A00000)) {
        return 2;
    }

    blob[37] = UINT8_C(9);
    if (stf_attack_hit_motion_prefix_resolve(
            UINT32_C(16),
            &inputs,
            offsets,
            sizeof(offsets) / sizeof(offsets[0]),
            blob,
            sizeof(blob),
            strides,
            sizeof(strides),
            &fallback,
            &result
        )) {
        return 3;
    }

    return 0;
}
