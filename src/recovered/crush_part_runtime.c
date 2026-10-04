#include "crush_part_runtime.h"

#include <string.h>

static uint16_t read_le16u(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8u));
}

static int16_t read_le16s(const uint8_t *p)
{
    return (int16_t)read_le16u(p);
}

static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8u) |
           ((uint32_t)p[2] << 16u) |
           ((uint32_t)p[3] << 24u);
}

static void write_le32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
    p[2] = (uint8_t)(value >> 16u);
    p[3] = (uint8_t)(value >> 24u);
}

static void write_le16(uint8_t *p, int16_t value)
{
    const uint16_t u = (uint16_t)value;
    p[0] = (uint8_t)u;
    p[1] = (uint8_t)(u >> 8u);
}

static int16_t add_wrap16(int16_t a, int32_t b)
{
    return (int16_t)((uint16_t)a + (uint16_t)b);
}

static int16_t approach_axis(int16_t current, int16_t target, int32_t step)
{
    const int32_t delta = (int32_t)target - (int32_t)current;

    if (target == 0 || target == current) {
        return current;
    }
    if (delta > step) {
        return add_wrap16(current, -step);
    }
    if (delta < -step) {
        return add_wrap16(current, step);
    }
    return target;
}

bool stf_crush_part_update_angles_model2(
    uint8_t *slot,
    size_t slot_size,
    stf_crush_part_angle_result *result
)
{
    uint32_t flags;
    uint32_t contacts;
    int32_t step;
    int16_t x;
    int16_t y;
    int16_t z;
    const int16_t spin = slot != NULL ? read_le16s(slot + 0x2Eu) : 0;

    if (slot == NULL || result == NULL || slot_size < STF_CRUSH_PART_SLOT_SIZE) {
        return false;
    }

    flags = read_le32(slot + 0x24u);
    contacts = read_le32(slot + 0x3Cu);
    x = read_le16s(slot + 0x1Cu);
    y = read_le16s(slot + 0x1Eu);
    z = read_le16s(slot + 0x20u);

    step = (flags & (UINT32_C(1) << 7u)) != 0u ? 0x800 : 0x1000;

    if ((flags & (UINT32_C(1) << 7u)) == 0u && contacts <= UINT32_C(3)) {
        if ((flags & (UINT32_C(1) << 12u)) != 0u) {
            x = add_wrap16(x, spin);
        }
        y = add_wrap16(y, spin);
        if ((flags & (UINT32_C(1) << 14u)) != 0u) {
            z = add_wrap16(z, spin);
        }
    } else {
        const int16_t target_x = read_le16s(slot + 0x28u);
        const int16_t target_y = read_le16s(slot + 0x2Au);
        const int16_t target_z = read_le16s(slot + 0x2Cu);
        const int32_t negative_step = -step;

        if (target_x != 0 && target_x != x) {
            const int32_t delta = (int32_t)target_x - (int32_t)x;
            if (delta > step) {
                x = add_wrap16(x, -step);
            } else if (delta < negative_step) {
                x = add_wrap16(x, step);
            } else {
                x = target_x;
            }
        }

        if (target_y != 0 && target_y != y) {
            const int32_t delta = (int32_t)target_y - (int32_t)y;
            if (delta > step || delta < negative_step) {
                const uint32_t divisor = contacts + UINT32_C(1);
                if (divisor != UINT32_C(0)) {
                    step /= (int32_t)divisor;
                }
                if (delta > 0) {
                    y = add_wrap16(y, -step);
                } else {
                    y = add_wrap16(y, step);
                }
            } else {
                y = target_y;
            }
        }

        if (target_z != 0 && target_z != z) {
            const int32_t delta = (int32_t)target_z - (int32_t)z;
            if (delta > step) {
                z = add_wrap16(z, -step);
            } else if (delta < negative_step) {
                z = add_wrap16(z, step);
            } else {
                z = target_z;
            }
        }
    }

    write_le16(slot + 0x1Cu, x);
    write_le16(slot + 0x1Eu, y);
    write_le16(slot + 0x20u, z);

    result->angle_x = x;
    result->angle_y = y;
    result->angle_z = z;
    result->step = (int16_t)step;
    return true;
}

bool stf_crush_part_build_draw_model2(
    const uint8_t *slot,
    size_t slot_size,
    stf_crush_part_draw *draw
)
{
    uint32_t flags;

    if (slot == NULL || draw == NULL || slot_size < STF_CRUSH_PART_SLOT_SIZE) {
        return false;
    }

    memset(draw, 0, sizeof(*draw));
    draw->object_id = read_le16u(slot + 0x22u);
    if (draw->object_id == UINT16_C(0)) {
        return true;
    }

    flags = read_le32(slot + 0x24u);
    draw->active = true;
    draw->owner_index = (uint8_t)(flags & UINT32_C(1));
    draw->use_saved_graphics_state =
        (flags & (UINT32_C(1) << 19u)) != 0u;
    draw->position[0] = read_le32(slot + 0x00u);
    draw->position[1] = read_le32(slot + 0x04u);
    draw->position[2] = read_le32(slot + 0x08u);
    draw->angle_x = read_le16s(slot + 0x1Cu);
    draw->angle_y = read_le16s(slot + 0x1Eu);
    draw->angle_z = read_le16s(slot + 0x20u);
    return true;
}


#include <string.h>

static float bits_to_float(uint32_t bits)
{
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static uint32_t float_to_bits(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static float read_f32(const uint8_t *p)
{
    return bits_to_float(read_le32(p));
}

static void write_f32(uint8_t *p, float value)
{
    const uint32_t bits = float_to_bits(value);
    p[0] = (uint8_t)bits;
    p[1] = (uint8_t)(bits >> 8u);
    p[2] = (uint8_t)(bits >> 16u);
    p[3] = (uint8_t)(bits >> 24u);
}

bool stf_crush_part_update_position_model2(
    uint8_t *slot,
    size_t slot_size,
    const stf_crush_part_physics_env *env,
    stf_crush_part_physics_result *result
)
{
    stf_crush_part_physics_result local;
    uint32_t flags;
    float px, py, pz, vx, vy, vz;
    const float damping = 0.98f;
    const float wall_bounce = -0.3f;
    const float stop_threshold = bits_to_float(UINT32_C(0x3A83126F));
    const float half = 0.5f;

    if (slot == NULL || env == NULL || result == NULL ||
        slot_size < STF_CRUSH_PART_SLOT_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    flags = read_le32(slot + 0x24u);
    px = read_f32(slot + 0x00u);
    py = read_f32(slot + 0x04u);
    pz = read_f32(slot + 0x08u);
    vx = read_f32(slot + 0x0Cu);
    vy = read_f32(slot + 0x10u);
    vz = read_f32(slot + 0x14u);

    if ((flags & (UINT32_C(1) << 7u)) != 0u) {
        if ((flags & (UINT32_C(1) << 3u)) != 0u &&
            env->effect_active_914 == UINT32_C(0) &&
            env->visibility_mask == UINT32_C(0)) {
            write_le16(slot + 0x22u, INT16_C(0));
            write_le32(slot + 0x24u, UINT32_C(0));
            local.deactivated = true;
            local.flags = UINT32_C(0);
            *result = local;
            return true;
        }
    } else {
        vy -= bits_to_float(env->gravity_bits);
        vx *= damping;
        vz *= damping;
        px += vx;
        py += vy;
        pz += vz;

        {
            float floor = read_f32(slot + 0x40u);
            if ((flags & (UINT32_C(1) << 19u)) != 0u) {
                floor += bits_to_float(env->stage_floor_bits);
            }

            if (py <= floor) {
                uint32_t contacts = read_le32(slot + 0x3Cu) + UINT32_C(1);
                const float bounce_param = read_f32(slot + 0x34u);
                float bounce_scale = 0.2f;

                write_le32(slot + 0x3Cu, contacts);
                local.floor_hit = true;
                local.ground_contacts = contacts;
                if (contacts <= UINT32_C(2)) {
                    local.request_floor_effect = true;
                }

                py -= vy;
                if (bounce_param != 0.0f) {
                    bounce_scale += 0.2f / bounce_param;
                }
                vy *= bounce_scale;
                vy = -vy;

                if (vy < 0.0f ? -vy <= stop_threshold : vy <= stop_threshold) {
                    flags |= UINT32_C(1) << 7u;
                    vx = 0.0f;
                    vy = 0.0f;
                    vz = 0.0f;
                    local.stopped_bouncing = true;
                } else {
                    vx *= 0.9f;
                    vz *= 0.9f;
                }
            }
        }
    }

    if (py <= bits_to_float(env->cage_height_bits)) {
        const float stage_x = bits_to_float(env->stage_x_bits);
        const float outer = stage_x + half;
        const float radius = read_f32(slot + 0x18u);
        const float inner = stage_x - radius;
        const bool x_negative = (float_to_bits(px) & UINT32_C(0x80000000)) != 0u;
        const float abs_x = px < 0.0f ? -px : px;
        const float abs_z = pz < 0.0f ? -pz : pz;

        if (abs_x > outer) {
            flags |= UINT32_C(1) << 19u;
        } else {
            const bool wall_open =
                x_negative
                    ? (env->finish_wall_flags & (UINT32_C(1) << 1u)) != 0u
                    : (env->finish_wall_flags & (UINT32_C(1) << 3u)) != 0u;
            if (!wall_open && abs_x > inner) {
                vx *= wall_bounce;
                px = x_negative ? -inner : inner;
                local.hit_x_wall = true;
            }
        }

        if (abs_z > outer) {
            flags |= UINT32_C(1) << 19u;
        } else {
            const bool wall_open =
                x_negative
                    ? (env->finish_wall_flags & (UINT32_C(1) << 2u)) != 0u
                    : (env->finish_wall_flags & (UINT32_C(1) << 0u)) != 0u;
            if (!wall_open && abs_z > inner) {
                vz *= wall_bounce;
                pz = pz < 0.0f ? -inner : inner;
                local.hit_z_wall = true;
            }
        }
    }

    write_f32(slot + 0x00u, px);
    write_f32(slot + 0x04u, py);
    write_f32(slot + 0x08u, pz);
    write_f32(slot + 0x0Cu, vx);
    write_f32(slot + 0x10u, vy);
    write_f32(slot + 0x14u, vz);
    write_le32(slot + 0x24u, flags);

    local.ground_contacts = read_le32(slot + 0x3Cu);
    local.flags = flags;
    *result = local;
    return true;
}
