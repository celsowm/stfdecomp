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

static int nearf_value(float actual, float expected)
{
    return fabsf(actual - expected) <= 0.00001f;
}

int main(void)
{
    uint32_t out = 0u;

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

    return 0;
}
