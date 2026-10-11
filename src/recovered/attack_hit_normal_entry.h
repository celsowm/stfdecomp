#ifndef STF_RECOVERED_ATTACK_HIT_NORMAL_ENTRY_H
#define STF_RECOVERED_ATTACK_HIT_NORMAL_ENTRY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attack_hit.h"
#include "attack_hit_guard.h"
#include "attack_hit_profile.h"
#include "attack_hit_strength.h"

/*
 * CPU-visible seam between attack_hit loc_2A8F8 and the guard-branch
 * classifier at 0x2A9F0. The intervening coprocessor sqrt is represented
 * by the existing semantic host scalar helper. It is NOT bit-exact to
 * the original Model 2 TGP. A real TGP trace remains needed.
 *
 * The caller supplies the original 40-byte-per-hit-kind table at
 * 0x50A800; no values are hardcoded or reconstructed from guesses.
 */
enum {
    STF_ATTACK_HIT_NORMAL_ENTRY_ATTACKER_MIN_SIZE = 0xC80u,
    STF_ATTACK_HIT_NORMAL_ENTRY_DEFENDER_MIN_SIZE = 0xC74u,
    STF_ATTACK_HIT_NORMAL_ENTRY_WORKSPACE_MIN_SIZE = 0x270u
};

typedef struct stf_attack_hit_normal_entry_result {
    stf_attack_hit_strength_input strength;
    stf_attack_hit_profile profile;
    uint32_t host_strength_scale_bits;
    stf_attack_guard_path guard_branch;
    bool host_strength_is_approximate;
} stf_attack_hit_normal_entry_result;

/*
 * Validate every buffer and profile before mutating workspace bit 0.
 * The supplied prefix carries the actual hit flags (including the
 * defender-bit29 override). This call does NOT dispatch the guard/reaction
 * branches; callers must continue through the unresolved attack_hit body.
 */
bool stf_attack_hit_normal_entry_apply_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    const uint8_t *defender,
    size_t defender_size,
    uint8_t *workspace,
    size_t workspace_size,
    const stf_attack_hit_prefix_result *prefix,
    const uint8_t *profile_table,
    size_t profile_table_size,
    stf_attack_hit_normal_entry_result *result
);
#endif
