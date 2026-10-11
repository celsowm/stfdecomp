#ifndef STF_RECOVERED_ATTACK_HIT_GUARD_VECTOR_H
#define STF_RECOVERED_ATTACK_HIT_GUARD_VECTOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_ATTACK_HIT_GUARD_VECTOR_ATTACKER_MIN_SIZE = 0x844u,
    STF_ATTACK_HIT_GUARD_VECTOR_DEFENDER_MIN_SIZE = 0x5ECu
};

typedef struct stf_attack_hit_guard_vector_query {
    uint32_t opcode;
    uint8_t argument_count;
    uint32_t words[2];
} stf_attack_hit_guard_vector_query;

/* Exactly one hardware reply word per command. The caller owns the actual
 * Model 2 TGP, including trigonometric/vector semantics and rounding. */
typedef bool (*stf_attack_hit_guard_vector_tgp_fn)(
    void *user_data,
    const stf_attack_hit_guard_vector_query *query,
    uint32_t *response_bits
);

typedef struct stf_attack_hit_guard_vector_result {
    uint8_t completed_commands;
    bool boosted;
    uint32_t initial_magnitude_bits;
    uint32_t unboosted[3]; /* defender +0x5E0 / +0x5E4 / +0x5E8 */
    uint32_t resulting[3];
} stf_attack_hit_guard_vector_result;

/*
 * i960 loc_2B738..loc_2B898 for BLOCK_A/B only (r8=r6=r11=0).
 * Runs five hardware commands:
 *   D001A1A(sqrt magnitude),
 *   12002424(0, sqrt), 12802525(0, sqrt),
 *   12002424(-combined_heading, response2),
 *   12802525(-combined_heading, response2).
 * The input strength is a supplied float (r9 at this boundary);
 * if a host-derived estimate is passed, this cannot claim ROM 1:1.
 *
 * If attacker+0x7D2 bit0 is set, pairs of 32-bit little-endian scales
 * from the ROM flt_2B904 table (hit_kind*8) scale +0x5E4 and the X/Z
 * components respectively. The table is never guessed or embedded.
 *
 * Failures never overwrite the three defender vector fields. Completed
 * command count is returned even for callback failure after progress.
 */
bool stf_attack_hit_guard_vector_apply_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    int32_t combined_heading,
    uint32_t strength_r9_bits,
    uint8_t hit_kind,
    const uint8_t *boost_table,
    size_t boost_table_size,
    stf_attack_hit_guard_vector_tgp_fn tgp,
    void *tgp_user_data,
    stf_attack_hit_guard_vector_result *result
);
#endif
