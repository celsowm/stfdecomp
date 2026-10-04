#ifndef STF_RECOVERED_RING_SCATTER_RUNTIME_H
#define STF_RECOVERED_RING_SCATTER_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "ring_scatter_plan.h"

typedef enum stf_ring_trajectory {
    STF_RING_TRAJECTORY_A = 0,
    STF_RING_TRAJECTORY_B,
    STF_RING_TRAJECTORY_C
} stf_ring_trajectory;

typedef struct stf_ring_profile_record {
    uint8_t max_ring_index;
    uint32_t velocity_scale_bits;
    stf_ring_trajectory trajectory;
} stf_ring_profile_record;

typedef struct stf_ring_slot {
    bool active;
    uint8_t source_ring_index;
    uint16_t age;
    uint32_t x_bits;
    uint32_t y_bits;
    uint32_t z_bits;
    uint32_t vx_bits;
    uint32_t vz_bits;
    stf_ring_trajectory trajectory;
    uint16_t blink_from_frame;
    uint16_t expire_at_frame;
    stf_ring_drop_mode drop_mode;
} stf_ring_slot;

typedef struct stf_ring_scatter_spawn_inputs {
    uint32_t defender_angle_x_bits;
    uint32_t defender_angle_z_bits;
    uint32_t attacker_angle_x_bits;
    uint32_t attacker_angle_z_bits;
    uint32_t spawn_x_bits;
    uint32_t spawn_y_bits;
    uint32_t spawn_z_bits;
} stf_ring_scatter_spawn_inputs;

typedef struct stf_ring_scatter_spawn_result {
    uint8_t spawned_count;
    uint8_t recycled_count;
    uint8_t slot_indices[16];
} stf_ring_scatter_spawn_result;

typedef struct stf_ring_trajectory_info {
    uint32_t sfight_rom_address;
    uint32_t schamp_rom_address;
    uint16_t sample_count;
} stf_ring_trajectory_info;

typedef struct stf_ring_tick_inputs {
    bool paused;
    const uint32_t *curve_words;
    size_t curve_word_count;
} stf_ring_tick_inputs;

typedef struct stf_ring_tick_result {
    bool released;
    bool landed;
    bool visible;
    uint32_t render_x_bits;
    uint32_t render_y_bits;
    uint32_t render_z_bits;
} stf_ring_tick_result;

/*
 * The profile records below are the small CPU-visible descriptor tables used by
 * ring_tobitiri_set.  Large per-frame trajectory curves remain external ROM
 * evidence and are represented by A/B/C IDs instead of being copied here.
 *
 * sfight trajectory bases:
 *   A = 0x000AE488
 *   B = 0x000AE618
 *   C = 0x000AE7F8
 *
 * Sonic Championship secondary evidence shifts those same tables by +0x138.
 */
bool stf_ring_profile_select(
    stf_ring_scatter_profile profile,
    uint8_t ring_index,
    stf_ring_profile_record *record
);

/*
 * Describe and consume the ROM-backed per-frame vertical trajectory curves.
 *
 * The repository deliberately does not embed the curve payloads. Callers may
 * provide words extracted from their own sfight/schamp ROM. Each table is a
 * run of IEEE-754 samples followed by the exact -1.0f sentinel used by the
 * original program.
 */
bool stf_ring_trajectory_info_get(
    stf_ring_trajectory trajectory,
    stf_ring_trajectory_info *info
);

bool stf_ring_trajectory_sample_bits(
    stf_ring_trajectory trajectory,
    const uint32_t *curve_words,
    size_t curve_word_count,
    uint16_t frame,
    uint32_t *sample_bits
);

/*
 * Recover the spawn half of ring_tobitiri_set:
 * - cpres1 horizontal atan/Y rotation;
 * - the eight 16-byte local position/velocity records;
 * - ROM profile threshold/scale/trajectory selection;
 * - 24-slot acquire/recycle and slot initialization.
 *
 * The caller supplies fighter positions from the two source coordinate sets
 * used by the original routine: +0x1F4/+0x1FC for yaw and +0x20C..+0x214 for
 * the actual spawn origin.
 */
bool stf_ring_scatter_spawn(
    const stf_ring_scatter_plan *plan,
    const stf_ring_scatter_spawn_inputs *inputs,
    stf_ring_pool *pool,
    stf_ring_slot slots[STF_RING_POOL_SLOT_COUNT],
    stf_ring_scatter_spawn_result *result
);

/*
 * Recover the CPU-visible per-frame ring_tobitiri lifecycle observed in the
 * Sonic Championship 0x78E84..0x791F4 corridor:
 * - pause freezes age and physics;
 * - expiry unlinks/releases the slot before physics;
 * - landed rings keep their stored -1.0f Y sentinel and render at Y=0;
 * - horizontal motion bounces at +/-7.5 by flipping velocity sign;
 * - a zero trajectory sample damps X/Z velocity by 0.7;
 * - the trajectory -1.0f sentinel transitions the slot to landed state;
 * - after blink_from_frame, bit 1 of age controls two-on/two-off visibility.
 *
 * curve_words points at the A/B/C table selected by slot->trajectory and must
 * include its trailing -1.0f sentinel.
 */
bool stf_ring_slot_tick(
    const stf_ring_tick_inputs *inputs,
    stf_ring_pool *pool,
    uint8_t slot_index,
    stf_ring_slot slots[STF_RING_POOL_SLOT_COUNT],
    stf_ring_tick_result *result
);

#endif
