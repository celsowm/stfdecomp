#ifndef STF_RECOVERED_DAMAGE_CALCULATION_H
#define STF_RECOVERED_DAMAGE_CALCULATION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_DAMAGE_RECEIVER_MIN_SIZE = 0x0C56u,
    STF_DAMAGE_DEALER_MIN_SIZE = 0x1F72u
};

typedef struct stf_damage_calculation_inputs {
    int16_t receiver_energy;
    int16_t dealer_energy;
    int16_t energy_max;
    bool disable_energy_scaling;
    int16_t game_timer;
    uint8_t receiver_character;
    uint32_t damage;
} stf_damage_calculation_inputs;

typedef struct stf_damage_calculation_result {
    uint32_t scaled_damage;
    bool energy_applied;
    int16_t receiver_energy_after;
    bool receiver_knocked_out;
    bool request_ring_scatter;
} stf_damage_calculation_result;

/*
 * Portable recovery of damage_calculation + its ketchup scaling kernel.
 *
 * The same energy-difference scaler is used by sub_19730, already recovered by
 * attack_hit_finish. This helper adds the persistent-state semantics specific
 * to damage_calculation:
 * - dealer +0x1F70 receives the scaled damage unconditionally;
 * - active rounds apply it to receiver +0x1AC unless character 16 is selected;
 * - receiver +0xC54 records the applied damage;
 * - lethal damage clamps energy to zero and sets fighter flag bit 5;
 * - non-zero applied damage requests ring_tobitiri_set as an external event.
 */
bool stf_damage_calculation_compute(
    const stf_damage_calculation_inputs *inputs,
    stf_damage_calculation_result *result
);

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
);

#endif
