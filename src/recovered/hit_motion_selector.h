#ifndef STF_RECOVERED_HIT_MOTION_SELECTOR_H
#define STF_RECOVERED_HIT_MOTION_SELECTOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct stf_hit_motion_selector_inputs {
    uint32_t slot;
    uint16_t selector_word;
    uint32_t source_flags_0;
    uint32_t target_flags_0;
    uint32_t target_flags_1a4;
    int16_t source_82a;
    int16_t source_26;
    int16_t target_5b4;
    uint32_t target_1f8_bits;
} stf_hit_motion_selector_inputs;

typedef struct stf_hit_motion_selector_result {
    size_t table_index;
    uint32_t raw_motion;
    uint32_t motion;
} stf_hit_motion_selector_result;

/*
 * Portable recovery of sub_2B94C + sub_2BA44 after the outer motion-row
 * pointer has been resolved.
 *
 * source_* corresponds to g7 in sub_2B94C and target_* to g8. selector_word
 * is the 16-bit value loaded from 0x50FE00. The caller supplies the row that
 * r11 points to; this helper recovers the deterministic inner index selection
 * and the SNC_DOWN remap without assuming the external table's layout.
 */
bool stf_hit_motion_select_from_row(
    const stf_hit_motion_selector_inputs *inputs,
    const uint32_t *motion_row,
    size_t motion_row_count,
    stf_hit_motion_selector_result *result
);

#endif
