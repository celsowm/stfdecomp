#ifndef STF_RECOVERED_ATTACK_HIT_MOTION_PREFIX_H
#define STF_RECOVERED_ATTACK_HIT_MOTION_PREFIX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum stf_motion_prefix_source {
    STF_MOTION_PREFIX_RECORD = 0,
    STF_MOTION_PREFIX_PROFILE
} stf_motion_prefix_source;

typedef struct stf_motion_fallback_profile {
    uint32_t scale_normal_bits;
    uint32_t scale_mode3_bits;
    uint32_t scale_down_bits;
    uint32_t scale_down_mode3_bits;
    int16_t angle_normal;
    int16_t angle_mode3;
    int16_t angle_down;
    int16_t angle_down_mode3;
} stf_motion_fallback_profile;

typedef struct stf_motion_prefix_inputs {
    uint32_t initial_r9_bits;
    uint32_t defender_flags_1a4;
    uint32_t hit_mode;
    uint8_t defender_combo_6f5;
    uint8_t combo_start;
    uint16_t combo_sub;
    uint16_t combo_limit;
    int16_t limit_xang;
    uint32_t defender_5d8_bits;
} stf_motion_prefix_inputs;

typedef struct stf_motion_prefix_result {
    stf_motion_prefix_source source;
    int32_t angle_r6;
    uint32_t scaled_r9_bits;
    uint32_t sqrt_r4_bits;
} stf_motion_prefix_result;

/*
 * Recover loc_2B624..loc_2B7E0 up to, but not including, command 0x12002424.
 *
 * When mht_record is non-NULL it must point at the record returned by
 * calc_mht_adr and contain at least 7 bytes. Otherwise fallback_profile is
 * used, matching the original r7 table selection.
 */
bool stf_attack_hit_motion_prefix_compute(
    const stf_motion_prefix_inputs *inputs,
    const uint8_t *mht_record,
    size_t mht_record_size,
    const stf_motion_fallback_profile *fallback_profile,
    stf_motion_prefix_result *result
);

#endif
