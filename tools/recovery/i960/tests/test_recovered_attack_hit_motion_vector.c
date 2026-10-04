#include <math.h>
#include <stdint.h>
#include <string.h>

#include "attack_hit_motion_vector.h"

static float bits_to_float(uint32_t bits)
{
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static int nearf_value(float actual, float expected)
{
    return fabsf(actual - expected) <= 0.0001f;
}

int main(void)
{
    stf_motion_prefix_result prefix;
    stf_motion_vector_inputs in;
    stf_motion_vector_result out;

    memset(&prefix, 0, sizeof(prefix));
    memset(&in, 0, sizeof(in));

    prefix.angle_r6 = 0;
    prefix.sqrt_r4_bits = UINT32_C(0x40000000);
    in.horizontal_angle_r10 = 0;

    if (!stf_attack_hit_motion_vector_compute(&prefix, &in, &out) ||
        !nearf_value(bits_to_float(out.x_5e0_bits), 0.0f) ||
        !nearf_value(bits_to_float(out.y_5e4_bits), 0.0f) ||
        !nearf_value(bits_to_float(out.z_5e8_bits), 2.0f)) {
        return 1;
    }

    memset(&in, 0, sizeof(in));
    prefix.angle_r6 = 0x4000;
    in.hit_mode = 1u;
    in.attacker_flags_0 = UINT32_C(1) << 18u;
    in.profile_horizontal_scale_bits = UINT32_C(0x40400000);
    in.profile_vertical_scale_bits = UINT32_C(0x3FC00000);

    if (!stf_attack_hit_motion_vector_compute(&prefix, &in, &out) ||
        !nearf_value(bits_to_float(out.y_5e4_bits), 8.0f)) {
        return 2;
    }

    memset(&in, 0, sizeof(in));
    prefix.angle_r6 = 0;
    in.hit_mode = 1u;
    in.profile_horizontal_scale_bits = UINT32_C(0x40400000);
    in.profile_vertical_scale_bits = UINT32_C(0x3FC00000);
    in.attacker_7d2 = UINT8_C(1);
    in.attacker_843 = UINT8_C(3);

    if (!stf_attack_hit_motion_vector_compute(&prefix, &in, &out) ||
        !nearf_value(bits_to_float(out.x_5e0_bits), 0.0f) ||
        !nearf_value(bits_to_float(out.y_5e4_bits), 0.0f) ||
        !nearf_value(bits_to_float(out.z_5e8_bits), 12.0f)) {
        return 3;
    }

    memset(&in, 0, sizeof(in));
    prefix.angle_r6 = 0x4000;
    in.hit_mode = 1u;
    in.profile_horizontal_scale_bits = UINT32_C(0x3F800000);
    in.profile_vertical_scale_bits = UINT32_C(0x3FC00000);

    if (!stf_attack_hit_motion_vector_compute(&prefix, &in, &out) ||
        !nearf_value(bits_to_float(out.y_5e4_bits), 3.0f)) {
        return 4;
    }

    in.defender_flags_1a4 = UINT32_C(1) << 16u;
    if (!stf_attack_hit_motion_vector_compute(&prefix, &in, &out) ||
        !nearf_value(bits_to_float(out.y_5e4_bits), 4.0f)) {
        return 5;
    }

    in.attacker_7d2 = UINT8_C(1);
    in.attacker_843 = UINT8_C(9);
    if (stf_attack_hit_motion_vector_compute(&prefix, &in, &out)) {
        return 6;
    }

    return 0;
}
