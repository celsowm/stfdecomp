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

    memset(&spawn_inputs, 0, sizeof(spawn_inputs));
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


    /*
     * Recover the per-frame ring_tobitiri state machine.  Use a synthetic A
     * curve so the test verifies semantics without embedding ROM payloads.
     */
    memset(slots, 0, sizeof(slots));
    stf_ring_pool_init(&pool);
    if (!stf_ring_pool_allocate(&pool, &slot) || slot != UINT8_C(23)) {
        return 20;
    }

    slots[slot].active = true;
    slots[slot].age = 0u;
    slots[slot].x_bits = float_to_bits(7.48f);
    slots[slot].y_bits = float_to_bits(1.0f);
    slots[slot].z_bits = float_to_bits(0.0f);
    slots[slot].vx_bits = float_to_bits(0.04f);
    slots[slot].vz_bits = float_to_bits(0.02f);
    slots[slot].trajectory = STF_RING_TRAJECTORY_A;
    slots[slot].blink_from_frame = 2u;
    slots[slot].expire_at_frame = 120u;

    curve_a[1] = float_to_bits(1.5f);
    curve_a[2] = 0u;

    {
        stf_ring_tick_inputs tick_inputs;
        stf_ring_tick_result tick_result;

        tick_inputs.paused = false;
        tick_inputs.stage_num = 0u;
        tick_inputs.curve_words = curve_a;
        tick_inputs.curve_word_count =
            sizeof(curve_a) / sizeof(curve_a[0]);

        if (!stf_ring_slot_tick(
                &tick_inputs, &pool, slot, slots, &tick_result
            ) ||
            tick_result.released || tick_result.landed ||
            !tick_result.visible ||
            slots[slot].age != 1u ||
            !nearf_value(bits_to_float(slots[slot].x_bits), 7.5f) ||
            !nearf_value(bits_to_float(slots[slot].z_bits), 0.02f) ||
            !nearf_value(bits_to_float(slots[slot].vx_bits), -0.04f) ||
            !nearf_value(bits_to_float(tick_result.render_y_bits), 1.5f) ||
            tick_result.primary_asset_id != UINT32_C(0x2C4) ||
            tick_result.draw_secondary ||
            tick_result.drop_variant != 0u) {
            return 21;
        }

        if (!stf_ring_slot_tick(
                &tick_inputs, &pool, slot, slots, &tick_result
            ) ||
            tick_result.released || tick_result.landed ||
            tick_result.visible ||
            slots[slot].age != 2u ||
            !nearf_value(bits_to_float(slots[slot].x_bits), 7.46f) ||
            !nearf_value(bits_to_float(slots[slot].vx_bits), -0.028f) ||
            !nearf_value(bits_to_float(slots[slot].vz_bits), 0.014f)) {
            return 22;
        }

        tick_inputs.paused = true;
        {
            const uint32_t x_before = slots[slot].x_bits;
            const uint32_t z_before = slots[slot].z_bits;
            if (!stf_ring_slot_tick(
                    &tick_inputs, &pool, slot, slots, &tick_result
                ) ||
                slots[slot].age != 2u ||
                slots[slot].x_bits != x_before ||
                slots[slot].z_bits != z_before ||
                tick_result.visible) {
                return 23;
            }
        }

        tick_inputs.paused = false;
        slots[slot].age = 98u;
        slots[slot].blink_from_frame = 200u;
        slots[slot].x_bits = float_to_bits(1.0f);
        slots[slot].y_bits = float_to_bits(0.5f);
        slots[slot].z_bits = float_to_bits(2.0f);
        slots[slot].vx_bits = float_to_bits(0.1f);
        slots[slot].vz_bits = float_to_bits(-0.1f);

        if (!stf_ring_slot_tick(
                &tick_inputs, &pool, slot, slots, &tick_result
            ) ||
            !tick_result.landed || !tick_result.visible ||
            slots[slot].age != 99u ||
            slots[slot].y_bits != UINT32_C(0xBF800000) ||
            tick_result.render_y_bits != 0u ||
            tick_result.primary_asset_id != UINT32_C(0x2C6) ||
            !nearf_value(bits_to_float(slots[slot].x_bits), 1.1f) ||
            !nearf_value(bits_to_float(slots[slot].z_bits), 1.9f)) {
            return 24;
        }

        {
            const uint32_t x_landed = slots[slot].x_bits;
            const uint32_t z_landed = slots[slot].z_bits;
            if (!stf_ring_slot_tick(
                    &tick_inputs, &pool, slot, slots, &tick_result
                ) ||
                !tick_result.landed ||
                slots[slot].age != 100u ||
                slots[slot].x_bits != x_landed ||
                slots[slot].z_bits != z_landed ||
                tick_result.render_y_bits != 0u) {
                return 25;
            }
        }


        /*
         * Stage 2 draws a second ring asset.  Concrete drop variants use the
         * five-entry drop table and expose the original 16-step spin phase.
         */
        tick_inputs.paused = true;
        tick_inputs.stage_num = STF_RING_SPECIAL_STAGE;
        slots[slot].age = 100u;
        slots[slot].blink_from_frame = 200u;
        slots[slot].y_bits = UINT32_C(0xBF800000);
        slots[slot].drop_variant = 0u;
        if (!stf_ring_slot_tick(
                &tick_inputs, &pool, slot, slots, &tick_result
            ) ||
            !tick_result.visible ||
            tick_result.primary_asset_id != UINT32_C(0x2C7) ||
            tick_result.secondary_asset_id != UINT32_C(0x336) ||
            !tick_result.draw_secondary) {
            return 26;
        }

        slots[slot].drop_variant = 5u;
        if (!stf_ring_slot_tick(
                &tick_inputs, &pool, slot, slots, &tick_result
            ) ||
            tick_result.primary_asset_id != UINT32_C(0x6AC) ||
            tick_result.secondary_asset_id != UINT32_C(0x6AC) ||
            tick_result.spin_phase != UINT16_C(0x4000) ||
            tick_result.drop_variant != 5u ||
            !tick_result.draw_secondary) {
            return 27;
        }

        tick_inputs.paused = false;
        tick_inputs.stage_num = 0u;
        slots[slot].drop_variant = 0u;
        slots[slot].age = 119u;
        slots[slot].expire_at_frame = 120u;
        if (!stf_ring_slot_tick(
                &tick_inputs, &pool, slot, slots, &tick_result
            ) ||
            !tick_result.released ||
            slots[slot].active ||
            (pool.occupied_mask & (UINT32_C(1) << slot)) != 0u ||
            pool.head != STF_RING_POOL_EMPTY ||
            pool.tail != STF_RING_POOL_EMPTY) {
            return 28;
        }
    }

    /*
     * Egg variants store the concrete RNG result (1..4) in each slot rather
     * than a generic "random" mode.
     */
    plan_inputs.damage = 20u;
    plan_inputs.defender_character = 11u;
    plan_inputs.defender_motion_1a8 = 0u;
    plan_inputs.stage_num = 0u;
    if (!stf_ring_scatter_plan_compute(&plan_inputs, &plan)) {
        return 29;
    }
    memset(slots, 0, sizeof(slots));
    stf_ring_pool_init(&pool);
    spawn_inputs.drop_random_values[0] = UINT32_C(2);
    spawn_inputs.drop_random_values[1] = UINT32_C(7);
    if (!stf_ring_scatter_spawn(
            &plan, &spawn_inputs, &pool, slots, &spawn_result
        ) ||
        slots[23].drop_variant != 3u ||
        slots[22].drop_variant != 4u) {
        return 30;
    }

    return 0;
}
