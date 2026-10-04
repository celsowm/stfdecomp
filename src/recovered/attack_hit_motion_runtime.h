#ifndef STF_RECOVERED_ATTACK_HIT_MOTION_RUNTIME_H
#define STF_RECOVERED_ATTACK_HIT_MOTION_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attack_hit_motion_prefix.h"
#include "motion_hit_table.h"
#include "motion_hit_rom_view.h"

typedef struct stf_attack_hit_motion_runtime_result {
    stf_motion_hit_lookup_status lookup_status;
    uint32_t record_offset;
    bool used_mht_record;
    stf_motion_prefix_result prefix;
} stf_attack_hit_motion_runtime_result;

/*
 * Compose attack_hit's post-reaction motion setup:
 *
 *   g0 = selected hit motion
 *   g1 = 0x11
 *   calc_mht_adr()
 *   found ? MHT record : 0x50A800 hit-kind fallback profile
 *
 * animation_offsets/motion_blob/record_size_by_tag are caller-supplied
 * portable representations of the original motion data.
 */
bool stf_attack_hit_motion_prefix_resolve_rom(
    uint32_t selected_motion,
    const stf_motion_prefix_inputs *prefix_inputs,
    const stf_motion_hit_rom_view *rom_view,
    const stf_motion_fallback_profile *fallback_profile,
    stf_attack_hit_motion_runtime_result *result
);

bool stf_attack_hit_motion_prefix_resolve(
    uint32_t selected_motion,
    const stf_motion_prefix_inputs *prefix_inputs,
    const uint32_t *animation_offsets,
    size_t animation_count,
    const uint8_t *motion_blob,
    size_t motion_blob_size,
    const uint8_t *record_size_by_tag,
    size_t record_size_count,
    const stf_motion_fallback_profile *fallback_profile,
    stf_attack_hit_motion_runtime_result *result
);

#endif
