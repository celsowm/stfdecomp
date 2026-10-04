#include "attack_hit_finish.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

static uint16_t read_le16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8u));
}

static int32_t round_nearest_even(float value)
{
    const double x = (double)value;
    const double lower = floor(x);
    const double fraction = x - lower;
    double rounded = 0.0;

    if (fraction < 0.5) {
        rounded = lower;
    } else if (fraction > 0.5) {
        rounded = lower + 1.0;
    } else {
        rounded = fmod(fabs(lower), 2.0) == 0.0 ? lower : lower + 1.0;
    }
    return (int32_t)rounded;
}

bool stf_attack_hit_finish_compute(
    const stf_attack_finish_inputs *inputs,
    stf_attack_finish_result *result
)
{
    stf_attack_finish_result local;
    int32_t difference = 0;
    int32_t ceiling = 0;
    uint32_t scaled_damage = 0u;

    if (inputs == NULL || result == NULL) {
        return false;
    }

    memset(&local, 0, sizeof(local));

    difference =
        (int32_t)inputs->defender_energy_1ac -
        (int32_t)inputs->attacker_energy_1ac;

    if (!inputs->disable_energy_scaling) {
        const int32_t missing =
            (int32_t)inputs->energy_max -
            (int32_t)inputs->defender_energy_1ac;

        ceiling = 50 - (missing >> 1);
        if (ceiling < 0) {
            ceiling = 0;
        }

        if (difference > ceiling) {
            difference = ceiling;
        }
        if (difference < -30) {
            difference = -30;
        }

        scaled_damage = (uint32_t)round_nearest_even(
            (float)(int32_t)inputs->damage *
            (1.0f + (float)difference * 0.01f)
        );
    } else {
        scaled_damage = inputs->damage;
    }

    local.scaled_damage = scaled_damage;
    local.energy_difference_percent = difference;
    local.defender_energy_after =
        (int32_t)inputs->defender_energy_1ac - (int32_t)scaled_damage;
    local.finish_blow = local.defender_energy_after <= 0;
    local.bonus_skill = local.finish_blow ? UINT32_C(500) : 0u;

    *result = local;
    return true;
}

bool stf_attack_hit_finish_apply_model2(
    uint8_t *attacker,
    size_t attacker_size,
    const uint8_t *defender,
    size_t defender_size,
    int16_t energy_max,
    bool disable_energy_scaling,
    uint32_t damage,
    stf_attack_finish_result *result
)
{
    stf_attack_finish_inputs inputs;
    stf_attack_finish_result local;

    if (attacker == NULL || defender == NULL ||
        attacker_size < STF_ATTACK_FINISH_ATTACKER_MIN_SIZE ||
        defender_size < STF_ATTACK_FINISH_DEFENDER_MIN_SIZE) {
        return false;
    }

    inputs.attacker_energy_1ac = (int16_t)read_le16(attacker + 0x1ACu);
    inputs.defender_energy_1ac = (int16_t)read_le16(defender + 0x1ACu);
    inputs.energy_max = energy_max;
    inputs.disable_energy_scaling = disable_energy_scaling;
    inputs.damage = damage;

    if (!stf_attack_hit_finish_compute(&inputs, &local)) {
        return false;
    }

    if (local.finish_blow) {
        attacker[0x7D2u] =
            (uint8_t)(attacker[0x7D2u] | UINT8_C(1));
    }

    if (result != NULL) {
        *result = local;
    }
    return true;
}
