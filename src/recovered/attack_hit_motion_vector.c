#include "attack_hit_motion_vector.h"

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

static bool scale_value(float *value, uint32_t scale_bits)
{
    const float scale = bits_to_float(scale_bits);

    if (value == NULL || !isfinite(scale)) {
        return false;
    }

    *value *= scale;
    return isfinite(*value);
}

static bool trig_command(
    uint32_t opcode,
    uint32_t angle,
    uint32_t scale,
    stf_attack_hit_guard_vector_tgp_fn tgp,
    void *tgp_user_data,
    uint8_t *completed,
    uint32_t *reply
)
{
    stf_attack_hit_guard_vector_query q;
    uint32_t bits = 0u;

    if (tgp == NULL) {
        return opcode == UINT32_C(0x12002424)
            ? stf_copro_scalar_sin_scale_bits(angle, scale, reply)
            : stf_copro_scalar_cos_scale_bits(angle, scale, reply);
    }

    memset(&q, 0, sizeof(q));
    q.opcode = opcode;
    q.argument_count = 2u;
    q.words[0] = angle;
    q.words[1] = scale;
    if (!tgp(tgp_user_data, &q, &bits) ||
        !isfinite(bits_to_float(bits))) {
        return false;
    }
    *reply = bits;
    ++*completed;
    return true;
}

bool stf_attack_hit_motion_vector_compute_with_tgp(
    const stf_motion_prefix_result *prefix,
    const stf_motion_vector_inputs *inputs,
    stf_attack_hit_guard_vector_tgp_fn tgp,
    void *tgp_user_data,
    uint8_t *completed_commands,
    stf_motion_vector_result *result
)
{
    static const float attacker_scale[9][2] = {
        {1.0f, 1.0f},
        {1.0f, 1.0f},
        {1.6f, 1.0f},
        {1.2f, 2.0f},
        {1.0f, 1.2f},
        {1.0f, 1.0f},
        {1.0f, 1.0f},
        {1.0f, 1.0f},
        {1.6f, 1.0f},
    };
    uint8_t completed = 0u;
    uint32_t y_bits = 0u;
    uint32_t horizontal_bits = 0u;
    uint32_t x_bits = 0u;
    uint32_t z_bits = 0u;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    const bool special_attacker =
        inputs != NULL &&
        (inputs->attacker_flags_0 & (UINT32_C(1) << 18u)) != 0u &&
        (inputs->attacker_7d2 & UINT8_C(1)) == 0u &&
        (inputs->attacker_70c & (UINT32_C(1) << 26u)) == 0u;

    if (completed_commands != NULL) {
        *completed_commands = 0u;
    }
    if (prefix == NULL || inputs == NULL || result == NULL) {
        return false;
    }

    if (!trig_command(UINT32_C(0x12002424),
            (uint32_t)prefix->angle_r6, prefix->sqrt_r4_bits,
            tgp, tgp_user_data, &completed, &y_bits)) {
        goto fail;
    }
    if (!trig_command(UINT32_C(0x12802525),
            (uint32_t)prefix->angle_r6, prefix->sqrt_r4_bits,
            tgp, tgp_user_data, &completed, &horizontal_bits)) {
        goto fail;
    }

    y = bits_to_float(y_bits);

    if (inputs->hit_mode != 0u) {
        if (special_attacker) {
            y *= 4.0f;
        } else if ((inputs->defender_flags_1a4 & (UINT32_C(1) << 16u)) != 0u) {
            y *= 2.0f;
        } else if (!scale_value(&y, inputs->profile_vertical_scale_bits)) {
            goto fail;
        }
    }

    if (!isfinite(y)) {
        goto fail;
    }
    if (!trig_command(UINT32_C(0x12002424),
            (uint32_t)(UINT32_C(0) -
                (uint32_t)inputs->horizontal_angle_r10),
            horizontal_bits, tgp, tgp_user_data, &completed, &x_bits)) {
        goto fail;
    }
    if (!trig_command(UINT32_C(0x12802525),
            (uint32_t)(UINT32_C(0) -
                (uint32_t)inputs->horizontal_angle_r10),
            horizontal_bits, tgp, tgp_user_data, &completed, &z_bits)) {
        goto fail;
    }

    x = bits_to_float(x_bits);
    z = bits_to_float(z_bits);

    if (inputs->hit_mode != 0u && !special_attacker) {
        if (!scale_value(&x, inputs->profile_horizontal_scale_bits) ||
            !scale_value(&z, inputs->profile_horizontal_scale_bits)) {
            goto fail;
        }
    }

    if ((inputs->attacker_7d2 & UINT8_C(1)) != 0u) {
        const uint8_t index = inputs->attacker_843;

        if (index >= 9u) {
            goto fail;
        }

        y *= attacker_scale[index][0];
        x *= attacker_scale[index][1];
        z *= attacker_scale[index][1];
    }

    if (!isfinite(x) || !isfinite(y) || !isfinite(z)) {
        goto fail;
    }

    result->x_5e0_bits = float_to_bits(x);
    result->y_5e4_bits = float_to_bits(y);
    result->z_5e8_bits = float_to_bits(z);
    if (completed_commands != NULL) {
        *completed_commands = completed;
    }
    return true;

fail:
    if (completed_commands != NULL) {
        *completed_commands = completed;
    }
    return false;
}

bool stf_attack_hit_motion_vector_compute(
    const stf_motion_prefix_result *prefix,
    const stf_motion_vector_inputs *inputs,
    stf_motion_vector_result *result
)
{
    return stf_attack_hit_motion_vector_compute_with_tgp(
        prefix, inputs, NULL, NULL, NULL, result
    );
}
