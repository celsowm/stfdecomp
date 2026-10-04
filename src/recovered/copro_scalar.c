#include "copro_scalar.h"

#include <math.h>
#include <string.h>

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

bool stf_copro_scalar_sqrt_bits(uint32_t input_bits, uint32_t *output_bits)
{
    const float input = bits_to_float(input_bits);

    if (output_bits == NULL || !isfinite(input) || input < 0.0f) {
        return false;
    }

    *output_bits = float_to_bits(sqrtf(input));
    return true;
}
