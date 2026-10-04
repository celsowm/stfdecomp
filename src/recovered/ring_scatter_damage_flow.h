#ifndef STF_RECOVERED_RING_SCATTER_DAMAGE_FLOW_H
#define STF_RECOVERED_RING_SCATTER_DAMAGE_FLOW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "damage_calculation.h"
#include "ring_scatter_runtime.h"

enum {
    STF_RING_DAMAGE_FLOW_FIGHTER_MIN_SIZE = 0x0218u
};

typedef struct stf_ring_damage_flow_inputs {
    uint8_t stage_num;
    uint32_t drop_random_values[16];
} stf_ring_damage_flow_inputs;

typedef struct stf_ring_damage_flow_result {
    bool requested;
    bool suppressed;
    bool request_ring_sound;
    uint8_t spawned_count;
    uint8_t recycled_count;
    uint8_t slot_indices[16];
} stf_ring_damage_flow_result;

/*
 * Compose the recovered damage_calculation ring event with ring_tobitiri_set.
 *
 * receiver/dealer are the same Model 2 fighter records passed to
 * stf_damage_calculation_apply_model2.  This adapter reads only the fields
 * consumed by ring_tobitiri_set and delegates policy/physics to the recovered
 * ring planner/runtime.
 */
bool stf_ring_scatter_apply_damage_event_model2(
    const stf_damage_calculation_result *damage,
    const uint8_t *receiver,
    size_t receiver_size,
    const uint8_t *dealer,
    size_t dealer_size,
    const stf_ring_damage_flow_inputs *inputs,
    stf_ring_pool *pool,
    stf_ring_slot slots[STF_RING_POOL_SLOT_COUNT],
    stf_ring_damage_flow_result *result
);

#endif
