#ifndef STF_RECOVERED_RING_SCATTER_PLAN_H
#define STF_RECOVERED_RING_SCATTER_PLAN_H

#include <stdbool.h>
#include <stdint.h>

enum {
    STF_RING_POOL_SLOT_COUNT = 24u,
    STF_RING_POOL_EMPTY = 99u,
    STF_RING_SPECIAL_MOTION = 0x1B4u,
    STF_RING_SPECIAL_STAGE = 2u
};

typedef enum stf_ring_scatter_profile {
    STF_RING_SCATTER_NONE = 0,
    STF_RING_SCATTER_DAMAGE_2,
    STF_RING_SCATTER_DAMAGE_4,
    STF_RING_SCATTER_DAMAGE_8,
    STF_RING_SCATTER_DAMAGE_16,
    STF_RING_SCATTER_SPECIAL_MOTION
} stf_ring_scatter_profile;

typedef enum stf_ring_drop_mode {
    STF_RING_DROP_RING = 0,
    STF_RING_DROP_RANDOM_1_TO_4,
    STF_RING_DROP_FIXED_5
} stf_ring_drop_mode;

typedef struct stf_ring_scatter_inputs {
    uint32_t damage;
    uint8_t defender_character;
    uint16_t defender_motion_1a8;
    uint8_t stage_num;
} stf_ring_scatter_inputs;

typedef struct stf_ring_scatter_plan {
    bool suppressed;
    bool request_ring_sound;
    uint8_t ring_count;
    uint16_t visible_from_frame;
    uint16_t expire_at_frame;
    stf_ring_scatter_profile profile;
    stf_ring_drop_mode drop_mode;
} stf_ring_scatter_plan;

typedef struct stf_ring_pool {
    uint32_t occupied_mask;
    uint8_t head;
} stf_ring_pool;

/*
 * CPU-only recovery of ring_tobitiri_set at 0x78788..0x78B34.
 *
 * This planner intentionally stops before the Model 2 coprocessor vector
 * transform and ROM-backed velocity/profile table dereference. It recovers the
 * branch policy that is portable gameplay behavior: character exclusions,
 * sound policy, ring count, lifetime profile, special drop kind, and the
 * 24-slot occupancy scan used by spanbit_ring/setbit_ring.
 */
bool stf_ring_scatter_plan_compute(
    const stf_ring_scatter_inputs *inputs,
    stf_ring_scatter_plan *plan
);

void stf_ring_pool_init(stf_ring_pool *pool);

bool stf_ring_pool_allocate(stf_ring_pool *pool, uint8_t *slot);

void stf_ring_pool_release(stf_ring_pool *pool, uint8_t slot);

#endif
