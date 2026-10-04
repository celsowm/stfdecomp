#ifndef STF_RECOVERED_RING_SCATTER_RUNTIME_H
#define STF_RECOVERED_RING_SCATTER_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>

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
    uint16_t visible_from_frame;
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

#endif
