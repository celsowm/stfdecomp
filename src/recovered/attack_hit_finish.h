#ifndef STF_RECOVERED_ATTACK_HIT_FINISH_H
#define STF_RECOVERED_ATTACK_HIT_FINISH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_ATTACK_FINISH_ATTACKER_MIN_SIZE = 0x7D3u,
    STF_ATTACK_FINISH_DEFENDER_MIN_SIZE = 0x1AEu
};

typedef struct stf_attack_finish_inputs {
    int16_t attacker_energy_1ac;
    int16_t defender_energy_1ac;
    int16_t energy_max;
    bool disable_energy_scaling;
    uint32_t damage;
} stf_attack_finish_inputs;

typedef struct stf_attack_finish_result {
    uint32_t scaled_damage;
    int32_t energy_difference_percent;
    int32_t defender_energy_after;
    bool finish_blow;
    uint32_t bonus_skill;
} stf_attack_finish_result;

/*
 * Recover sub_19730 plus the finish-blow test at ah_hit_sd_end.
 *
 * The damage multiplier is:
 *   1 + clamp(defender_energy - attacker_energy, -30, dynamic_ceiling) * 0.01
 *
 * dynamic_ceiling = max(0, 50 - ((energy_max - defender_energy) >> 1)).
 * cvtri uses i960 default round-to-nearest-even.
 */
bool stf_attack_hit_finish_compute(
    const stf_attack_finish_inputs *inputs,
    stf_attack_finish_result *result
);

bool stf_attack_hit_finish_apply_model2(
    uint8_t *attacker,
    size_t attacker_size,
    const uint8_t *defender,
    size_t defender_size,
    int16_t energy_max,
    bool disable_energy_scaling,
    uint32_t damage,
    stf_attack_finish_result *result
);

#endif
