#ifndef STF_RECOVERED_ATTACK_HIT_MOTION_SELECT_H
#define STF_RECOVERED_ATTACK_HIT_MOTION_SELECT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef bool (*stf_hit_motion_table_resolver)(
    uint8_t table_selector,
    const uint32_t **table_words,
    size_t *table_word_count,
    void *user_data
);

typedef struct stf_hit_motion_select_inputs {
    uint16_t hit_flags_50fe00;
    uint32_t motion_group;
    uint32_t attacker_flags_0;
    uint32_t defender_flags_0;
    uint32_t defender_flags_1a4;
    int16_t attacker_82a;
    int16_t attacker_26;
    int16_t defender_5b4;
    uint32_t defender_1f8_bits;
} stf_hit_motion_select_inputs;

typedef struct stf_hit_motion_select_result {
    uint8_t table_selector;
    uint32_t table_index;
    uint32_t motion;
    bool used_special_group5_index;
    bool used_distance_variant;
    bool used_down_override;
} stf_hit_motion_select_result;

/*
 * Portable recovery of sub_2B94C + sub_2BA44.
 *
 * The original uk_hit_motions tables remain caller-supplied. The resolver
 * receives the low byte of hit_flags_50fe00 and returns the selected 32-bit
 * motion table.
 */
bool stf_attack_hit_motion_select(
    const stf_hit_motion_select_inputs *inputs,
    stf_hit_motion_table_resolver resolver,
    void *resolver_user_data,
    stf_hit_motion_select_result *result
);

#endif
