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

static bool trig_scale_bits(
    uint32_t angle_word,
    uint32_t scale_bits,
    bool cosine,
    uint32_t *output_bits
)
{
    const float scale = bits_to_float(scale_bits);
    const uint16_t angle = (uint16_t)angle_word;
    const float tau = 6.28318530717958647692f;
    const float radians = (float)angle * (tau / 65536.0f);
    const float trig = cosine ? cosf(radians) : sinf(radians);
    const float result = trig * scale;

    if (output_bits == NULL || !isfinite(scale) || !isfinite(result)) {
        return false;
    }

    *output_bits = float_to_bits(result);
    return true;
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

bool stf_copro_scalar_sin_scale_bits(
    uint32_t angle_word,
    uint32_t scale_bits,
    uint32_t *output_bits
)
{
    return trig_scale_bits(angle_word, scale_bits, false, output_bits);
}

bool stf_copro_scalar_cos_scale_bits(
    uint32_t angle_word,
    uint32_t scale_bits,
    uint32_t *output_bits
)
{
    return trig_scale_bits(angle_word, scale_bits, true, output_bits);
}
