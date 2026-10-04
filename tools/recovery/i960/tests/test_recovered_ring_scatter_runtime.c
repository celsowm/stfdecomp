#include <math.h>
#include <stdint.h>
#include <string.h>

#include "ring_scatter_runtime.h"

static uint32_t float_to_bits(float value)
{
    uint32_t bits = 0u;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static float bits_to_float(uint32_t bits)
{
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static int nearf_value(float actual, float expected)
{
    return fabsf(actual - expected) <= 0.0001f;
}

int main(void)
{
    stf_ring_profile_record record;
    stf_ring_scatter_inputs plan_inputs;
    stf_ring_scatter_plan plan;
    stf_ring_scatter_spawn_inputs spawn_inputs;
    stf_ring_scatter_spawn_result spawn_result;
    stf_ring_pool pool;
    stf_ring_slot slots[STF_RING_POOL_SLOT_COUNT];
    uint8_t slot = 0u;
    unsigned index = 0u;
    stf_ring_trajectory_info trajectory_info;
    uint32_t trajectory_sample = 0u;
    uint32_t curve_a[100] = {0u};
    uint32_t curve_b[120] = {0u};
    uint32_t curve_c[140] = {0u};

    curve_a[0] = UINT32_C(0x4029999A);
    curve_a[98] = 0u;
    curve_a[99] = UINT32_C(0xBF800000);
    curve_b[0] = UINT32_C(0x402E147B);
    curve_b[119] = UINT32_C(0xBF800000);
    curve_c[0] = UINT32_C(0x40247AE1);
    curve_c[138] = 0u;
    curve_c[139] = UINT32_C(0xBF800000);

    if (!stf_ring_trajectory_info_get(
            STF_RING_TRAJECTORY_A, &trajectory_info
        ) ||
        trajectory_info.sfight_rom_address != UINT32_C(0x000AE488) ||
        trajectory_info.schamp_rom_address != UINT32_C(0x000AE5C0) ||
        trajectory_info.sample_count != UINT16_C(99)) {
        return 1;
    }

    if (!stf_ring_trajectory_info_get(
            STF_RING_TRAJECTORY_B, &trajectory_info
        ) ||
        trajectory_info.sample_count != UINT16_C(119) ||
        !stf_ring_trajectory_info_get(
            STF_RING_TRAJECTORY_C, &trajectory_info
        ) ||
        trajectory_info.sample_count != UINT16_C(139)) {
        return 2;
    }

    if (!stf_ring_trajectory_sample_bits(
            STF_RING_TRAJECTORY_A,
            curve_a,
            sizeof(curve_a) / sizeof(curve_a[0]),
            0u,
            &trajectory_sample
        ) ||
        trajectory_sample != UINT32_C(0x4029999A) ||
        stf_ring_trajectory_sample_bits(
            STF_RING_TRAJECTORY_A,
            curve_a,
            sizeof(curve_a) / sizeof(curve_a[0]),
            99u,
            &trajectory_sample
        )) {
        return 3;
    }

    curve_b[119] = 0u;
    if (stf_ring_trajectory_sample_bits(
            STF_RING_TRAJECTORY_B,
            curve_b,
            sizeof(curve_b) / sizeof(curve_b[0]),
            0u,
            &trajectory_sample
        )) {
        return 4;
    }
    curve_b[119] = UINT32_C(0xBF800000);

    if (!stf_ring_trajectory_sample_bits(
            STF_RING_TRAJECTORY_C,
            curve_c,
            sizeof(curve_c) / sizeof(curve_c[0]),
            138u,
            &trajectory_sample
        ) ||
        trajectory_sample != 0u) {
        return 5;
    }

    if (!stf_ring_profile_select(
            STF_RING_SCATTER_DAMAGE_16, 0u, &record
        ) ||
        record.max_ring_index != 8u ||
        record.velocity_scale_bits != UINT32_C(0x3F800000) ||
        record.trajectory != STF_RING_TRAJECTORY_C) {
        return 1;
    }

    if (!stf_ring_profile_select(
            STF_RING_SCATTER_DAMAGE_16, 9u, &record
        ) ||
        record.max_ring_index != 16u ||
        record.velocity_scale_bits != UINT32_C(0x3FB33333) ||
        record.trajectory != STF_RING_TRAJECTORY_B) {
        return 2;
    }

    if (!stf_ring_profile_select(
            STF_RING_SCATTER_DAMAGE_16, 17u, &record
        ) ||
        record.max_ring_index != 24u ||
        record.velocity_scale_bits != UINT32_C(0x3FE66666) ||
        record.trajectory != STF_RING_TRAJECTORY_A) {
        return 3;
    }

    plan_inputs.damage = 20u;
    plan_inputs.defender_character = 0u;
    plan_inputs.defender_motion_1a8 = 0u;
    plan_inputs.stage_num = 0u;

    if (!stf_ring_scatter_plan_compute(&plan_inputs, &plan)) {
        return 4;
    }

    memset(slots, 0, sizeof(slots));
    stf_ring_pool_init(&pool);

    spawn_inputs.defender_angle_x_bits = float_to_bits(1.0f);
    spawn_inputs.defender_angle_z_bits = float_to_bits(0.0f);
    spawn_inputs.attacker_angle_x_bits = float_to_bits(0.0f);
    spawn_inputs.attacker_angle_z_bits = float_to_bits(0.0f);
    spawn_inputs.spawn_x_bits = float_to_bits(10.0f);
    spawn_inputs.spawn_y_bits = float_to_bits(2.0f);
    spawn_inputs.spawn_z_bits = float_to_bits(20.0f);

    if (!stf_ring_scatter_spawn(
            &plan, &spawn_inputs, &pool, slots, &spawn_result
        ) ||
        spawn_result.spawned_count != 2u ||
        spawn_result.recycled_count != 0u ||
        spawn_result.slot_indices[0] != 23u ||
        spawn_result.slot_indices[1] != 22u) {
        return 5;
    }

    if (!slots[23].active ||
        !nearf_value(bits_to_float(slots[23].x_bits), 10.5f) ||
        !nearf_value(bits_to_float(slots[23].y_bits), 2.0f) ||
        !nearf_value(bits_to_float(slots[23].z_bits), 20.0f) ||
        !nearf_value(bits_to_float(slots[23].vx_bits), 0.04f) ||
        !nearf_value(bits_to_float(slots[23].vz_bits), 0.0f) ||
        slots[23].trajectory != STF_RING_TRAJECTORY_A ||
        slots[23].blink_from_frame != 60u ||
        slots[23].expire_at_frame != 90u) {
        return 6;
    }

    if (!nearf_value(bits_to_float(slots[22].x_bits), 9.5f) ||
        !nearf_value(bits_to_float(slots[22].z_bits), 20.0f) ||
        !nearf_value(bits_to_float(slots[22].vx_bits), -0.04f)) {
        return 7;
    }

    memset(slots, 0, sizeof(slots));
    stf_ring_pool_init(&pool);

    spawn_inputs.defender_angle_x_bits = float_to_bits(0.0f);
    spawn_inputs.defender_angle_z_bits = float_to_bits(1.0f);

    if (!stf_ring_scatter_spawn(
            &plan, &spawn_inputs, &pool, slots, &spawn_result
        ) ||
        !nearf_value(bits_to_float(slots[23].x_bits), 10.0f) ||
        !nearf_value(bits_to_float(slots[23].z_bits), 20.5f) ||
        !nearf_value(bits_to_float(slots[23].vx_bits), 0.0f) ||
        !nearf_value(bits_to_float(slots[23].vz_bits), 0.04f)) {
        return 8;
    }

    memset(slots, 0, sizeof(slots));
    stf_ring_pool_init(&pool);
    for (index = 0u; index < STF_RING_POOL_SLOT_COUNT; ++index) {
        if (!stf_ring_pool_allocate(&pool, &slot)) {
            return 9;
        }
    }

    spawn_inputs.defender_angle_x_bits = float_to_bits(1.0f);
    spawn_inputs.defender_angle_z_bits = float_to_bits(0.0f);

    if (!stf_ring_scatter_spawn(
            &plan, &spawn_inputs, &pool, slots, &spawn_result
        ) ||
        spawn_result.spawned_count != 2u ||
        spawn_result.recycled_count != 2u ||
        spawn_result.slot_indices[0] != 23u ||
        spawn_result.slot_indices[1] != 22u ||
        pool.head != 21u ||
        pool.tail != 22u) {
        return 10;
    }

    return 0;
}
