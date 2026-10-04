#ifndef STF_RECOVERED_ATTACK_HIT_NORMAL_REACTION_H
#define STF_RECOVERED_ATTACK_HIT_NORMAL_REACTION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_NORMAL_REACTION_ATTACKER_MIN_SIZE = 0x85Fu,
    STF_NORMAL_REACTION_DEFENDER_MIN_SIZE = 0x5E0u
};

typedef struct stf_normal_reaction_result {
    uint32_t defender_198;
    int16_t defender_5de;
    int32_t reaction_argument;
    bool used_mode_specific_85e;
    bool used_hit_flag_floor;
    bool used_defender_bit16_override;
    bool requires_sub_2b94c;
} stf_normal_reaction_result;

/*
 * Recover the normal reaction path 0x2B488..0x2B554.
 *
 * motion_g0 is the external sub_2B94C result selected from hit_mode & 0x1f.
 * The returned reaction_argument is the r4 value after loc_2B554.
 */
bool stf_attack_hit_normal_reaction_apply_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint32_t damage,
    uint32_t hit_mode,
    uint8_t hit_flags_50fe03,
    uint32_t motion_g0,
    stf_normal_reaction_result *result
);

#endif
