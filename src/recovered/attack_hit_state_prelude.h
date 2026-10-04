#ifndef STF_RECOVERED_ATTACK_HIT_STATE_PRELUDE_H
#define STF_RECOVERED_ATTACK_HIT_STATE_PRELUDE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_ATTACK_HIT_STATE_PRELUDE_MIN_SIZE = 0x124Cu
};

typedef struct stf_attack_hit_state_prelude_result {
    uint32_t attacker_194;
    bool copied_122x;
    bool copied_124x;
    uint32_t attacker_121c;
    int16_t attacker_1220;
    int16_t attacker_1244;
} stf_attack_hit_state_prelude_result;

/*
 * Portable recovery of the attack_hit state prelude at the schamp
 * 0x2B0D0..0x2B124 corridor (sfight equivalent is anchored separately).
 *
 * The block:
 * - sets attacker +0x194 to 0x10000001;
 * - when (+0x1224 & 1) is set, copies +0x1228 -> +0x121C and
 *   signed +0x1226 -> +0x1220;
 * - when (+0x1248 & 1) is set, copies signed +0x124A -> +0x1244.
 */
bool stf_attack_hit_state_prelude_apply_model2(
    uint8_t *attacker,
    size_t attacker_size,
    stf_attack_hit_state_prelude_result *result
);

#endif
