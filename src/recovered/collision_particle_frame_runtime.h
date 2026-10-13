#ifndef STF_RECOVERED_COLLISION_PARTICLE_FRAME_RUNTIME_H
#define STF_RECOVERED_COLLISION_PARTICLE_FRAME_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "collision_particle_render.h"
#include "collision_combat_runtime.h"

typedef struct stf_collision_particle_frame_result {
    bool updated;
    bool draws_built;
    stf_collision_particle_update_result update;
    stf_collision_particle_render_result render;
} stf_collision_particle_frame_result;

/*
 * One EXPLICIT particle frame step: sub_32B10 slot update, followed by
 * recovered efc_disp draw-command extraction. This only describes the
 * requested update-then-draw order: the host must schedule it at the
 * correct place in the original game loop, independently from hit
 * collision/particle allocation.
 *
 * The original updater AND renderer scan exactly slots 0..15, even when
 * the collision allocator is in a 32-slot submode. Slots 16..31 are
 * deliberately untouched and not drawn here.
 *
 * The first sixteen 0x24-byte records are staged so that malformed
 * descriptor/frame data or insufficient draw capacity does not leave
 * partially advanced particle state or partially written draw commands.
 * Scratch storage is fixed-size; the caller retains ownership of
 * all RAM and effect descriptor/frame tables.
 */
bool stf_collision_particle_frame_step_model2(
    uint8_t *slot_bytes,
    size_t slot_bytes_size,
    const stf_collision_particle_descriptor
        descriptors[STF_COLLISION_PARTICLE_KIND_COUNT],
    stf_collision_particle_draw *draws,
    size_t draw_capacity,
    stf_collision_particle_frame_result *result
);

/* Use the SAME live particle slots and ROM descriptors configured for the
 * recovered damage_unit in a collision_combat_model2. Call this separately
 * at the game's particle tick; it is not invoked by collision/combat,
 * because the exact scheduler position still needs hardware evidence. */
bool stf_collision_particle_frame_step_combat_model2(
    const stf_collision_combat_model2 *combat,
    stf_collision_particle_draw *draws,
    size_t draw_capacity,
    stf_collision_particle_frame_result *result
);

#endif
