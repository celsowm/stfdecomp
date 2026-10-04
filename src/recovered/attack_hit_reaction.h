#ifndef STF_RECOVERED_ATTACK_HIT_REACTION_H
#define STF_RECOVERED_ATTACK_HIT_REACTION_H

#include <stdbool.h>
#include <stdint.h>

typedef enum stf_attack_reaction_path {
    STF_ATTACK_REACTION_NORMAL = 0,
    STF_ATTACK_REACTION_GENERIC,
    STF_ATTACK_REACTION_DOWN_COMBO,
    STF_ATTACK_REACTION_SPECIAL_BIT16
} stf_attack_reaction_path;

typedef struct stf_attack_reaction_inputs {
    uint32_t defender_flags_1a4;
    uint32_t defender_flags_70c;
    uint16_t hit_flags_50fe00;
    uint32_t damage;
    uint8_t attacker_kind_821;
    uint32_t scaled_damage;
    int16_t defender_energy_1ac;
} stf_attack_reaction_inputs;

/*
 * Recover the pure reaction branch selection from ah_not_finish_blow through
 * loc_2B488. NORMAL means the code reaches the standard motion/stun path at
 * 0x2B488.
 */
bool stf_attack_hit_reaction_classify(
    const stf_attack_reaction_inputs *inputs,
    stf_attack_reaction_path *path
);

#endif
