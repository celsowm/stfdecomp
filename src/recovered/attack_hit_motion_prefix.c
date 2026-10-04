#include "attack_hit_motion_prefix.h"
#include "copro_scalar.h"

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

static float bits_to_float(uint32_t bits)
{
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static uint32_t float_to_bits(float value)
{
    uint32_t bits = 0u;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static bool bit32(uint32_t value, unsigned bit)
{
    return (value & (UINT32_C(1) << bit)) != 0u;
}

bool stf_attack_hit_motion_prefix_compute(
    const stf_motion_prefix_inputs *inputs,
    const uint8_t *mht_record,
    size_t mht_record_size,
    const stf_motion_fallback_profile *fallback_profile,
    stf_motion_prefix_result *result
)
{
    stf_motion_prefix_result local;
    uint32_t angle_raw = 0u;
    uint32_t scale_bits = 0u;
    float r9 = 0.0f;
    float adjusted = 0.0f;
    float sqrt_input = 0.0f;
    uint32_t sqrt_bits = 0u;

    if (inputs == NULL || result == NULL) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    r9 = bits_to_float(inputs->initial_r9_bits);

    if (mht_record != NULL) {
        if (mht_record_size < 7u) {
            return false;
        }

        local.source = STF_MOTION_PREFIX_RECORD;
        angle_raw = read_le16(mht_record + 1u);
        scale_bits = read_le32(mht_record + 3u);
        r9 *= bits_to_float(scale_bits);
        r9 *= 2.0f;
    } else {
        bool down = false;
        bool mode3 = false;

        if (fallback_profile == NULL) {
            return false;
        }

        local.source = STF_MOTION_PREFIX_PROFILE;
        down = bit32(inputs->defender_flags_1a4, 4u);
        mode3 = inputs->hit_mode == UINT32_C(3);

        if (!down && !mode3) {
            angle_raw = (uint32_t)(int32_t)fallback_profile->angle_normal;
            scale_bits = fallback_profile->scale_normal_bits;
        } else if (!down && mode3) {
            angle_raw = (uint32_t)(int32_t)fallback_profile->angle_mode3;
            scale_bits = fallback_profile->scale_mode3_bits;
        } else if (down && !mode3) {
            angle_raw = (uint32_t)(int32_t)fallback_profile->angle_down;
            scale_bits = fallback_profile->scale_down_bits;
        } else {
            angle_raw = (uint32_t)(int32_t)fallback_profile->angle_down_mode3;
            scale_bits = fallback_profile->scale_down_mode3_bits;
        }

        r9 *= bits_to_float(scale_bits);
    }

    if (inputs->defender_combo_6f5 > inputs->combo_start) {
        const uint32_t count =
            (uint32_t)inputs->defender_combo_6f5 -
            (uint32_t)inputs->combo_start;
        const uint32_t reduction = (uint32_t)inputs->combo_sub * count;
        const uint32_t original_angle = angle_raw;
        uint32_t floor_value = inputs->combo_limit;

        angle_raw -= reduction;

        if (!((uint32_t)inputs->combo_limit <= original_angle)) {
            floor_value = 0u;
        }

        if ((int32_t)angle_raw < (int32_t)floor_value) {
            angle_raw = floor_value;
        }
    }

    if ((int32_t)angle_raw > (int32_t)inputs->limit_xang) {
        angle_raw = (uint32_t)(int32_t)inputs->limit_xang;
    }

    local.angle_r6 = (int32_t)angle_raw;
    local.scaled_r9_bits = float_to_bits(r9);

    adjusted =
        60.0f +
        (bits_to_float(inputs->defender_5d8_bits) - 60.0f) * 0.3f;
    if (adjusted == 0.0f) {
        return false;
    }

    sqrt_input = (r9 / adjusted) * 4.0f;
    if (!stf_copro_scalar_sqrt_bits(float_to_bits(sqrt_input), &sqrt_bits)) {
        return false;
    }

    local.sqrt_r4_bits = sqrt_bits;
    *result = local;
    return true;
}
