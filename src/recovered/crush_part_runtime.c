#include "crush_part_runtime.h"
#include "copro_scalar.h"

#include <math.h>
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

bool stf_crush_part_oidasi_model2(
    uint8_t *slot,
    size_t slot_size,
    const stf_crush_part_oidasi_input *input,
    stf_crush_part_oidasi_result *result
)
{
    stf_crush_part_oidasi_result local;
    float x;
    float y;
    float z;
    float dx;
    float dz;
    const float scale = bits_to_float(UINT32_C(0x3E23D70A));

    if (slot == NULL || input == NULL || result == NULL ||
        slot_size < STF_CRUSH_PART_SLOT_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    x = read_f32(slot + 0x00u);
    y = read_f32(slot + 0x04u);
    z = read_f32(slot + 0x08u);

    if (input->fighter0_parts_locked || input->fighter1_parts_locked) {
        local.skipped = true;
        local.position_bits[0] = read_le32(slot + 0x00u);
        local.position_bits[1] = read_le32(slot + 0x04u);
        local.position_bits[2] = read_le32(slot + 0x08u);
        *result = local;
        return true;
    }

    dx = bits_to_float(input->command77_output0_bits);
    dz = bits_to_float(input->command77_output1_bits);
    if (!isfinite(dx) || !isfinite(dz)) {
        return false;
    }

    x += dx * scale;
    z += dz * scale;
    write_f32(slot + 0x00u, x);
    write_f32(slot + 0x08u, z);

    local.applied = true;
    local.position_bits[0] = read_le32(slot + 0x00u);
    local.position_bits[1] = read_le32(slot + 0x04u);
    local.position_bits[2] = read_le32(slot + 0x08u);
    *result = local;
    return true;
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


bool stf_crush_part_update_position_with_visibility_model2(
    uint8_t *slot,
    size_t slot_size,
    const stf_crush_part_physics_env *env,
    const uint32_t camera_matrix_bits[12],
    uint32_t focus_distance_bits,
    stf_crush_part_physics_result *result,
    stf_crush_part_visibility_result *visibility
)
{
    stf_crush_part_physics_env local_env;
    stf_crush_part_visibility_result local_visibility;
    uint32_t flags;

    if (slot == NULL || env == NULL || camera_matrix_bits == NULL ||
        result == NULL || slot_size < STF_CRUSH_PART_SLOT_SIZE) {
        return false;
    }

    local_env = *env;
    memset(&local_visibility, 0, sizeof(local_visibility));
    flags = read_le32(slot + 0x24u);

    if ((flags & (UINT32_C(1) << 7u)) != 0u &&
        (flags & (UINT32_C(1) << 3u)) != 0u &&
        env->effect_active_914 == UINT32_C(0)) {
        stf_crush_part_visibility_world_input input;
        memset(&input, 0, sizeof(input));
        input.world_position_bits[0] = read_le32(slot + 0x00u);
        input.world_position_bits[1] = read_le32(slot + 0x04u);
        input.world_position_bits[2] = read_le32(slot + 0x08u);
        input.radius_bits = read_le32(slot + 0x18u);
        input.focus_distance_bits = focus_distance_bits;
        memcpy(input.camera_matrix_bits, camera_matrix_bits,
               sizeof(input.camera_matrix_bits));

        if (!stf_crush_part_visibility_from_matrix_model2(
                &input, &local_visibility
            )) {
            return false;
        }
        local_env.visibility_mask = local_visibility.mask;
    }

    if (visibility != NULL) {
        *visibility = local_visibility;
    }

    return stf_crush_part_update_position_model2(
        slot, slot_size, &local_env, result
    );
}


bool stf_crush_part_visibility_mask_model2(
    const stf_crush_part_visibility_input *input,
    stf_crush_part_visibility_result *result
)
{
    stf_crush_part_visibility_result local;
    float camera_x;
    float camera_y;
    float camera_z;
    float radius;
    float focus;
    float screen_x;
    float screen_y;
    float screen_radius;
    const float limit_x = 248.0f;
    const float limit_y = 192.0f;

    if (input == NULL || result == NULL) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    camera_x = bits_to_float(input->camera_x_bits);
    camera_y = bits_to_float(input->camera_y_bits);
    camera_z = bits_to_float(input->camera_z_bits);
    radius = bits_to_float(input->radius_bits);
    focus = bits_to_float(input->focus_distance_bits);

    if ((input->camera_z_bits & UINT32_C(0x80000000)) != 0u) {
        local.behind_camera = true;
        *result = local;
        return true;
    }

    if (camera_z == 0.0f) {
        return false;
    }

    screen_x = (camera_x * focus) / camera_z;
    screen_y = (camera_y * focus) / camera_z;
    screen_radius = (radius * focus) / camera_z;

    local.screen_x_bits = float_to_bits(screen_x);
    local.screen_y_bits = float_to_bits(screen_y);
    local.screen_radius_bits = float_to_bits(screen_radius);

    if (screen_x < limit_x && screen_x > -limit_x) {
        const float top = screen_y - screen_radius;
        const float bottom = screen_y + screen_radius;

        if (top < limit_y && top > -limit_y) {
            local.mask |= UINT32_C(1) << 0u;
        }
        if (bottom < limit_y && bottom > -limit_y) {
            local.mask |= UINT32_C(1) << 1u;
        }
    }

    if (screen_y < limit_y && screen_y > -limit_y) {
        const float left = screen_x - screen_radius;
        const float right = screen_x + screen_radius;

        if (left < limit_x && left > -limit_x) {
            local.mask |= UINT32_C(1) << 2u;
        }
        if (right < limit_x && right > -limit_x) {
            local.mask |= UINT32_C(1) << 3u;
        }
    }

    *result = local;
    return true;
}


bool stf_crush_part_visibility_from_matrix_model2(
    const stf_crush_part_visibility_world_input *input,
    stf_crush_part_visibility_result *result
)
{
    stf_crush_part_visibility_input camera_input;
    uint32_t transformed[3];

    if (input == NULL || result == NULL) {
        return false;
    }

    if (!stf_copro_scalar_transform_point_bits(
            input->camera_matrix_bits,
            input->world_position_bits,
            transformed
        )) {
        return false;
    }

    memset(&camera_input, 0, sizeof(camera_input));
    camera_input.camera_x_bits = transformed[0];
    camera_input.camera_y_bits = transformed[1];
    camera_input.camera_z_bits = transformed[2];
    camera_input.radius_bits = input->radius_bits;
    camera_input.focus_distance_bits = input->focus_distance_bits;

    return stf_crush_part_visibility_mask_model2(&camera_input, result);
}


static bool crush_bookkeeping_mode(uint8_t also_mode, uint8_t also_sub_mode)
{
    if (also_mode == UINT8_C(17)) {
        return true;
    }
    if (also_mode == UINT8_C(9)) {
        return also_sub_mode != UINT8_C(17) &&
               also_sub_mode != UINT8_C(23);
    }
    if (also_mode == UINT8_C(3)) {
        return also_sub_mode == UINT8_C(5) ||
               also_sub_mode == UINT8_C(9) ||
               also_sub_mode == UINT8_C(10) ||
               also_sub_mode == UINT8_C(14) ||
               also_sub_mode == UINT8_C(15) ||
               also_sub_mode == UINT8_C(16);
    }
    return false;
}

bool stf_crush_part_spawn_model2(
    uint8_t *slot,
    size_t slot_size,
    const stf_crush_part_spawn_input *input,
    stf_crush_part_spawn_result *result
)
{
    stf_crush_part_spawn_result local;
    const uint8_t *record;
    uint32_t flags;
    uint32_t spin_seed;
    uint8_t spin_index;
    int16_t angle_y;

    if (slot == NULL || input == NULL || result == NULL ||
        slot_size < STF_CRUSH_PART_SLOT_SIZE ||
        input->record == NULL ||
        input->record_size < STF_CRUSH_PART_RECORD_SIZE ||
        input->spin_table == NULL ||
        input->spin_table_count < STF_CRUSH_PART_SPIN_TABLE_COUNT) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    record = input->record;
    local.slot_was_free = read_le32(slot + 0x24u) == UINT32_C(0);
    if (!local.slot_was_free) {
        *result = local;
        return true;
    }

    flags = read_le32(record + 0x1Cu) |
            (uint32_t)input->fighter_flags_byte;
    flags |= UINT32_C(1) << 11u;

    write_le32(slot + 0x24u, flags);
    write_le32(slot + 0x00u, input->base_position[0]);
    write_le32(slot + 0x04u, input->base_position[1]);
    write_le32(slot + 0x08u, input->base_position[2]);

    write_le16(slot + 0x22u, (int16_t)read_le16u(record + 0x06u));
    write_le32(slot + 0x0Cu, input->velocity[0]);
    write_le32(slot + 0x10u, input->velocity[1]);
    write_le32(slot + 0x14u, input->velocity[2]);
    write_le32(slot + 0x18u, read_le32(record + 0x18u));

    write_le16(slot + 0x32u, (int16_t)input->part_index);
    write_le16(slot + 0x30u, (int16_t)input->record_index);
    write_le16(slot + 0x28u, read_le16s(record + 0x0Eu));
    write_le16(slot + 0x2Au, read_le16s(record + 0x10u));
    write_le16(slot + 0x2Cu, read_le16s(record + 0x12u));

    write_le16(slot + 0x1Cu, read_le16s(record + 0x08u));
    angle_y = add_wrap16(input->fighter_angle_y, read_le16s(record + 0x0Au));
    write_le16(slot + 0x1Eu, angle_y);
    write_le16(slot + 0x20u, (int16_t)read_le16u(record + 0x0Cu));

    write_le32(slot + 0x34u, read_le32(record + 0x14u));
    write_le32(slot + 0x40u, read_le32(record + 0x20u));
    write_le32(slot + 0x44u, read_le32(record + 0x24u));
    write_le32(slot + 0x38u, UINT32_C(0));
    write_le32(slot + 0x3Cu, UINT32_C(0));

    spin_seed = input->base_position[0] + read_le32(record + 0x14u);
    spin_seed += input->base_position[1];
    spin_seed += input->base_position[2];
    spin_index = (uint8_t)(spin_seed & UINT32_C(0x0F));
    write_le16(slot + 0x2Eu, input->spin_table[spin_index]);

    local.spawned = true;
    local.spin_index = spin_index;
    local.spin_value = input->spin_table[spin_index];
    local.flags = flags;
    local.object_id = read_le16u(record + 0x06u);
    *result = local;
    return true;
}

bool stf_crush_part_should_delete_weight_model2(
    uint8_t also_mode,
    uint8_t also_sub_mode
)
{
    if (also_mode == UINT8_C(17)) {
        return true;
    }

    if (also_mode == UINT8_C(9)) {
        return also_sub_mode != UINT8_C(16) &&
               also_sub_mode != UINT8_C(17) &&
               also_sub_mode != UINT8_C(22) &&
               also_sub_mode != UINT8_C(23);
    }

    if (also_mode == UINT8_C(3)) {
        return also_sub_mode == UINT8_C(5) ||
               also_sub_mode == UINT8_C(9) ||
               also_sub_mode == UINT8_C(10) ||
               also_sub_mode == UINT8_C(14) ||
               also_sub_mode == UINT8_C(15) ||
               also_sub_mode == UINT8_C(16);
    }

    return false;
}

bool stf_crush_part_delete_weight_model2(
    uint8_t *defender,
    size_t defender_size,
    const uint8_t *record,
    size_t record_size,
    uint8_t also_mode,
    uint8_t also_sub_mode,
    bool *applied
)
{
    float remaining;
    float base;
    float weight;

    if (defender == NULL || record == NULL || applied == NULL ||
        defender_size < 0x7E0u ||
        record_size < STF_CRUSH_PART_RECORD_SIZE) {
        return false;
    }

    *applied = false;
    if (!stf_crush_part_should_delete_weight_model2(
            also_mode, also_sub_mode)) {
        return true;
    }

    weight = bits_to_float(read_le32(record + 0x14u));
    remaining = read_f32(defender + 0x7D8u) - weight;
    base = read_f32(defender + 0x7DCu);
    write_f32(defender + 0x7D8u, remaining);
    write_f32(defender + 0x5D8u, base + remaining);
    *applied = true;
    return true;
}

bool stf_crush_part_bookkeeping_model2(
    uint8_t *defender,
    size_t defender_size,
    const uint8_t *record,
    size_t record_size,
    uint8_t part_index,
    uint8_t also_mode,
    uint8_t also_sub_mode,
    stf_crush_part_bookkeeping_result *result
)
{
    stf_crush_part_bookkeeping_result local;
    uint32_t stored_word;
    unsigned lane;

    if (defender == NULL || record == NULL || result == NULL ||
        defender_size < 0x1F68u ||
        record_size < STF_CRUSH_PART_RECORD_SIZE ||
        part_index >= 16u) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    if (!crush_bookkeeping_mode(also_mode, also_sub_mode)) {
        *result = local;
        return true;
    }

    stored_word = (uint32_t)(int32_t)read_le16s(record + 0x04u);
    write_le32(defender + 0x40u + (size_t)part_index * 4u, stored_word);

    for (lane = 0u; lane < 3u; ++lane) {
        uint8_t *word = defender + 0x1F60u + lane * 2u;
        uint16_t bits = read_le16u(word);
        if ((bits & (uint16_t)(UINT16_C(1) << part_index)) == 0u) {
            bits = (uint16_t)(bits | (uint16_t)(UINT16_C(1) << part_index));
            write_le16(word, (int16_t)bits);
            local.history_lane = (uint8_t)lane;
            local.applied = true;
            local.stored_record_word = stored_word;
            *result = local;
            return true;
        }
    }

    {
        uint8_t *word = defender + 0x1F66u;
        uint16_t bits = read_le16u(word);
        bits = (uint16_t)(bits | (uint16_t)(UINT16_C(1) << part_index));
        write_le16(word, (int16_t)bits);
    }

    local.history_lane = UINT8_C(3);
    local.applied = true;
    local.stored_record_word = stored_word;
    *result = local;
    return true;
}


bool stf_crush_part_build_speed_requests_model2(
    const stf_crush_part_speed_input *input,
    stf_crush_part_speed_request *requests,
    size_t request_capacity,
    stf_crush_part_speed_result *result
)
{
    stf_crush_part_speed_result local;
    unsigned iterations;
    unsigned i;
    const unsigned height =
        input != NULL && input->body_height_83d < UINT8_C(70)
            ? input->body_height_83d
            : 70u;
    float radial;

    if (input == NULL || result == NULL ||
        input->records == NULL ||
        input->tables == NULL ||
        input->profile_843 >= UINT8_C(6)) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    if (input->count > STF_CRUSH_PART_MAX_SPEEDS) {
        local.count_rejected = true;
        *result = local;
        return true;
    }

    iterations = input->count == UINT8_C(0) ? 1u : input->count;
    if (requests == NULL || request_capacity < iterations ||
        input->records_size < (size_t)iterations * STF_CRUSH_PART_RECORD_SIZE) {
        return false;
    }

    radial =
        ((float)height / 60.0f) *
        bits_to_float(input->tables->radial_profile_bits[input->profile_843]);

    for (i = 0u; i < iterations; ++i) {
        const uint8_t *record =
            input->records + (size_t)i * STF_CRUSH_PART_RECORD_SIZE;
        const uint16_t object_id = read_le16u(record + 0x06u);
        uint32_t seed =
            (uint32_t)(int32_t)input->fighter_angle_26 +
            (uint32_t)(int32_t)input->fighter_angle_82a;
        uint8_t index;
        float vertical;
        const float multiplier =
            input->effect_active_914 == UINT32_C(1) ? 1.5f : 1.0f;
        const float jitter = bits_to_float(input->tables->jitter_bits[
            (seed + input->part_index + object_id +
             read_le32(record + 0x14u)) & UINT32_C(0x0F)
        ]);

        seed += input->part_index;
        seed += object_id;
        seed += read_le32(record + 0x14u);
        index = (uint8_t)(seed & UINT32_C(0x0F));

        vertical =
            (((float)height / 60.0f) *
                bits_to_float(
                    input->tables->vertical_profile_bits[input->profile_843]
                ) +
             bits_to_float(UINT32_C(0x3D75C28F))) *
            multiplier;

        radial += jitter;
        vertical += jitter;
        vertical += jitter;

        requests[i].jitter_index = index;
        requests[i].angle = (uint16_t)(
            (uint16_t)input->fighter_angle_26 +
            (uint16_t)input->fighter_angle_82a -
            (uint16_t)input->tables->angle_offsets[index]
        );
        requests[i].radial_speed_bits = float_to_bits(radial);
        requests[i].vertical_speed_bits = float_to_bits(vertical);
    }

    local.generated = (uint8_t)iterations;
    *result = local;
    return true;
}

bool stf_crush_part_resolve_speed_model2(
    const stf_crush_part_speed_request *request,
    uint32_t command24_output_bits,
    uint32_t command25_output_bits,
    uint32_t velocity_bits[3]
)
{
    if (request == NULL || velocity_bits == NULL) {
        return false;
    }

    velocity_bits[0] = command24_output_bits ^ UINT32_C(0x80000000);
    velocity_bits[1] = request->vertical_speed_bits;
    velocity_bits[2] = command25_output_bits;
    return true;
}

bool stf_crush_part_resolve_speed_semantic_model2(
    const stf_crush_part_speed_request *request,
    uint32_t velocity_bits[3]
)
{
    uint32_t command24_output_bits;
    uint32_t command25_output_bits;

    if (request == NULL || velocity_bits == NULL) {
        return false;
    }

    if (!stf_copro_scalar_sin_scale_bits(
            request->angle,
            request->radial_speed_bits,
            &command24_output_bits
        ) ||
        !stf_copro_scalar_cos_scale_bits(
            request->angle,
            request->radial_speed_bits,
            &command25_output_bits
        )) {
        return false;
    }

    return stf_crush_part_resolve_speed_model2(
        request,
        command24_output_bits,
        command25_output_bits,
        velocity_bits
    );
}


static uint8_t crush_remaining_parts_count(const uint8_t *slot)
{
    const uint32_t flags = read_le32(slot + 0x24u);
    return ((flags & (UINT32_C(1) << 11u)) != 0u &&
            (flags & (UINT32_C(1) << 3u)) != 0u)
        ? UINT8_C(1)
        : UINT8_C(0);
}

bool stf_crush_part_set_model2(
    uint8_t *defender,
    size_t defender_size,
    uint8_t *slot,
    size_t slot_size,
    const stf_crush_part_set_input *input,
    stf_crush_part_set_result *result
)
{
    stf_crush_part_set_result local;
    unsigned iterations;
    unsigned i;
    const uint8_t *first_record;

    if (defender == NULL || slot == NULL || input == NULL || result == NULL ||
        input->records == NULL || input->velocities == NULL ||
        input->spin_table == NULL ||
        slot_size < STF_CRUSH_PART_SLOT_SIZE ||
        defender_size < 0x1F68u ||
        input->part_index >= 16u ||
        input->spin_table_count < STF_CRUSH_PART_SPIN_TABLE_COUNT ||
        input->count > STF_CRUSH_PART_MAX_SPEEDS) {
        return false;
    }

    iterations = input->count == UINT8_C(0) ? 1u : input->count;
    if (input->records_size <
            (size_t)iterations * STF_CRUSH_PART_RECORD_SIZE ||
        input->velocity_count < iterations) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    first_record = input->records;

    if ((read_le32(first_record + 0x1Cu) &
         (UINT32_C(1) << 2u)) != 0u) {
        uint32_t bits = read_le32(defender + 0x1F40u);
        bits |= UINT32_C(1) << input->part_index;
        write_le32(defender + 0x1F40u, bits);
        local.marked_part_1f40 = true;
    }

    for (i = 0u; i < iterations; ++i) {
        const uint8_t *record =
            input->records + (size_t)i * STF_CRUSH_PART_RECORD_SIZE;
        const uint32_t combined_flags =
            read_le32(record + 0x1Cu) | (uint32_t)defender[0x04u];
        bool weight_applied = false;

        ++local.iterations_entered;
        if (!stf_crush_part_delete_weight_model2(
                defender, defender_size,
                record, STF_CRUSH_PART_RECORD_SIZE,
                input->also_mode, input->also_sub_mode,
                &weight_applied)) {
            return false;
        }
        if (weight_applied) {
            ++local.weights_applied;
        }

        if ((combined_flags & (UINT32_C(1) << 3u)) != 0u) {
            if ((combined_flags & (UINT32_C(1) << 1u)) == 0u) {
                local.spawn_gate_rejected = true;
                break;
            }

            if (input->effect_active_914 != UINT32_C(1) &&
                crush_remaining_parts_count(slot) >= UINT8_C(2)) {
                local.spawn_gate_rejected = true;
                break;
            }
        }

        if (read_le32(slot + 0x24u) != UINT32_C(0)) {
            local.slot_occupied_break = true;
            break;
        }

        {
            stf_crush_part_spawn_input spawn_input;
            stf_crush_part_spawn_result spawn_result;
            const size_t position_offset =
                0x1F4u + (size_t)input->part_index * 0x0Cu;

            memset(&spawn_input, 0, sizeof(spawn_input));
            spawn_input.record = record;
            spawn_input.record_size = STF_CRUSH_PART_RECORD_SIZE;
            spawn_input.base_position[0] =
                read_le32(defender + position_offset + 0u);
            spawn_input.base_position[1] =
                read_le32(defender + position_offset + 4u);
            spawn_input.base_position[2] =
                read_le32(defender + position_offset + 8u);
            spawn_input.velocity[0] = input->velocities[i][0];
            spawn_input.velocity[1] = input->velocities[i][1];
            spawn_input.velocity[2] = input->velocities[i][2];
            spawn_input.part_index = input->part_index;
            spawn_input.record_index = input->record_index;
            spawn_input.fighter_flags_byte = defender[0x04u];
            spawn_input.fighter_angle_y = read_le16s(defender + 0x26u);
            spawn_input.spin_table = input->spin_table;
            spawn_input.spin_table_count = input->spin_table_count;

            if (!stf_crush_part_spawn_model2(
                    slot, slot_size, &spawn_input, &spawn_result)) {
                return false;
            }

            local.spawn = spawn_result;
            if (spawn_result.spawned) {
                ++local.spawned_count;
            }
        }
    }

    if (!stf_crush_part_bookkeeping_model2(
            defender, defender_size,
            first_record, STF_CRUSH_PART_RECORD_SIZE,
            input->part_index,
            input->also_mode, input->also_sub_mode,
            &local.bookkeeping)) {
        return false;
    }

    *result = local;
    return true;
}


bool stf_crush_part_put_model2(
    uint8_t *defender,
    size_t defender_size,
    uint8_t *slot,
    size_t slot_size,
    const stf_crush_part_put_input *input,
    stf_crush_part_put_result *result
)
{
    stf_crush_part_put_result local;
    stf_crush_part_speed_request requests[STF_CRUSH_PART_MAX_SPEEDS];
    stf_crush_part_set_input set_input;
    size_t i;

    if (defender == NULL || slot == NULL || input == NULL || result == NULL ||
        input->spin_table == NULL ||
        input->spin_table_count < STF_CRUSH_PART_SPIN_TABLE_COUNT) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    memset(requests, 0, sizeof(requests));
    memset(&set_input, 0, sizeof(set_input));

    if (!stf_crush_part_build_speed_requests_model2(
            &input->speed,
            requests,
            STF_CRUSH_PART_MAX_SPEEDS,
            &local.speed
        )) {
        return false;
    }

    if (local.speed.count_rejected) {
        *result = local;
        return true;
    }

    for (i = 0u; i < local.speed.generated; ++i) {
        if (!stf_crush_part_resolve_speed_semantic_model2(
                &requests[i],
                local.velocities[i]
            )) {
            return false;
        }
    }

    set_input.records = input->speed.records;
    set_input.records_size = input->speed.records_size;
    set_input.velocities = local.velocities;
    set_input.velocity_count = local.speed.generated;
    set_input.count = input->speed.count;
    set_input.part_index = input->speed.part_index;
    set_input.record_index = input->record_index;
    set_input.effect_active_914 = input->speed.effect_active_914;
    set_input.also_mode = input->also_mode;
    set_input.also_sub_mode = input->also_sub_mode;
    set_input.spin_table = input->spin_table;
    set_input.spin_table_count = input->spin_table_count;

    if (!stf_crush_part_set_model2(
            defender,
            defender_size,
            slot,
            slot_size,
            &set_input,
            &local.set
        )) {
        return false;
    }

    *result = local;
    return true;
}


bool stf_crush_part_floor_sound_select_model2(
    uint32_t slot_flags,
    stf_crush_part_floor_sound_result *result
)
{
    stf_crush_part_floor_sound_result local;
    uint32_t nibble;
    int bit;

    if (result == NULL) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    nibble = (slot_flags >> 28u) & UINT32_C(0x0F);
    if (nibble == UINT32_C(0)) {
        *result = local;
        return true;
    }

    for (bit = 3; bit >= 0; --bit) {
        if ((nibble & (UINT32_C(1) << (unsigned)bit)) != 0u) {
            local.request_sound = true;
            local.table_index = (uint8_t)bit;
            *result = local;
            return true;
        }
    }

    *result = local;
    return true;
}
