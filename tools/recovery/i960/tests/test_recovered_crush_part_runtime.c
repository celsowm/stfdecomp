#include <stdint.h>
#include <string.h>

#include "crush_part_runtime.h"

static void write_le16(uint8_t *p, int16_t v)
{
    const uint16_t u = (uint16_t)v;
    p[0] = (uint8_t)u;
    p[1] = (uint8_t)(u >> 8u);
}

static void write_le32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8u);
    p[2] = (uint8_t)(v >> 16u);
    p[3] = (uint8_t)(v >> 24u);
}

static int test_free_spin_branch(void)
{
    uint8_t slot[STF_CRUSH_PART_SLOT_SIZE];
    stf_crush_part_angle_result result;

    memset(slot, 0, sizeof(slot));
    write_le16(slot + 0x1Cu, INT16_C(100));
    write_le16(slot + 0x1Eu, INT16_C(200));
    write_le16(slot + 0x20u, INT16_C(300));
    write_le16(slot + 0x2Eu, INT16_C(25));
    write_le32(slot + 0x24u,
               (UINT32_C(1) << 12u) | (UINT32_C(1) << 14u));
    write_le32(slot + 0x3Cu, UINT32_C(2));

    if (!stf_crush_part_update_angles_model2(slot, sizeof(slot), &result) ||
        result.angle_x != INT16_C(125) ||
        result.angle_y != INT16_C(225) ||
        result.angle_z != INT16_C(325) ||
        result.step != INT16_C(0x1000)) {
        return 1;
    }
    return 0;
}

static int test_target_approach_branch(void)
{
    uint8_t slot[STF_CRUSH_PART_SLOT_SIZE];
    stf_crush_part_angle_result result;

    memset(slot, 0, sizeof(slot));
    write_le16(slot + 0x1Cu, INT16_C(10000));
    write_le16(slot + 0x1Eu, INT16_C(10000));
    write_le16(slot + 0x20u, INT16_C(-10000));
    write_le16(slot + 0x28u, INT16_C(0));
    write_le16(slot + 0x2Au, INT16_C(-10000));
    write_le16(slot + 0x2Cu, INT16_C(10000));
    write_le32(slot + 0x24u, UINT32_C(1) << 7u);
    write_le32(slot + 0x3Cu, UINT32_C(3));

    if (!stf_crush_part_update_angles_model2(slot, sizeof(slot), &result) ||
        result.angle_x != INT16_C(10000) ||
        result.angle_y != INT16_C(10512) ||
        result.angle_z != INT16_C(-10512) ||
        result.step != INT16_C(0x0200)) {
        return 1;
    }
    return 0;
}

static int test_draw_extraction(void)
{
    uint8_t slot[STF_CRUSH_PART_SLOT_SIZE];
    stf_crush_part_draw draw;

    memset(slot, 0, sizeof(slot));
    write_le16(slot + 0x22u, INT16_C(777));
    write_le32(slot + 0x24u, (UINT32_C(1) << 19u) | UINT32_C(1));
    write_le32(slot + 0x00u, UINT32_C(0x11111111));
    write_le32(slot + 0x04u, UINT32_C(0x22222222));
    write_le32(slot + 0x08u, UINT32_C(0x33333333));
    write_le16(slot + 0x1Cu, INT16_C(10));
    write_le16(slot + 0x1Eu, INT16_C(20));
    write_le16(slot + 0x20u, INT16_C(30));

    if (!stf_crush_part_build_draw_model2(slot, sizeof(slot), &draw) ||
        !draw.active ||
        !draw.use_saved_graphics_state ||
        draw.owner_index != UINT8_C(1) ||
        draw.object_id != UINT16_C(777) ||
        draw.position[0] != UINT32_C(0x11111111) ||
        draw.position[1] != UINT32_C(0x22222222) ||
        draw.position[2] != UINT32_C(0x33333333) ||
        draw.angle_x != INT16_C(10) ||
        draw.angle_y != INT16_C(20) ||
        draw.angle_z != INT16_C(30)) {
        return 1;
    }
    return 0;
}

static int test_position_air_integration(void);
static int test_floor_bounce_and_stop(void);
static int test_wall_reflection(void);
static int test_dormant_visibility_deactivation(void);

int main(void)
{
    if (test_free_spin_branch() != 0) return 1;
    if (test_target_approach_branch() != 0) return 1;
    if (test_draw_extraction() != 0) return 1;
    if (test_position_air_integration() != 0) return 1;
    if (test_floor_bounce_and_stop() != 0) return 1;
    if (test_wall_reflection() != 0) return 1;
    if (test_dormant_visibility_deactivation() != 0) return 1;
    return 0;
}


static uint32_t fbits(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static float read_f32(const uint8_t *p)
{
    uint32_t bits =
        (uint32_t)p[0] |
        ((uint32_t)p[1] << 8u) |
        ((uint32_t)p[2] << 16u) |
        ((uint32_t)p[3] << 24u);
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static void write_f32(uint8_t *p, float value)
{
    write_le32(p, fbits(value));
}

static int test_position_air_integration(void)
{
    uint8_t slot[STF_CRUSH_PART_SLOT_SIZE];
    stf_crush_part_physics_env env;
    stf_crush_part_physics_result result;

    memset(slot, 0, sizeof(slot));
    memset(&env, 0, sizeof(env));

    write_f32(slot + 0x00u, 1.0f);
    write_f32(slot + 0x04u, 5.0f);
    write_f32(slot + 0x08u, 2.0f);
    write_f32(slot + 0x0Cu, 1.0f);
    write_f32(slot + 0x10u, 2.0f);
    write_f32(slot + 0x14u, -1.0f);
    write_f32(slot + 0x40u, -100.0f);
    env.gravity_bits = fbits(0.5f);
    env.stage_x_bits = fbits(100.0f);
    env.cage_height_bits = fbits(100.0f);

    if (!stf_crush_part_update_position_model2(
            slot, sizeof(slot), &env, &result
        ) ||
        result.floor_hit ||
        read_f32(slot + 0x00u) < 1.979f ||
        read_f32(slot + 0x00u) > 1.981f ||
        read_f32(slot + 0x04u) < 6.499f ||
        read_f32(slot + 0x04u) > 6.501f ||
        read_f32(slot + 0x08u) < 1.019f ||
        read_f32(slot + 0x08u) > 1.021f) {
        return 1;
    }

    return 0;
}

static int test_floor_bounce_and_stop(void)
{
    uint8_t slot[STF_CRUSH_PART_SLOT_SIZE];
    stf_crush_part_physics_env env;
    stf_crush_part_physics_result result;

    memset(slot, 0, sizeof(slot));
    memset(&env, 0, sizeof(env));

    write_f32(slot + 0x04u, 0.01f);
    write_f32(slot + 0x10u, -0.001f);
    write_f32(slot + 0x34u, 1.0f);
    write_f32(slot + 0x40u, 0.0f);
    env.gravity_bits = fbits(0.001f);
    env.stage_x_bits = fbits(100.0f);
    env.cage_height_bits = fbits(100.0f);

    if (!stf_crush_part_update_position_model2(
            slot, sizeof(slot), &env, &result
        ) ||
        !result.floor_hit ||
        !result.request_floor_effect ||
        !result.stopped_bouncing ||
        result.ground_contacts != UINT32_C(1) ||
        (result.flags & (UINT32_C(1) << 7u)) == 0u ||
        read_f32(slot + 0x0Cu) != 0.0f ||
        read_f32(slot + 0x10u) != 0.0f ||
        read_f32(slot + 0x14u) != 0.0f) {
        return 1;
    }

    return 0;
}

static int test_wall_reflection(void)
{
    uint8_t slot[STF_CRUSH_PART_SLOT_SIZE];
    stf_crush_part_physics_env env;
    stf_crush_part_physics_result result;

    memset(slot, 0, sizeof(slot));
    memset(&env, 0, sizeof(env));

    write_f32(slot + 0x00u, 9.5f);
    write_f32(slot + 0x04u, 0.0f);
    write_f32(slot + 0x08u, 0.0f);
    write_f32(slot + 0x0Cu, 1.0f);
    write_f32(slot + 0x18u, 1.0f);
    write_f32(slot + 0x40u, -100.0f);

    env.gravity_bits = fbits(0.0f);
    env.stage_x_bits = fbits(10.0f);
    env.cage_height_bits = fbits(100.0f);

    if (!stf_crush_part_update_position_model2(
            slot, sizeof(slot), &env, &result
        ) ||
        !result.hit_x_wall ||
        read_f32(slot + 0x00u) < 8.999f ||
        read_f32(slot + 0x00u) > 9.001f ||
        read_f32(slot + 0x0Cu) > -0.293f ||
        read_f32(slot + 0x0Cu) < -0.295f) {
        return 1;
    }

    return 0;
}

static int test_dormant_visibility_deactivation(void)
{
    uint8_t slot[STF_CRUSH_PART_SLOT_SIZE];
    stf_crush_part_physics_env env;
    stf_crush_part_physics_result result;

    memset(slot, 0, sizeof(slot));
    memset(&env, 0, sizeof(env));

    write_le16(slot + 0x22u, INT16_C(9));
    write_le32(slot + 0x24u,
               (UINT32_C(1) << 7u) | (UINT32_C(1) << 3u));
    env.effect_active_914 = UINT32_C(0);
    env.visibility_mask = UINT32_C(0);

    if (!stf_crush_part_update_position_model2(
            slot, sizeof(slot), &env, &result
        ) ||
        !result.deactivated ||
        slot[0x22u] != UINT8_C(0) ||
        slot[0x23u] != UINT8_C(0) ||
        result.flags != UINT32_C(0)) {
        return 1;
    }

    return 0;
}
