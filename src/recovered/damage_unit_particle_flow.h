#ifndef STF_RECOVERED_DAMAGE_UNIT_PARTICLE_FLOW_H
#define STF_RECOVERED_DAMAGE_UNIT_PARTICLE_FLOW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "collision_particle_runtime.h"
#include "damage_unit_post.h"

typedef struct stf_damage_unit_particle_flow_result {
    bool requested;
    stf_collision_particle_result particle;
} stf_damage_unit_particle_flow_result;

/*
 * Compose the particle-producing branches at the end of damage_unit with the
 * recovered sub_32A5C allocator.
 *
 * position_a/position_b in stage represent the already-populated globals
 * 0x50A3FC/0x50A408. This adapter applies only the writes performed locally by
 * damage_unit (draw kind and flag bit) before invoking the allocator.
 */
bool stf_damage_unit_particle_flow_apply_model2(
    const stf_damage_unit_post_result *post,
    uint8_t also_sub_mode,
    const stf_collision_particle_descriptor
        descriptors[STF_COLLISION_PARTICLE_KIND_COUNT],
    uint8_t *slot_bytes,
    size_t slot_bytes_size,
    stf_collision_particle_stage *stage,
    stf_damage_unit_particle_flow_result *result
);

#endif
