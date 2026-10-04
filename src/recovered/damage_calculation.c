#include "damage_calculation.h"

#include "attack_hit_finish.h"

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

static void write_le16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
}

static void write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

bool stf_damage_calculation_compute(
    const stf_damage_calculation_inputs *inputs,
    stf_damage_calculation_result *result
)
{
    stf_attack_finish_inputs scale_inputs;
    stf_attack_finish_result scale_result;
    stf_damage_calculation_result local;
    int32_t remaining = 0;

    if (inputs == NULL || result == NULL) {
        return false;
    }

    memset(&local, 0, sizeof(local));

    scale_inputs.attacker_energy_1ac = inputs->dealer_energy;
    scale_inputs.defender_energy_1ac = inputs->receiver_energy;
    scale_inputs.energy_max = inputs->energy_max;
    scale_inputs.disable_energy_scaling = inputs->disable_energy_scaling;
    scale_inputs.damage = inputs->damage;

    if (!stf_attack_hit_finish_compute(&scale_inputs, &scale_result)) {
        return false;
    }

    local.scaled_damage = scale_result.scaled_damage;
    local.receiver_energy_after = inputs->receiver_energy;
    local.energy_applied =
        inputs->game_timer > 0 &&
        inputs->receiver_character != UINT8_C(16);

    if (local.energy_applied) {
        remaining =
            (int32_t)inputs->receiver_energy -
            (int32_t)local.scaled_damage;

        if (remaining <= 0) {
            local.receiver_energy_after = 0;
            local.receiver_knocked_out = true;
        } else {
            local.receiver_energy_after = (int16_t)remaining;
        }

        local.request_ring_scatter = local.scaled_damage != 0u;
    }

    *result = local;
    return true;
}

bool stf_damage_calculation_apply_model2(
    uint8_t *receiver,
    size_t receiver_size,
    uint8_t *dealer,
    size_t dealer_size,
    int16_t energy_max,
    bool disable_energy_scaling,
    int16_t game_timer,
    uint32_t damage,
    stf_damage_calculation_result *result
)
{
    stf_damage_calculation_inputs inputs;
    stf_damage_calculation_result local;

    if (receiver == NULL || dealer == NULL ||
        receiver_size < STF_DAMAGE_RECEIVER_MIN_SIZE ||
        dealer_size < STF_DAMAGE_DEALER_MIN_SIZE) {
        return false;
    }

    inputs.receiver_energy = (int16_t)read_le16(receiver + 0x1ACu);
    inputs.dealer_energy = (int16_t)read_le16(dealer + 0x1ACu);
    inputs.energy_max = energy_max;
    inputs.disable_energy_scaling = disable_energy_scaling;
    inputs.game_timer = game_timer;
    inputs.receiver_character = receiver[0x1B1u];
    inputs.damage = damage;

    if (!stf_damage_calculation_compute(&inputs, &local)) {
        return false;
    }

    write_le16(dealer + 0x1F70u, (uint16_t)local.scaled_damage);

    if (local.energy_applied) {
        write_le16(receiver + 0x0C54u, (uint16_t)local.scaled_damage);
        write_le16(receiver + 0x1ACu, (uint16_t)local.receiver_energy_after);

        if (local.receiver_knocked_out) {
            uint32_t flags = read_le32(receiver);
            flags |= UINT32_C(1) << 5u;
            write_le32(receiver, flags);
        }
    }

    if (result != NULL) {
        *result = local;
    }
    return true;
}
