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

bool stf_copro_scalar_atan2_angle_bits(
    uint32_t x_bits,
    uint32_t z_bits,
    uint16_t *angle_word
)
{
    const float x = bits_to_float(x_bits);
    const float z = bits_to_float(z_bits);
    const float tau = 6.28318530717958647692f;
    float angle = 0.0f;
    long scaled = 0;

    if (angle_word == NULL || !isfinite(x) || !isfinite(z)) {
        return false;
    }

    angle = atan2f(z, x);
    if (angle < 0.0f) {
        angle += tau;
    }

    scaled = lroundf(angle * (65536.0f / tau));
    *angle_word = (uint16_t)scaled;
    return true;
}

bool stf_copro_scalar_rotate_y_xz_bits(
    uint16_t angle_word,
    uint32_t x_bits,
    uint32_t z_bits,
    uint32_t *out_x_bits,
    uint32_t *out_z_bits
)
{
    const float x = bits_to_float(x_bits);
    const float z = bits_to_float(z_bits);
    const float tau = 6.28318530717958647692f;
    const float radians = (float)angle_word * (tau / 65536.0f);
    const float sine = sinf(radians);
    const float cosine = cosf(radians);
    const float out_x = cosine * x - sine * z;
    const float out_z = sine * x + cosine * z;

    if (out_x_bits == NULL || out_z_bits == NULL ||
        !isfinite(x) || !isfinite(z) ||
        !isfinite(out_x) || !isfinite(out_z)) {
        return false;
    }

    *out_x_bits = float_to_bits(out_x);
    *out_z_bits = float_to_bits(out_z);
    return true;
}


bool stf_copro_scalar_transform_point_bits(
    const uint32_t matrix_bits[12],
    const uint32_t input_bits[3],
    uint32_t output_bits[3]
)
{
    float m[12];
    float x;
    float y;
    float z;
    float out_x;
    float out_y;
    float out_z;
    size_t i;

    if (matrix_bits == NULL || input_bits == NULL || output_bits == NULL) {
        return false;
    }

    for (i = 0u; i < 12u; ++i) {
        m[i] = bits_to_float(matrix_bits[i]);
        if (!isfinite(m[i])) {
            return false;
        }
    }

    x = bits_to_float(input_bits[0]);
    y = bits_to_float(input_bits[1]);
    z = bits_to_float(input_bits[2]);
    if (!isfinite(x) || !isfinite(y) || !isfinite(z)) {
        return false;
    }

    out_x = x * m[0] + y * m[3] + z * m[6] + m[9];
    out_y = x * m[1] + y * m[4] + z * m[7] + m[10];
    out_z = x * m[2] + y * m[5] + z * m[8] + m[11];

    if (!isfinite(out_x) || !isfinite(out_y) || !isfinite(out_z)) {
        return false;
    }

    output_bits[0] = float_to_bits(out_x);
    output_bits[1] = float_to_bits(out_y);
    output_bits[2] = float_to_bits(out_z);
    return true;
}


bool stf_copro_collision_sphere_overlap_bits(
    const uint32_t query_xyz_bits[3],
    uint32_t query_radius_bits,
    const uint32_t ball_xyz_bits[3],
    uint32_t ball_radius_bits,
    bool *overlap,
    uint32_t *distance_bits,
    uint32_t *penetration_bits
)
{
    float qx;
    float qy;
    float qz;
    float qr;
    float bx;
    float by;
    float bz;
    float br;
    float dx;
    float dy;
    float dz;
    float distance;
    float radius_sum;
    float penetration;

    if (query_xyz_bits == NULL || ball_xyz_bits == NULL ||
        overlap == NULL || distance_bits == NULL ||
        penetration_bits == NULL) {
        return false;
    }

    qx = bits_to_float(query_xyz_bits[0]);
    qy = bits_to_float(query_xyz_bits[1]);
    qz = bits_to_float(query_xyz_bits[2]);
    qr = bits_to_float(query_radius_bits);
    bx = bits_to_float(ball_xyz_bits[0]);
    by = bits_to_float(ball_xyz_bits[1]);
    bz = bits_to_float(ball_xyz_bits[2]);
    br = bits_to_float(ball_radius_bits);

    if (!isfinite(qx) || !isfinite(qy) || !isfinite(qz) ||
        !isfinite(qr) || !isfinite(bx) || !isfinite(by) ||
        !isfinite(bz) || !isfinite(br) || qr < 0.0f || br < 0.0f) {
        return false;
    }

    dx = bx - qx;
    dy = by - qy;
    dz = bz - qz;
    distance = sqrtf(dx * dx + dy * dy + dz * dz);
    radius_sum = qr + br;
    penetration = radius_sum - distance;

    *overlap = br != 0.0f && distance <= radius_sum;
    *distance_bits = float_to_bits(distance);
    *penetration_bits = float_to_bits(
        penetration > 0.0f ? penetration : 0.0f
    );
    return true;
}


static uint32_t command77_unit_mask(
    const stf_copro_command77_collision_state *state,
    size_t fighter,
    uint32_t ball_mask
)
{
    uint32_t unit_mask = 0u;
    size_t ball;

    for (ball = 0u; ball < STF_COPRO_COMMAND77_BALLS; ++ball) {
        if ((ball_mask & (UINT32_C(1) << ball)) != 0u) {
            const uint8_t unit = state->balls[fighter][ball].unit_index;
            unit_mask |= UINT32_C(1) << (unit & UINT8_C(31));
        }
    }
    return unit_mask;
}

bool stf_copro_command77_semantic_bits(
    const uint32_t query_xyz_bits[3],
    uint32_t query_radius_bits,
    const stf_copro_command77_collision_state *state,
    uint32_t output_words[STF_COPRO_COMMAND77_WORDS]
)
{
    float query[3];
    float query_radius;
    float push_x = 0.0f;
    float push_z = 0.0f;
    uint32_t last_fighter = UINT32_MAX;
    uint32_t last_ball = 0u;
    uint32_t last_unit = 0u;
    uint32_t ball_masks[STF_COPRO_COMMAND77_FIGHTERS] = {0u, 0u};
    uint32_t unit_masks[STF_COPRO_COMMAND77_FIGHTERS] = {0u, 0u};
    size_t fighter;

    if (query_xyz_bits == NULL || state == NULL || output_words == NULL) {
        return false;
    }

    query[0] = bits_to_float(query_xyz_bits[0]);
    query[1] = bits_to_float(query_xyz_bits[1]);
    query[2] = bits_to_float(query_xyz_bits[2]);
    query_radius = bits_to_float(query_radius_bits);
    if (!isfinite(query[0]) || !isfinite(query[1]) ||
        !isfinite(query[2]) || !isfinite(query_radius) ||
        query_radius < 0.0f) {
        return false;
    }

    for (fighter = 0u; fighter < STF_COPRO_COMMAND77_FIGHTERS; ++fighter) {
        const stf_copro_command77_ball *root = &state->balls[fighter][13u];
        const float root_x = bits_to_float(root->position_bits[0]);
        const float root_y = bits_to_float(root->position_bits[1]);
        const float root_z = bits_to_float(root->position_bits[2]);
        const float root_dx = root_x - query[0];
        const float root_dy = root_y - query[1];
        const float root_dz = root_z - query[2];
        const float root_distance = sqrtf(
            root_dx * root_dx + root_dy * root_dy + root_dz * root_dz
        );
        float last_factor = 0.0f;
        size_t ball;

        if (!isfinite(root_x) || !isfinite(root_y) || !isfinite(root_z) ||
            !isfinite(root_distance)) {
            return false;
        }

        if (root_distance > 3.0f) {
            continue;
        }

        for (ball = 0u; ball < STF_COPRO_COMMAND77_BALLS; ++ball) {
            const stf_copro_command77_ball *entry = &state->balls[fighter][ball];
            const uint32_t radius_bits = entry->radius_bits;
            const float ball_radius = bits_to_float(radius_bits);
            float bx;
            float by;
            float bz;
            float dx;
            float dy;
            float dz;
            float distance;
            float radius_sum;
            float factor;

            if (radius_bits == UINT32_C(0)) {
                continue;
            }

            bx = bits_to_float(entry->position_bits[0]);
            by = bits_to_float(entry->position_bits[1]);
            bz = bits_to_float(entry->position_bits[2]);
            if (!isfinite(bx) || !isfinite(by) || !isfinite(bz) ||
                !isfinite(ball_radius) || ball_radius < 0.0f) {
                return false;
            }

            dx = bx - query[0];
            dy = by - query[1];
            dz = bz - query[2];
            distance = sqrtf(dx * dx + dy * dy + dz * dz);
            radius_sum = ball_radius + query_radius;
            if (!isfinite(distance) || !isfinite(radius_sum) ||
                radius_sum <= 0.0f || distance > radius_sum) {
                continue;
            }

            factor = 1.0f - (2.0f * distance / radius_sum);
            if (!isfinite(factor)) {
                return false;
            }

            push_x += dx * factor;
            push_z += dz * factor;
            last_factor = factor;
            last_ball = (uint32_t)ball;
            ball_masks[fighter] |= UINT32_C(1) << ball;
        }

        unit_masks[fighter] = command77_unit_mask(
            state, fighter, ball_masks[fighter]
        );

        /*
         * Firmware tests the final per-fighter factor after the 32-ball scan.
         * Thus the last overlapping ball controls whether last_fighter/unit are
         * updated, even though all overlaps remain represented in the masks.
         */
        if (last_factor != 0.0f) {
            last_fighter = (uint32_t)fighter;
            last_unit = state->balls[fighter][last_ball].unit_index;
        }
    }

    output_words[0] = float_to_bits(push_x);
    output_words[1] = float_to_bits(push_z);
    output_words[2] = last_fighter;
    output_words[3] = last_ball;
    output_words[4] = last_unit;
    output_words[5] = ball_masks[0];
    output_words[6] = unit_masks[0];
    output_words[7] = ball_masks[1];
    output_words[8] = unit_masks[1];
    return true;
}
