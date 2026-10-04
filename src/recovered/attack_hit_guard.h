#ifndef STF_RECOVERED_ATTACK_HIT_GUARD_H
#define STF_RECOVERED_ATTACK_HIT_GUARD_H

#include <stdbool.h>
#include <stdint.h>

typedef enum stf_attack_guard_path {
    STF_ATTACK_GUARD_EXIT = 0,
    STF_ATTACK_GUARD_CONTINUE,
    STF_ATTACK_GUARD_BRANCH_2AC74,
    STF_ATTACK_GUARD_BLOCK_A_2AA70,
    STF_ATTACK_GUARD_BLOCK_B_2AB54
} stf_attack_guard_path;

typedef struct stf_attack_guard_inputs {
    uint32_t opponent_flags_1a4;
    uint32_t opponent_flags_5b8;
    uint32_t attacker_flags_000;
    int16_t attacker_field_c7c;
    int32_t opponent_field_c70;
    uint16_t hit_flags_50fe00;
} stf_attack_guard_inputs;

/*
 * Recover the pure branch classifier at attack_hit 0x2A9F0..0x2AA70.
 * Side effects inside the selected branches remain outside this contract.
 */
bool stf_attack_hit_guard_classify(
    const stf_attack_guard_inputs *inputs,
    stf_attack_guard_path *path
);

#endif
