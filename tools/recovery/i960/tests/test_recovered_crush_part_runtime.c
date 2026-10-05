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
static int test_visibility_mask_centered(void);
static int test_visibility_mask_partial_edges(void);
static int test_visibility_behind_camera(void);
static int test_spawn_from_rom_record(void);
static int test_spawn_respects_occupied_slot(void);
static int test_delete_weight_mode_gate(void);
static int test_spawn_bookkeeping_lane_selection(void);

int main(void)
{
    if (test_free_spin_branch() != 0) return 1;
    if (test_target_approach_branch() != 0) return 1;
    if (test_draw_extraction() != 0) return 1;
    if (test_position_air_integration() != 0) return 1;
    if (test_floor_bounce_and_stop() != 0) return 1;
    if (test_wall_reflection() != 0) return 1;
    if (test_dormant_visibility_deactivation() != 0) return 1;
    if (test_visibility_mask_centered() != 0) return 1;
    if (test_visibility_mask_partial_edges() != 0) return 1;
    if (test_visibility_behind_camera() != 0) return 1;
    if (test_spawn_from_rom_record() != 0) return 1;
    if (test_spawn_respects_occupied_slot() != 0) return 1;
    if (test_delete_weight_mode_gate() != 0) return 1;
    if (test_spawn_bookkeeping_lane_selection() != 0) return 1;
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
    stf_crush_part_visibility_input visibility_input;
    stf_crush_part_visibility_result visibility;

    memset(slot, 0, sizeof(slot));
    memset(&env, 0, sizeof(env));
    memset(&visibility_input, 0, sizeof(visibility_input));

    visibility_input.camera_x_bits = fbits(30.0f);
    visibility_input.camera_y_bits = fbits(30.0f);
    visibility_input.camera_z_bits = fbits(10.0f);
    visibility_input.radius_bits = fbits(1.0f);
    visibility_input.focus_distance_bits = fbits(100.0f);

    if (!stf_crush_part_visibility_mask_model2(
            &visibility_input, &visibility
        ) ||
        visibility.mask != UINT32_C(0)) {
        return 1;
    }

    write_le16(slot + 0x22u, INT16_C(9));
    write_le32(slot + 0x24u,
               (UINT32_C(1) << 7u) | (UINT32_C(1) << 3u));
    env.effect_active_914 = UINT32_C(0);
    env.visibility_mask = visibility.mask;

    if (!stf_crush_part_update_position_model2(
            slot, sizeof(slot), &env, &result
        ) ||
        !result.deactivated ||
        slot[0x22u] != UINT8_C(0) ||
        slot[0x23u] != UINT8_C(0) ||
        result.flags != UINT32_C(0)) {
        return 2;
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
    input.radius_bits = UINT32_C(0x3F000000);
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
