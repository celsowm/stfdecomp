#include "ring_scatter_damage_flow.h"

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

bool stf_ring_scatter_apply_damage_event_model2(
    const stf_damage_calculation_result *damage,
    const uint8_t *receiver,
    size_t receiver_size,
    const uint8_t *dealer,
    size_t dealer_size,
    const stf_ring_damage_flow_inputs *inputs,
    stf_ring_pool *pool,
    stf_ring_slot slots[STF_RING_POOL_SLOT_COUNT],
    stf_ring_damage_flow_result *result
)
{
    stf_ring_damage_flow_result local;
    stf_ring_scatter_inputs plan_inputs;
    stf_ring_scatter_plan plan;
    stf_ring_scatter_spawn_inputs spawn_inputs;
    stf_ring_scatter_spawn_result spawn_result;

    if (damage == NULL || receiver == NULL || dealer == NULL ||
        inputs == NULL || pool == NULL || slots == NULL ||
        receiver_size < STF_RING_DAMAGE_FLOW_FIGHTER_MIN_SIZE ||
        dealer_size < STF_RING_DAMAGE_FLOW_FIGHTER_MIN_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    if (!damage->request_ring_scatter || damage->scaled_damage == 0u) {
        if (result != NULL) {
            *result = local;
        }
        return true;
    }

    local.requested = true;

    memset(&plan_inputs, 0, sizeof(plan_inputs));
    plan_inputs.damage = damage->scaled_damage;
    plan_inputs.defender_character = receiver[0x1B1u];
    plan_inputs.defender_motion_1a8 = read_le16(receiver + 0x1A8u);
    plan_inputs.stage_num = inputs->stage_num;

    if (!stf_ring_scatter_plan_compute(&plan_inputs, &plan)) {
        return false;
    }

    local.suppressed = plan.suppressed;
    local.request_ring_sound = plan.request_ring_sound;
    if (plan.suppressed) {
        if (result != NULL) {
            *result = local;
        }
        return true;
    }

    memset(&spawn_inputs, 0, sizeof(spawn_inputs));
    spawn_inputs.defender_angle_x_bits = read_le32(receiver + 0x1F4u);
    spawn_inputs.defender_angle_z_bits = read_le32(receiver + 0x1FCu);
    spawn_inputs.attacker_angle_x_bits = read_le32(dealer + 0x1F4u);
    spawn_inputs.attacker_angle_z_bits = read_le32(dealer + 0x1FCu);
    spawn_inputs.spawn_x_bits = read_le32(receiver + 0x20Cu);
    spawn_inputs.spawn_y_bits = read_le32(receiver + 0x210u);
    spawn_inputs.spawn_z_bits = read_le32(receiver + 0x214u);
    memcpy(
        spawn_inputs.drop_random_values,
        inputs->drop_random_values,
        sizeof(spawn_inputs.drop_random_values)
    );

    if (!stf_ring_scatter_spawn(
            &plan, &spawn_inputs, pool, slots, &spawn_result
        )) {
        return false;
    }

    local.spawned_count = spawn_result.spawned_count;
    local.recycled_count = spawn_result.recycled_count;
    memcpy(
        local.slot_indices,
        spawn_result.slot_indices,
        sizeof(local.slot_indices)
    );

    if (result != NULL) {
        *result = local;
    }
    return true;
}
