#ifndef STF_RECOVERED_ATTACK_HIT_DOWN_REACTION_H
#define STF_RECOVERED_ATTACK_HIT_DOWN_REACTION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum stf_down_reaction_route {
    STF_DOWN_REACTION_GENERIC = 0,
    STF_DOWN_REACTION_SPECIAL_BIT16
} stf_down_reaction_route;

enum {
    STF_DOWN_REACTION_DEFENDER_MIN_SIZE = 0x6F6u
};

typedef struct stf_down_reaction_result {
    uint32_t defender_198;
    int16_t defender_5de;
    uint32_t selected_motion;
    uint8_t defender_down_combo_6f5;
    bool requires_sub_2b94c;
    bool requires_calc_mht;
    bool used_motion_override;
    bool used_height_motion_fixup;
} stf_down_reaction_result;

/* Recover loc_2B5E4 routing. */
bool stf_attack_hit_down_route(
    uint32_t attacker_flags_1a4,
    uint32_t defender_flags_1a4,
    stf_down_reaction_route *route
);

/* Recover loc_2B560 state, after the route selected SPECIAL_BIT16. */
bool stf_attack_hit_special_bit16_reaction_apply_model2(
    uint8_t *defender,
    size_t defender_size,
    uint32_t damage,
    uint32_t motion_g0,
    stf_down_reaction_result *result
);

/*
 * Recover loc_2B594/loc_2B5F8 through the write to defender+0x5DE.
 * increment_down_combo corresponds to entering through loc_2B594.
 */
bool stf_attack_hit_generic_down_reaction_apply_model2(
    uint8_t *defender,
    size_t defender_size,
    uint32_t damage,
    uint8_t hit_kind_50fe02,
    uint32_t motion_g0,
    bool increment_down_combo,
    stf_down_reaction_result *result
);

#endif
