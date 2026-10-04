#ifndef STF_RECOVERED_COLLISION_PARTICLE_RUNTIME_H
#define STF_RECOVERED_COLLISION_PARTICLE_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_COLLISION_PARTICLE_KIND_COUNT = 11u,
    STF_COLLISION_PARTICLE_SLOT_SIZE = 0x24u,
    STF_COLLISION_PARTICLE_NORMAL_SLOTS = 16u,
    STF_COLLISION_PARTICLE_EXTENDED_SLOTS = 32u
};

typedef struct stf_collision_particle_descriptor {
    uint32_t effect_address;
    uint32_t initial_value;
} stf_collision_particle_descriptor;

typedef struct stf_collision_particle_stage {
    uint32_t position_a[3];
    uint32_t position_b[3];
    uint16_t draw_particle;
    uint8_t flags;
} stf_collision_particle_stage;

typedef struct stf_collision_particle_result {
    bool requested;
    bool allocated;
    uint8_t slot_index;
    uint8_t slot_limit;
    uint32_t effect_address;
    uint32_t initial_value;
} stf_collision_particle_result;

/*
 * Recover sub_32A5C, the collision-particle slot allocator.
 *
 * The original collision_particle_effects table contains ROM addresses.
 * Callers provide the resolved address token and first 32-bit value for each
 * particle kind, keeping proprietary effect data outside the portable core.
 *
 * also_sub_mode 0x1A/0x1B exposes 32 slots; all other modes scan 16.
 * A non-zero request consumes the first free 0x24-byte slot (slot +0x1C == 0)
 * and clears the staged position vectors/draw kind whether allocation succeeds
 * or the pool is full. A zero draw kind returns without clearing the stage.
 */
bool stf_collision_particle_put_model2(
    uint8_t *slot_bytes,
    size_t slot_bytes_size,
    uint8_t also_sub_mode,
    const stf_collision_particle_descriptor
        descriptors[STF_COLLISION_PARTICLE_KIND_COUNT],
    stf_collision_particle_stage *stage,
    stf_collision_particle_result *result
);

#endif
