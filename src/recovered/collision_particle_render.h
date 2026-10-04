#ifndef STF_RECOVERED_COLLISION_PARTICLE_RENDER_H
#define STF_RECOVERED_COLLISION_PARTICLE_RENDER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "collision_particle_runtime.h"

typedef struct stf_collision_particle_draw {
    uint8_t slot_index;
    uint8_t flags;
    bool use_saved_graphics_state;
    bool suppress_orientation;
    uint32_t position[3];
    uint32_t scale_bits;
    uint16_t frame_id;
} stf_collision_particle_draw;

typedef struct stf_collision_particle_render_result {
    uint8_t active_slots;
    uint8_t draw_count;
} stf_collision_particle_render_result;

/*
 * Recover the collision-particle portion of efc_disp.
 *
 * The original routine scans exactly 16 slots. Each active slot contributes
 * one set_obj call using slot +0x00..+0x08 as position, +0x20 replicated as
 * XYZ scale, and +0x1A as the object/frame id. Flag bit 2 suppresses the
 * orientation words normally sourced from g13; flag bit 0 wraps the draw in
 * the original graphics-state save/restore sequence.
 *
 * This helper emits portable draw commands instead of touching Model 2 command
 * RAM or invoking set_obj directly.
 */
bool stf_collision_particle_build_draws_model2(
    const uint8_t *slot_bytes,
    size_t slot_bytes_size,
    stf_collision_particle_draw *draws,
    size_t draw_capacity,
    stf_collision_particle_render_result *result
);

#endif
