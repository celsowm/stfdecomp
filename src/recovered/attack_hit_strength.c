#include "attack_hit_strength.h"
#include "copro_scalar.h"

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

static void write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

static uint32_t u8_to_float_bits(uint8_t value)
{
    unsigned top = 0u;
    uint32_t mantissa = 0u;
    uint32_t exponent = 0u;

    if (value == 0u) {
        return 0u;
    }

    for (top = 7u; top > 0u; --top) {
        if ((value & (uint8_t)(UINT8_C(1) << top)) != 0u) {
            break;
        }
    }

    mantissa = (uint32_t)(
        value & (uint8_t)~(uint8_t)(UINT8_C(1) << top)
    );
    mantissa <<= (23u - top);
    exponent = (uint32_t)(127u + top) << 23u;
    return exponent | mantissa;
}

bool stf_attack_hit_strength_prepare_model2(
    const uint8_t *fighter,
    size_t fighter_size,
    uint8_t *workspace,
    size_t workspace_size,
    stf_attack_hit_strength_input *result
)
{
    uint32_t flags = 0u;
    int16_t field_82a = 0;
    int16_t field_026 = 0;
    stf_attack_hit_strength_input local;

    if (fighter == NULL || workspace == NULL || result == NULL ||
        fighter_size < STF_ATTACK_HIT_STRENGTH_FIGHTER_MODEL2_MIN_SIZE ||
        workspace_size < STF_ATTACK_HIT_STRENGTH_WORKSPACE_MODEL2_MIN_SIZE) {
        return false;
    }

    flags = read_le32(workspace + 0x26Cu);
    flags |= UINT32_C(1);
    write_le32(workspace + 0x26Cu, flags);

    field_82a = (int16_t)read_le16(fighter + 0x82Au);
    field_026 = (int16_t)read_le16(fighter + 0x026u);

    local.combined_82a_026 = (int32_t)field_82a + (int32_t)field_026;
    local.raw_822 = fighter[0x822u];
    local.raw_822_float_bits = u8_to_float_bits(local.raw_822);
    local.workspace_26c = flags;

    *result = local;
    return true;
}


static float stf_bits_to_float(uint32_t bits)
{
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static uint32_t stf_float_to_bits(float value)
{
    uint32_t bits = 0u;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

bool stf_attack_hit_strength_scale_bits(
    uint32_t raw_strength_float_bits,
    uint32_t hit_kind_scale_bits,
    uint32_t *result_bits
)
{
    uint32_t sqrt_bits = 0u;
    float value = 0.0f;
    const float raw_strength = stf_bits_to_float(raw_strength_float_bits);
    const float hit_kind_scale = stf_bits_to_float(hit_kind_scale_bits);

    if (result_bits == NULL) {
        return false;
    }

    value = raw_strength * 0.01f;
    if (!stf_copro_scalar_sqrt_bits(stf_float_to_bits(value), &sqrt_bits)) {
        return false;
    }

    value = stf_bits_to_float(sqrt_bits);
    value *= 50.0f;
    value *= 0.0024999999f;
    value *= hit_kind_scale;

    *result_bits = stf_float_to_bits(value);
    return true;
}
