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

static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8u) |
           ((uint32_t)p[2] << 16u) |
           ((uint32_t)p[3] << 24u);
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

static int test_oidasi_correction(void);
static int test_composed_frame_transaction(void);
static int test_position_air_integration(void);
static int test_floor_bounce_and_stop(void);
static int test_wall_reflection(void);
static int test_dormant_visibility_deactivation(void);
static int test_visibility_mask_centered(void);
static int test_visibility_mask_partial_edges(void);
static int test_visibility_behind_camera(void);
static int test_visibility_from_matrix(void);
static int test_spawn_from_rom_record(void);
static int test_spawn_respects_occupied_slot(void);
static int test_delete_weight_mode_gate(void);
static int test_spawn_bookkeeping_lane_selection(void);
static int test_speed_request_generation(void);
static int test_speed_semantic_resolution(void);
static int test_speed_request_count_limit(void);
static int test_composed_crush_part_set(void);
static int test_composed_crush_part_gate_reject(void);
static int test_floor_sound_selection(void);
static int test_crush_part_lifecycle_flow(void);

int main(void)
{
    if (test_free_spin_branch() != 0) return 1;
    if (test_target_approach_branch() != 0) return 1;
    if (test_draw_extraction() != 0) return 1;
    if (test_oidasi_correction() != 0) return 1;
    if (test_composed_frame_transaction() != 0) return 1;
    if (test_position_air_integration() != 0) return 1;
    if (test_floor_bounce_and_stop() != 0) return 1;
    if (test_wall_reflection() != 0) return 1;
    if (test_dormant_visibility_deactivation() != 0) return 1;
    if (test_visibility_mask_centered() != 0) return 1;
    if (test_visibility_mask_partial_edges() != 0) return 1;
    if (test_visibility_behind_camera() != 0) return 1;
    if (test_visibility_from_matrix() != 0) return 1;
    if (test_spawn_from_rom_record() != 0) return 1;
    if (test_spawn_respects_occupied_slot() != 0) return 1;
    if (test_delete_weight_mode_gate() != 0) return 1;
    if (test_spawn_bookkeeping_lane_selection() != 0) return 1;
    if (test_speed_request_generation() != 0) return 1;
    if (test_speed_semantic_resolution() != 0) return 1;
    if (test_speed_request_count_limit() != 0) return 1;
    if (test_composed_crush_part_set() != 0) return 1;
    if (test_composed_crush_part_gate_reject() != 0) return 1;
    if (test_floor_sound_selection() != 0) return 1;
    if (test_crush_part_lifecycle_flow() != 0) return 1;
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

static int test_oidasi_correction(void)
{
    uint8_t slot[STF_CRUSH_PART_SLOT_SIZE];
    stf_crush_part_oidasi_input input;
    stf_crush_part_oidasi_result result;

    memset(slot, 0, sizeof(slot));
    memset(&input, 0, sizeof(input));

    write_f32(slot + 0x00u, 1.0f);
    write_f32(slot + 0x04u, 2.0f);
    write_f32(slot + 0x08u, 3.0f);
    write_f32(slot + 0x18u, 4.0f);

    input.command77.words[0] = fbits(10.0f);
    input.command77.words[1] = fbits(-5.0f);

    if (!stf_crush_part_oidasi_model2(
            slot, sizeof(slot), &input, &result
        ) ||
        result.skipped ||
        !result.applied ||
        read_f32(slot + 0x00u) < 2.599f ||
        read_f32(slot + 0x00u) > 2.601f ||
        read_f32(slot + 0x04u) != 2.0f ||
        read_f32(slot + 0x08u) < 2.199f ||
        read_f32(slot + 0x08u) > 2.201f) {
        return 1;
    }

    input.fighter0_parts_locked = true;
    if (!stf_crush_part_oidasi_model2(
            slot, sizeof(slot), &input, &result
        ) ||
        !result.skipped ||
        result.applied ||
        read_f32(slot + 0x00u) < 2.599f ||
        read_f32(slot + 0x00u) > 2.601f ||
        read_f32(slot + 0x08u) < 2.199f ||
        read_f32(slot + 0x08u) > 2.201f) {
        return 2;
    }

    return 0;
}


static int test_composed_frame_transaction(void)
{
    uint8_t slot[STF_CRUSH_PART_SLOT_SIZE];
    stf_crush_part_frame_input input;
    stf_crush_part_frame_result result;
    stf_copro_command77_collision_state collision;
    uint32_t camera_matrix[12];
    size_t i;

    memset(slot, 0, sizeof(slot));
    memset(&input, 0, sizeof(input));
    memset(&collision, 0, sizeof(collision));
    memset(camera_matrix, 0, sizeof(camera_matrix));

    for (i = 0u; i < 12u; ++i) {
        camera_matrix[i] = fbits(0.0f);
    }
    camera_matrix[0] = fbits(1.0f);
    camera_matrix[4] = fbits(1.0f);
    camera_matrix[8] = fbits(1.0f);
    camera_matrix[11] = fbits(10.0f);

    write_f32(slot + 0x00u, 1.0f);
    write_f32(slot + 0x04u, 5.0f);
    write_f32(slot + 0x08u, 2.0f);
    write_f32(slot + 0x0Cu, 0.0f);
    write_f32(slot + 0x10u, 0.0f);
    write_f32(slot + 0x14u, 0.0f);
    write_f32(slot + 0x18u, 1.0f);
    write_f32(slot + 0x40u, -100.0f);
    write_le16(slot + 0x22u, INT16_C(77));
    write_le16(slot + 0x2Eu, INT16_C(32));
    write_le32(slot + 0x24u, UINT32_C(1) << 12u);

    collision.balls[0][13].position_bits[0] = fbits(1.0f);
    collision.balls[0][13].position_bits[1] = fbits(5.0f);
    collision.balls[0][13].position_bits[2] = fbits(2.0f);
    collision.balls[0][0].position_bits[0] = fbits(1.5f);
    collision.balls[0][0].position_bits[1] = fbits(5.0f);
    collision.balls[0][0].position_bits[2] = fbits(2.0f);
    collision.balls[0][0].radius_bits = fbits(1.0f);
    collision.balls[0][0].unit_index = UINT8_C(4);
    collision.balls[1][13].position_bits[0] = fbits(20.0f);

    input.command77_collision = &collision;
    input.physics.gravity_bits = fbits(0.0f);
    input.physics.stage_x_bits = fbits(100.0f);
    input.physics.cage_height_bits = fbits(100.0f);
    input.camera_matrix_bits = camera_matrix;
    input.focus_distance_bits = fbits(100.0f);

    if (!stf_crush_part_frame_model2(
            slot, sizeof(slot), &input, &result
        ) ||
        !result.oidasi.applied ||
        result.physics.deactivated ||
        !result.angle_updated ||
        read_f32(slot + 0x00u) < 1.039f ||
        read_f32(slot + 0x00u) > 1.041f ||
        read_f32(slot + 0x08u) < 1.999f ||
        read_f32(slot + 0x08u) > 2.001f ||
        result.angles.angle_x != INT16_C(32) ||
        result.angles.angle_y != INT16_C(32)) {
        return 1;
    }

    return 0;
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

    write_f32(slot + 0x04u, 0.001f);
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
    stf_crush_part_visibility_result visibility;
    uint32_t camera_matrix[12];
    size_t i;

    memset(slot, 0, sizeof(slot));
    memset(&env, 0, sizeof(env));
    memset(camera_matrix, 0, sizeof(camera_matrix));

    for (i = 0u; i < 12u; ++i) {
        camera_matrix[i] = fbits(0.0f);
    }
    camera_matrix[0] = fbits(1.0f);
    camera_matrix[4] = fbits(1.0f);
    camera_matrix[8] = fbits(1.0f);
    camera_matrix[11] = fbits(10.0f);

    write_f32(slot + 0x00u, 30.0f);
    write_f32(slot + 0x04u, 30.0f);
    write_f32(slot + 0x08u, 0.0f);
    write_f32(slot + 0x18u, 1.0f);
    write_le16(slot + 0x22u, INT16_C(9));
    write_le32(slot + 0x24u,
               (UINT32_C(1) << 7u) | (UINT32_C(1) << 3u));
    env.effect_active_914 = UINT32_C(0);

    if (!stf_crush_part_update_position_with_visibility_model2(
            slot, sizeof(slot), &env,
            camera_matrix, fbits(100.0f),
            &result, &visibility
        ) ||
        visibility.mask != UINT32_C(0) ||
        !result.deactivated ||
        slot[0x22u] != UINT8_C(0) ||
        slot[0x23u] != UINT8_C(0) ||
        result.flags != UINT32_C(0)) {
        return 1;
    }

    return 0;
}


static int test_visibility_mask_centered(void)
{
    stf_crush_part_visibility_input input;
    stf_crush_part_visibility_result result;

    memset(&input, 0, sizeof(input));
    input.camera_x_bits = fbits(0.0f);
    input.camera_y_bits = fbits(0.0f);
    input.camera_z_bits = fbits(10.0f);
    input.radius_bits = fbits(1.0f);
    input.focus_distance_bits = fbits(100.0f);

    if (!stf_crush_part_visibility_mask_model2(&input, &result) ||
        result.behind_camera ||
        result.mask != UINT32_C(0x0F) ||
        result.screen_x_bits != fbits(0.0f) ||
        result.screen_y_bits != fbits(0.0f)) {
        return 1;
    }

    return 0;
}

static int test_visibility_mask_partial_edges(void)
{
    stf_crush_part_visibility_input input;
    stf_crush_part_visibility_result result;

    memset(&input, 0, sizeof(input));
    input.camera_x_bits = fbits(20.0f);
    input.camera_y_bits = fbits(0.0f);
    input.camera_z_bits = fbits(10.0f);
    input.radius_bits = fbits(10.0f);
    input.focus_distance_bits = fbits(100.0f);

    if (!stf_crush_part_visibility_mask_model2(&input, &result) ||
        result.mask != ((UINT32_C(1) << 0u) |
                        (UINT32_C(1) << 1u) |
                        (UINT32_C(1) << 2u))) {
        return 1;
    }

    return 0;
}

static int test_visibility_behind_camera(void)
{
    stf_crush_part_visibility_input input;
    stf_crush_part_visibility_result result;

    memset(&input, 0, sizeof(input));
    input.camera_z_bits = fbits(-1.0f);
    input.focus_distance_bits = fbits(100.0f);

    if (!stf_crush_part_visibility_mask_model2(&input, &result) ||
        !result.behind_camera ||
        result.mask != UINT32_C(0)) {
        return 1;
    }

    return 0;
}


static int test_visibility_from_matrix(void)
{
    stf_crush_part_visibility_world_input input;
    stf_crush_part_visibility_result result;
    size_t i;

    memset(&input, 0, sizeof(input));
    for (i = 0u; i < 12u; ++i) {
        input.camera_matrix_bits[i] = fbits(0.0f);
    }

    input.camera_matrix_bits[0] = fbits(1.0f);
    input.camera_matrix_bits[4] = fbits(1.0f);
    input.camera_matrix_bits[8] = fbits(1.0f);
    input.camera_matrix_bits[11] = fbits(10.0f);
    input.world_position_bits[0] = fbits(0.0f);
    input.world_position_bits[1] = fbits(0.0f);
    input.world_position_bits[2] = fbits(0.0f);
    input.radius_bits = fbits(1.0f);
    input.focus_distance_bits = fbits(100.0f);

    if (!stf_crush_part_visibility_from_matrix_model2(&input, &result) ||
        result.behind_camera ||
        result.mask != UINT32_C(0x0F) ||
        result.screen_x_bits != fbits(0.0f) ||
        result.screen_y_bits != fbits(0.0f)) {
        return 1;
    }

    input.camera_matrix_bits[11] = fbits(-10.0f);
    if (!stf_crush_part_visibility_from_matrix_model2(&input, &result) ||
        !result.behind_camera ||
        result.mask != UINT32_C(0)) {
        return 2;
    }

    return 0;
}


static int test_spawn_from_rom_record(void)
{
    uint8_t slot[STF_CRUSH_PART_SLOT_SIZE];
    uint8_t record[STF_CRUSH_PART_RECORD_SIZE];
    int16_t spin_table[STF_CRUSH_PART_SPIN_TABLE_COUNT];
    stf_crush_part_spawn_input input;
    stf_crush_part_spawn_result result;
    unsigned i;

    memset(slot, 0, sizeof(slot));
    memset(record, 0, sizeof(record));
    memset(&input, 0, sizeof(input));

    for (i = 0u; i < STF_CRUSH_PART_SPIN_TABLE_COUNT; ++i) {
        spin_table[i] = (int16_t)(i * 10);
    }

    write_le16(record + 0x04u, INT16_C(-2));
    write_le16(record + 0x06u, INT16_C(777));
    write_le16(record + 0x08u, INT16_C(10));
    write_le16(record + 0x0Au, INT16_C(20));
    write_le16(record + 0x0Cu, INT16_C(30));
    write_le16(record + 0x0Eu, INT16_C(100));
    write_le16(record + 0x10u, INT16_C(200));
    write_le16(record + 0x12u, INT16_C(300));
    write_f32(record + 0x14u, 0.5f);
    write_f32(record + 0x18u, 1.0f);
    write_le32(record + 0x1Cu, UINT32_C(1) << 3u);
    write_f32(record + 0x20u, -1.0f);
    write_le32(record + 0x24u, UINT32_C(0x12345678));

    input.record = record;
    input.record_size = sizeof(record);
    input.base_position[0] = UINT32_C(1);
    input.base_position[1] = UINT32_C(2);
    input.base_position[2] = UINT32_C(3);
    input.velocity[0] = UINT32_C(0x3F800000);
    input.velocity[1] = UINT32_C(0x40000000);
    input.velocity[2] = UINT32_C(0x40400000);
    input.part_index = UINT8_C(4);
    input.record_index = UINT8_C(2);
    input.fighter_flags_byte = UINT8_C(1);
    input.fighter_angle_y = INT16_C(100);
    input.spin_table = spin_table;
    input.spin_table_count = STF_CRUSH_PART_SPIN_TABLE_COUNT;

    if (!stf_crush_part_spawn_model2(
            slot, sizeof(slot), &input, &result
        ) ||
        !result.slot_was_free ||
        !result.spawned ||
        result.object_id != UINT16_C(777) ||
        result.spin_index != UINT8_C(6) ||
        result.spin_value != INT16_C(60) ||
        (result.flags & (UINT32_C(1) << 11u)) == 0u ||
        (result.flags & (UINT32_C(1) << 3u)) == 0u ||
        (result.flags & UINT32_C(1)) == 0u ||
        slot[0x00u] != UINT8_C(1) ||
        slot[0x04u] != UINT8_C(2) ||
        slot[0x08u] != UINT8_C(3) ||
        read_f32(slot + 0x0Cu) != 1.0f ||
        read_f32(slot + 0x10u) != 2.0f ||
        read_f32(slot + 0x14u) != 3.0f ||
        read_f32(slot + 0x18u) != 1.0f ||
        ((uint16_t)slot[0x1Eu] | ((uint16_t)slot[0x1Fu] << 8u)) != UINT16_C(120) ||
        ((uint16_t)slot[0x2Eu] | ((uint16_t)slot[0x2Fu] << 8u)) != UINT16_C(60) ||
        ((uint16_t)slot[0x30u] | ((uint16_t)slot[0x31u] << 8u)) != UINT16_C(2) ||
        ((uint16_t)slot[0x32u] | ((uint16_t)slot[0x33u] << 8u)) != UINT16_C(4)) {
        return 1;
    }

    return 0;
}

static int test_spawn_respects_occupied_slot(void)
{
    uint8_t slot[STF_CRUSH_PART_SLOT_SIZE];
    uint8_t record[STF_CRUSH_PART_RECORD_SIZE];
    int16_t spin_table[STF_CRUSH_PART_SPIN_TABLE_COUNT];
    stf_crush_part_spawn_input input;
    stf_crush_part_spawn_result result;

    memset(slot, 0, sizeof(slot));
    memset(record, 0, sizeof(record));
    memset(spin_table, 0, sizeof(spin_table));
    memset(&input, 0, sizeof(input));

    write_le32(slot + 0x24u, UINT32_C(0x800));
    write_le16(slot + 0x22u, INT16_C(55));
    write_le16(record + 0x06u, INT16_C(99));

    input.record = record;
    input.record_size = sizeof(record);
    input.spin_table = spin_table;
    input.spin_table_count = STF_CRUSH_PART_SPIN_TABLE_COUNT;

    if (!stf_crush_part_spawn_model2(
            slot, sizeof(slot), &input, &result
        ) ||
        result.slot_was_free ||
        result.spawned ||
        ((uint16_t)slot[0x22u] | ((uint16_t)slot[0x23u] << 8u)) != UINT16_C(55)) {
        return 1;
    }

    return 0;
}

static int test_delete_weight_mode_gate(void)
{
    uint8_t defender[0x800];
    uint8_t record[STF_CRUSH_PART_RECORD_SIZE];
    bool applied = false;

    memset(defender, 0, sizeof(defender));
    memset(record, 0, sizeof(record));
    write_f32(defender + 0x7D8u, 10.0f);
    write_f32(defender + 0x7DCu, 100.0f);
    write_f32(record + 0x14u, 2.0f);

    if (!stf_crush_part_delete_weight_model2(
            defender, sizeof(defender),
            record, sizeof(record),
            UINT8_C(17), UINT8_C(0), &applied
        ) ||
        !applied ||
        read_f32(defender + 0x7D8u) != 8.0f ||
        read_f32(defender + 0x5D8u) != 108.0f) {
        return 1;
    }

    if (!stf_crush_part_should_delete_weight_model2(UINT8_C(9), UINT8_C(0)) ||
        stf_crush_part_should_delete_weight_model2(UINT8_C(9), UINT8_C(16)) ||
        !stf_crush_part_should_delete_weight_model2(UINT8_C(3), UINT8_C(5)) ||
        stf_crush_part_should_delete_weight_model2(UINT8_C(12), UINT8_C(0))) {
        return 2;
    }

    return 0;
}

static int test_spawn_bookkeeping_lane_selection(void)
{
    uint8_t defender[0x2000];
    uint8_t record[STF_CRUSH_PART_RECORD_SIZE];
    stf_crush_part_bookkeeping_result result;

    memset(defender, 0, sizeof(defender));
    memset(record, 0, sizeof(record));

    write_le16(record + 0x04u, INT16_C(-2));
    write_le16(defender + 0x1F60u, (int16_t)(UINT16_C(1) << 3u));

    if (!stf_crush_part_bookkeeping_model2(
            defender, sizeof(defender),
            record, sizeof(record),
            UINT8_C(3), UINT8_C(17), UINT8_C(0),
            &result
        ) ||
        !result.applied ||
        result.history_lane != UINT8_C(1) ||
        result.stored_record_word != UINT32_C(0xFFFFFFFE) ||
        ((uint16_t)defender[0x1F62u] |
            ((uint16_t)defender[0x1F63u] << 8u)) != (uint16_t)(UINT16_C(1) << 3u) ||
        ((uint32_t)defender[0x4Cu] |
            ((uint32_t)defender[0x4Du] << 8u) |
            ((uint32_t)defender[0x4Eu] << 16u) |
            ((uint32_t)defender[0x4Fu] << 24u)) != UINT32_C(0xFFFFFFFE)) {
        return 1;
    }

    return 0;
}


static float float_from_bits(uint32_t bits)
{
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static void fill_speed_tables(stf_crush_part_speed_tables *tables)
{
    static const float radial[6] = {
        0.06f, 0.15f, 0.09f, 0.09f, 0.26f, 0.10f
    };
    static const float vertical[6] = {
        0.04f, 0.03f, 0.08f, 0.06f, 0.03f, 0.0f
    };
    static const uint16_t angle[16] = {
        0xE940u, 0xF1C8u, 0xFA50u, 0xEC18u,
        0xF778u, 0xEEF0u, 0xF4A0u, 0xFD28u,
        0x05B0u, 0x0888u, 0x02D8u, 0x13E8u,
        0x0E38u, 0x16C0u, 0x0B60u, 0x1110u
    };
    static const uint32_t jitter[16] = {
        0x3D0B4396u, 0x3CFDF3B6u, 0xBB449BA6u, 0x3C1374BCu,
        0x3D0F5C29u, 0xBB03126Fu, 0x3CED9168u, 0x3B449BA6u,
        0x3C8B4396u, 0x3D5D2F1Bu, 0xBC83126Fu, 0x3CAC0831u,
        0x3D3020C5u, 0x3C75C28Fu, 0xBBC49BA6u, 0x3D8B4396u
    };
    size_t i;

    memset(tables, 0, sizeof(*tables));
    for (i = 0u; i < 6u; ++i) {
        tables->radial_profile_bits[i] = fbits(radial[i]);
        tables->vertical_profile_bits[i] = fbits(vertical[i]);
    }
    for (i = 0u; i < 16u; ++i) {
        tables->angle_offsets[i] = (int16_t)angle[i];
        tables->jitter_bits[i] = jitter[i];
    }
}

static int test_speed_request_generation(void)
{
    uint8_t records[2u * STF_CRUSH_PART_RECORD_SIZE];
    stf_crush_part_speed_input input;
    stf_crush_part_speed_request requests[STF_CRUSH_PART_MAX_SPEEDS];
    stf_crush_part_speed_result result;
    stf_crush_part_speed_tables tables;
    uint32_t velocity[3];
    float radial0;
    float radial1;
    float vertical0;

    memset(records, 0, sizeof(records));
    memset(&input, 0, sizeof(input));
    memset(requests, 0, sizeof(requests));
    fill_speed_tables(&tables);

    write_le16(records + STF_CRUSH_PART_RECORD_SIZE + 0x06u, INT16_C(1));

    input.count = UINT8_C(2);
    input.body_height_83d = UINT8_C(0);
    input.profile_843 = UINT8_C(5);
    input.effect_active_914 = UINT32_C(0);
    input.fighter_angle_26 = INT16_C(0);
    input.fighter_angle_82a = INT16_C(0);
    input.part_index = UINT8_C(0);
    input.records = records;
    input.records_size = sizeof(records);
    input.tables = &tables;

    if (!stf_crush_part_build_speed_requests_model2(
            &input, requests, STF_CRUSH_PART_MAX_SPEEDS, &result
        ) ||
        result.count_rejected ||
        result.generated != UINT8_C(2) ||
        requests[0].jitter_index != UINT8_C(0) ||
        requests[1].jitter_index != UINT8_C(1) ||
        requests[0].angle != UINT16_C(0x16C0)) {
        return 1;
    }

    radial0 = float_from_bits(requests[0].radial_speed_bits);
    radial1 = float_from_bits(requests[1].radial_speed_bits);
    vertical0 = float_from_bits(requests[0].vertical_speed_bits);

    if (radial0 < 0.0339f || radial0 > 0.0341f ||
        radial1 < 0.0649f || radial1 > 0.0651f ||
        vertical0 < 0.1279f || vertical0 > 0.1281f) {
        return 2;
    }

    if (!stf_crush_part_resolve_speed_model2(
            &requests[0],
            fbits(1.0f),
            fbits(2.0f),
            velocity
        ) ||
        velocity[0] != fbits(-1.0f) ||
        velocity[1] != requests[0].vertical_speed_bits ||
        velocity[2] != fbits(2.0f)) {
        return 3;
    }

    return 0;
}

static int test_speed_semantic_resolution(void)
{
    stf_crush_part_speed_request request;
    uint32_t velocity[3];

    memset(&request, 0, sizeof(request));
    request.angle = UINT16_C(0x0000);
    request.radial_speed_bits = fbits(2.0f);
    request.vertical_speed_bits = fbits(3.0f);

    if (!stf_crush_part_resolve_speed_semantic_model2(
            &request, velocity
        ) ||
        velocity[0] != UINT32_C(0x80000000) ||
        velocity[1] != fbits(3.0f) ||
        velocity[2] != fbits(2.0f)) {
        return 1;
    }

    return 0;
}

static int test_speed_request_count_limit(void)
{
    uint8_t records[STF_CRUSH_PART_RECORD_SIZE];
    stf_crush_part_speed_input input;
    stf_crush_part_speed_request request;
    stf_crush_part_speed_result result;
    stf_crush_part_speed_tables tables;

    memset(records, 0, sizeof(records));
    memset(&input, 0, sizeof(input));
    memset(&request, 0, sizeof(request));
    fill_speed_tables(&tables);

    input.count = UINT8_C(5);
    input.profile_843 = UINT8_C(0);
    input.records = records;
    input.records_size = sizeof(records);
    input.tables = &tables;

    if (!stf_crush_part_build_speed_requests_model2(
            &input, &request, 1u, &result
        ) ||
        !result.count_rejected ||
        result.generated != UINT8_C(0)) {
        return 1;
    }

    return 0;
}


static int test_composed_crush_part_set(void)
{
    uint8_t defender[0x2000];
    uint8_t slot[STF_CRUSH_PART_SLOT_SIZE];
    uint8_t records[2u * STF_CRUSH_PART_RECORD_SIZE];
    uint32_t velocities[2][3];
    int16_t spin_table[STF_CRUSH_PART_SPIN_TABLE_COUNT];
    stf_crush_part_set_input input;
    stf_crush_part_set_result result;
    const size_t position_offset = 0x1F4u + 0x0Cu;

    memset(defender, 0, sizeof(defender));
    memset(slot, 0, sizeof(slot));
    memset(records, 0, sizeof(records));
    memset(velocities, 0, sizeof(velocities));
    memset(spin_table, 0, sizeof(spin_table));
    memset(&input, 0, sizeof(input));

    defender[0x04u] = UINT8_C(1);
    write_le16(defender + 0x26u, INT16_C(100));
    write_f32(defender + position_offset + 0u, 1.0f);
    write_f32(defender + position_offset + 4u, 2.0f);
    write_f32(defender + position_offset + 8u, 3.0f);
    write_f32(defender + 0x7D8u, 10.0f);
    write_f32(defender + 0x7DCu, 100.0f);

    write_le16(records + 0x04u, INT16_C(42));
    write_le16(records + 0x06u, INT16_C(777));
    write_f32(records + 0x14u, 1.0f);
    write_f32(records + 0x18u, 0.5f);
    write_le32(records + 0x1Cu, UINT32_C(1) << 2u);

    write_f32(records + STF_CRUSH_PART_RECORD_SIZE + 0x14u, 2.0f);
    write_f32(records + STF_CRUSH_PART_RECORD_SIZE + 0x18u, 0.25f);

    velocities[0][0] = fbits(0.1f);
    velocities[0][1] = fbits(0.2f);
    velocities[0][2] = fbits(0.3f);
    velocities[1][0] = fbits(0.4f);
    velocities[1][1] = fbits(0.5f);
    velocities[1][2] = fbits(0.6f);

    input.records = records;
    input.records_size = sizeof(records);
    input.velocities = velocities;
    input.velocity_count = 2u;
    input.count = UINT8_C(2);
    input.part_index = UINT8_C(1);
    input.record_index = UINT8_C(3);
    input.effect_active_914 = UINT32_C(0);
    input.also_mode = UINT8_C(17);
    input.also_sub_mode = UINT8_C(0);
    input.spin_table = spin_table;
    input.spin_table_count = STF_CRUSH_PART_SPIN_TABLE_COUNT;

    if (!stf_crush_part_set_model2(
            defender, sizeof(defender),
            slot, sizeof(slot),
            &input, &result
        ) ||
        !result.marked_part_1f40 ||
        result.spawn_gate_rejected ||
        !result.slot_occupied_break ||
        result.iterations_entered != UINT8_C(2) ||
        result.weights_applied != UINT8_C(2) ||
        result.spawned_count != UINT8_C(1) ||
        !result.spawn.spawned ||
        result.spawn.object_id != UINT16_C(777) ||
        !result.bookkeeping.applied ||
        result.bookkeeping.history_lane != UINT8_C(0) ||
        read_f32(defender + 0x7D8u) != 7.0f ||
        read_f32(defender + 0x5D8u) != 107.0f ||
        (((uint32_t)defender[0x1F40u] |
          ((uint32_t)defender[0x1F41u] << 8u) |
          ((uint32_t)defender[0x1F42u] << 16u) |
          ((uint32_t)defender[0x1F43u] << 24u)) &
         (UINT32_C(1) << 1u)) == 0u ||
        read_f32(slot + 0x00u) != 1.0f ||
        read_f32(slot + 0x04u) != 2.0f ||
        read_f32(slot + 0x08u) != 3.0f ||
        read_f32(slot + 0x0Cu) != 0.1f ||
        read_f32(slot + 0x10u) != 0.2f ||
        read_f32(slot + 0x14u) != 0.3f ||
        read_f32(slot + 0x18u) != 0.5f) {
        return 1;
    }

    return 0;
}

static int test_composed_crush_part_gate_reject(void)
{
    uint8_t defender[0x2000];
    uint8_t slot[STF_CRUSH_PART_SLOT_SIZE];
    uint8_t record[STF_CRUSH_PART_RECORD_SIZE];
    uint32_t velocity[1][3];
    int16_t spin_table[STF_CRUSH_PART_SPIN_TABLE_COUNT];
    stf_crush_part_set_input input;
    stf_crush_part_set_result result;

    memset(defender, 0, sizeof(defender));
    memset(slot, 0, sizeof(slot));
    memset(record, 0, sizeof(record));
    memset(velocity, 0, sizeof(velocity));
    memset(spin_table, 0, sizeof(spin_table));
    memset(&input, 0, sizeof(input));

    write_le16(record + 0x06u, INT16_C(99));
    write_f32(record + 0x14u, 1.0f);
    write_f32(record + 0x18u, 0.5f);
    write_le32(record + 0x1Cu, UINT32_C(1) << 3u);

    input.records = record;
    input.records_size = sizeof(record);
    input.velocities = velocity;
    input.velocity_count = 1u;
    input.count = UINT8_C(1);
    input.part_index = UINT8_C(0);
    input.also_mode = UINT8_C(17);
    input.spin_table = spin_table;
    input.spin_table_count = STF_CRUSH_PART_SPIN_TABLE_COUNT;

    if (!stf_crush_part_set_model2(
            defender, sizeof(defender),
            slot, sizeof(slot),
            &input, &result
        ) ||
        !result.spawn_gate_rejected ||
        result.spawned_count != UINT8_C(0) ||
        result.iterations_entered != UINT8_C(1) ||
        result.weights_applied != UINT8_C(1) ||
        ((uint16_t)slot[0x22u] |
         ((uint16_t)slot[0x23u] << 8u)) != UINT16_C(0)) {
        return 1;
    }

    return 0;
}


static int test_floor_sound_selection(void)
{
    stf_crush_part_floor_sound_result result;

    if (!stf_crush_part_floor_sound_select_model2(
            UINT32_C(0), &result
        ) ||
        result.request_sound) {
        return 1;
    }

    if (!stf_crush_part_floor_sound_select_model2(
            UINT32_C(0x50000000), &result
        ) ||
        !result.request_sound ||
        result.table_index != UINT8_C(2)) {
        return 2;
    }

    if (!stf_crush_part_floor_sound_select_model2(
            UINT32_C(0xF0000000), &result
        ) ||
        !result.request_sound ||
        result.table_index != UINT8_C(3)) {
        return 3;
    }

    return 0;
}

static int test_crush_part_lifecycle_flow(void)
{
    uint8_t defender[0x2000];
    uint8_t slot[STF_CRUSH_PART_SLOT_SIZE];
    uint8_t record[STF_CRUSH_PART_RECORD_SIZE];
    int16_t spin_table[STF_CRUSH_PART_SPIN_TABLE_COUNT];
    stf_crush_part_speed_tables speed_tables;
    stf_crush_part_put_input put_input;
    stf_crush_part_put_result put_result;
    stf_crush_part_physics_env physics_env;
    stf_crush_part_physics_result physics_result;
    stf_crush_part_angle_result angle_result;
    stf_crush_part_visibility_world_input visibility_input;
    stf_crush_part_visibility_result visibility_result;
    stf_crush_part_draw draw;
    stf_crush_part_floor_sound_result sound_result;
    unsigned i;

    memset(defender, 0, sizeof(defender));
    memset(slot, 0, sizeof(slot));
    memset(record, 0, sizeof(record));
    memset(spin_table, 0, sizeof(spin_table));
    fill_speed_tables(&speed_tables);
    memset(&put_input, 0, sizeof(put_input));
    memset(&physics_env, 0, sizeof(physics_env));
    memset(&visibility_input, 0, sizeof(visibility_input));

    for (i = 0u; i < STF_CRUSH_PART_SPIN_TABLE_COUNT; ++i) {
        spin_table[i] = INT16_C(32);
    }

    write_le16(record + 0x06u, INT16_C(321));
    write_f32(record + 0x14u, 1.0f);
    write_f32(record + 0x18u, 1.0f);
    write_le32(record + 0x1Cu, UINT32_C(0xA0000000));
    write_f32(record + 0x20u, -100.0f);

    write_f32(defender + 0x1F4u, 0.0f);
    write_f32(defender + 0x1F8u, 5.0f);
    write_f32(defender + 0x1FCu, 0.0f);

    put_input.speed.count = UINT8_C(1);
    put_input.speed.body_height_83d = UINT8_C(60);
    put_input.speed.profile_843 = UINT8_C(0);
    put_input.speed.part_index = UINT8_C(0);
    put_input.speed.records = record;
    put_input.speed.records_size = sizeof(record);
    put_input.speed.tables = &speed_tables;
    put_input.spin_table = spin_table;
    put_input.spin_table_count = STF_CRUSH_PART_SPIN_TABLE_COUNT;

    if (!stf_crush_part_put_model2(
            defender, sizeof(defender),
            slot, sizeof(slot),
            &put_input, &put_result
        ) ||
        put_result.speed.generated != UINT8_C(1) ||
        put_result.set.spawned_count != UINT8_C(1) ||
        !put_result.set.spawn.spawned ||
        put_result.set.spawn.object_id != UINT16_C(321)) {
        return 1;
    }

    physics_env.gravity_bits = fbits(0.0f);
    physics_env.stage_x_bits = fbits(100.0f);
    physics_env.cage_height_bits = fbits(100.0f);

    if (!stf_crush_part_update_position_model2(
            slot, sizeof(slot), &physics_env, &physics_result
        ) ||
        physics_result.deactivated ||
        !stf_crush_part_update_angles_model2(
            slot, sizeof(slot), &angle_result
        )) {
        return 3;
    }

    visibility_input.camera_matrix_bits[0] = fbits(1.0f);
    visibility_input.camera_matrix_bits[4] = fbits(1.0f);
    visibility_input.camera_matrix_bits[8] = fbits(1.0f);
    visibility_input.camera_matrix_bits[11] = fbits(10.0f);
    visibility_input.world_position_bits[0] = read_le32(slot + 0x00u);
    visibility_input.world_position_bits[1] = read_le32(slot + 0x04u);
    visibility_input.world_position_bits[2] = read_le32(slot + 0x08u);
    visibility_input.radius_bits = fbits(1.0f);
    visibility_input.focus_distance_bits = fbits(100.0f);

    if (!stf_crush_part_visibility_from_matrix_model2(
            &visibility_input, &visibility_result
        ) ||
        visibility_result.mask != UINT32_C(0x0F) ||
        !stf_crush_part_build_draw_model2(
            slot, sizeof(slot), &draw
        ) ||
        !draw.active ||
        draw.object_id != UINT16_C(321) ||
        !stf_crush_part_floor_sound_select_model2(
            read_le32(slot + 0x24u), &sound_result
        ) ||
        !sound_result.request_sound ||
        sound_result.table_index != UINT8_C(3)) {
        return 4;
    }

    return 0;
}
