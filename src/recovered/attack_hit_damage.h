#ifndef STF_RECOVERED_ATTACK_HIT_DAMAGE_H
#define STF_RECOVERED_ATTACK_HIT_DAMAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_ATTACK_DAMAGE_ATTACKER_MIN_SIZE = 0xC7Eu,
    STF_ATTACK_DAMAGE_DEFENDER_MIN_SIZE = 0x823u
};

typedef enum stf_attack_damage_reason {
    STF_ATTACK_DAMAGE_HIT = 0,
    STF_ATTACK_DAMAGE_OC_DP,
    STF_ATTACK_DAMAGE_OC_DOWN,
    STF_ATTACK_DAMAGE_ORG_COMBO,
    STF_ATTACK_DAMAGE_UKEMI,
    STF_ATTACK_DAMAGE_COUNTER,
    STF_ATTACK_DAMAGE_SMALL_COUNTER,
    STF_ATTACK_DAMAGE_DOWN,
    STF_ATTACK_DAMAGE_TETSU,
    STF_ATTACK_DAMAGE_AIR,
    STF_ATTACK_DAMAGE_TENDER,
    STF_ATTACK_DAMAGE_HIT_ALT,
    STF_ATTACK_DAMAGE_MULTI
} stf_attack_damage_reason;

typedef struct stf_attack_damage_result {
    uint32_t damage;
    stf_attack_damage_reason reason;
    uint32_t hit_mode;
    uint32_t attacker_194;
    uint32_t attacker_flags_000;
} stf_attack_damage_result;

/*
 * Recover attack_hit damage/context transform 0x2B0F8..0x2B318.
 *
 * The combo tables are the STF-direct constants at flt_2E4BC/flt_2E4E0.
 * cvtri behavior is reproduced as round-to-nearest-even (default i960 mode).
 */
bool stf_attack_hit_damage_apply_model2(
    uint8_t *attacker,
    size_t attacker_size,
    const uint8_t *defender,
    size_t defender_size,
    uint32_t initial_damage,
    stf_attack_damage_result *result
);

#endif
