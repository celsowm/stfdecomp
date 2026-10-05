#include <math.h>
#include <stdint.h>
#include <string.h>

#include "copro_scalar.h"

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

static int nearf_value(float actual, float expected)
{
    return fabsf(actual - expected) <= 0.00001f;
}

int main(void)
{
    uint32_t out = 0u;
    uint32_t out_z = 0u;
    uint16_t angle = 0u;

    if (!stf_copro_scalar_sqrt_bits(UINT32_C(0x40800000), &out) ||
        out != UINT32_C(0x40000000)) {
        return 1;
    }

    if (!stf_copro_scalar_sqrt_bits(UINT32_C(0x3F800000), &out) ||
        out != UINT32_C(0x3F800000)) {
        return 2;
    }

    if (!stf_copro_scalar_sqrt_bits(UINT32_C(0x00000000), &out) ||
        out != UINT32_C(0x00000000)) {
        return 3;
    }

    if (stf_copro_scalar_sqrt_bits(UINT32_C(0xBF800000), &out)) {
        return 4;
    }

    if (!stf_copro_scalar_sin_scale_bits(
            UINT32_C(0x0000), UINT32_C(0x40000000), &out
        ) ||
        !nearf_value(bits_to_float(out), 0.0f)) {
        return 5;
    }

    if (!stf_copro_scalar_cos_scale_bits(
            UINT32_C(0x0000), UINT32_C(0x40000000), &out
        ) ||
        !nearf_value(bits_to_float(out), 2.0f)) {
        return 6;
    }

    if (!stf_copro_scalar_sin_scale_bits(
            UINT32_C(0x4000), UINT32_C(0x40000000), &out
        ) ||
        !nearf_value(bits_to_float(out), 2.0f)) {
        return 7;
    }

    if (!stf_copro_scalar_cos_scale_bits(
            UINT32_C(0x4000), UINT32_C(0x40000000), &out
        ) ||
        !nearf_value(bits_to_float(out), 0.0f)) {
        return 8;
    }

    if (!stf_copro_scalar_sin_scale_bits(
            UINT32_C(0xC000), UINT32_C(0x40000000), &out
        ) ||
        !nearf_value(bits_to_float(out), -2.0f)) {
        return 9;
    }

    if (!stf_copro_scalar_atan2_angle_bits(
            float_to_bits(1.0f), float_to_bits(0.0f), &angle
        ) ||
        angle != UINT16_C(0x0000)) {
        return 10;
    }

    if (!stf_copro_scalar_atan2_angle_bits(
            float_to_bits(0.0f), float_to_bits(1.0f), &angle
        ) ||
        angle != UINT16_C(0x4000)) {
        return 11;
    }

    if (!stf_copro_scalar_atan2_angle_bits(
            float_to_bits(-1.0f), float_to_bits(0.0f), &angle
        ) ||
        angle != UINT16_C(0x8000)) {
        return 12;
    }

    if (!stf_copro_scalar_rotate_y_xz_bits(
            UINT16_C(0x0000),
            float_to_bits(1.0f),
            float_to_bits(0.0f),
            &out,
            &out_z
        ) ||
        !nearf_value(bits_to_float(out), 1.0f) ||
        !nearf_value(bits_to_float(out_z), 0.0f)) {
        return 13;
    }

    if (!stf_copro_scalar_rotate_y_xz_bits(
            UINT16_C(0x4000),
            float_to_bits(1.0f),
            float_to_bits(0.0f),
            &out,
            &out_z
        ) ||
        !nearf_value(bits_to_float(out), 0.0f) ||
        !nearf_value(bits_to_float(out_z), 1.0f)) {
        return 14;
    }

    {
        uint32_t matrix[12] = {
            float_to_bits(1.0f), float_to_bits(0.0f), float_to_bits(0.0f),
            float_to_bits(0.0f), float_to_bits(1.0f), float_to_bits(0.0f),
            float_to_bits(0.0f), float_to_bits(0.0f), float_to_bits(1.0f),
            float_to_bits(10.0f), float_to_bits(20.0f), float_to_bits(30.0f)
        };
        uint32_t input[3] = {
            float_to_bits(1.0f),
            float_to_bits(2.0f),
            float_to_bits(3.0f)
        };
        uint32_t output[3];

        if (!stf_copro_scalar_transform_point_bits(matrix, input, output) ||
            !nearf_value(bits_to_float(output[0]), 11.0f) ||
            !nearf_value(bits_to_float(output[1]), 22.0f) ||
            !nearf_value(bits_to_float(output[2]), 33.0f)) {
            return 15;
        }
    }

    {
        uint32_t matrix[12] = {
            float_to_bits(0.0f), float_to_bits(0.0f), float_to_bits(-1.0f),
            float_to_bits(0.0f), float_to_bits(1.0f), float_to_bits(0.0f),
            float_to_bits(1.0f), float_to_bits(0.0f), float_to_bits(0.0f),
            float_to_bits(0.0f), float_to_bits(0.0f), float_to_bits(0.0f)
        };
        uint32_t input[3] = {
            float_to_bits(2.0f),
            float_to_bits(3.0f),
            float_to_bits(4.0f)
        };
        uint32_t output[3];

        if (!stf_copro_scalar_transform_point_bits(matrix, input, output) ||
            !nearf_value(bits_to_float(output[0]), 4.0f) ||
            !nearf_value(bits_to_float(output[1]), 3.0f) ||
            !nearf_value(bits_to_float(output[2]), -2.0f)) {
            return 16;
        }
    }

    {
        const uint32_t query[3] = {
            float_to_bits(0.0f),
            float_to_bits(0.0f),
            float_to_bits(0.0f)
        };
        const uint32_t ball[3] = {
            float_to_bits(3.0f),
            float_to_bits(4.0f),
            float_to_bits(0.0f)
        };
        bool overlap = false;
        uint32_t distance_bits = 0u;
        uint32_t penetration_bits = 0u;

        if (!stf_copro_collision_sphere_overlap_bits(
                query,
                float_to_bits(2.0f),
                ball,
                float_to_bits(4.0f),
                &overlap,
                &distance_bits,
                &penetration_bits
            ) ||
            !overlap ||
            !nearf_value(bits_to_float(distance_bits), 5.0f) ||
            !nearf_value(bits_to_float(penetration_bits), 1.0f)) {
            return 17;
        }

        if (!stf_copro_collision_sphere_overlap_bits(
                query,
                float_to_bits(0.5f),
                ball,
                float_to_bits(1.0f),
                &overlap,
                &distance_bits,
                &penetration_bits
            ) ||
            overlap ||
            !nearf_value(bits_to_float(distance_bits), 5.0f) ||
            !nearf_value(bits_to_float(penetration_bits), 0.0f)) {
            return 18;
        }

        if (!stf_copro_collision_sphere_overlap_bits(
                query,
                float_to_bits(100.0f),
                ball,
                float_to_bits(0.0f),
                &overlap,
                &distance_bits,
                &penetration_bits
            ) ||
            overlap) {
            return 19;
        }
    }


    {
        stf_copro_command77_collision_state state;
        const uint32_t query[3] = {
            float_to_bits(0.0f),
            float_to_bits(0.0f),
            float_to_bits(0.0f)
        };
        uint32_t words[STF_COPRO_COMMAND77_WORDS];

        memset(&state, 0, sizeof(state));

        /* P0 ball 13 is the broad-phase root and remains close. */
        state.balls[0][13].position_bits[0] = float_to_bits(0.0f);
        state.balls[0][13].position_bits[1] = float_to_bits(0.0f);
        state.balls[0][13].position_bits[2] = float_to_bits(0.0f);

        state.balls[0][0].position_bits[0] = float_to_bits(0.25f);
        state.balls[0][0].radius_bits = float_to_bits(0.5f);
        state.balls[0][0].unit_index = UINT8_C(3);

        state.balls[0][1].position_bits[2] = float_to_bits(0.2f);
        state.balls[0][1].radius_bits = float_to_bits(0.5f);
        state.balls[0][1].unit_index = UINT8_C(5);

        /* P1 is broad-phase rejected. */
        state.balls[1][13].position_bits[0] = float_to_bits(10.0f);

        if (!stf_copro_command77_semantic_bits(
                query, float_to_bits(0.5f), &state, words
            ) ||
            !nearf_value(bits_to_float(words[0]), 0.125f) ||
            !nearf_value(bits_to_float(words[1]), 0.12f) ||
            words[2] != UINT32_C(0) ||
            words[3] != UINT32_C(1) ||
            words[4] != UINT32_C(5) ||
            words[5] != UINT32_C(0x00000003) ||
            words[6] != ((UINT32_C(1) << 3u) | (UINT32_C(1) << 5u)) ||
            words[7] != UINT32_C(0) ||
            words[8] != UINT32_C(0)) {
            return 20;
        }
    }

    return 0;
}
