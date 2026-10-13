#include "collision_particle_frame_runtime.h"

#include <string.h>

bool stf_collision_particle_frame_step_model2(
    uint8_t *slot_bytes,
    size_t slot_bytes_size,
    const stf_collision_particle_descriptor
        descriptors[STF_COLLISION_PARTICLE_KIND_COUNT],
    stf_collision_particle_draw *draws,
    size_t draw_capacity,
    stf_collision_particle_frame_result *result
)
{
    enum {
        ACTIVE_BYTES = STF_COLLISION_PARTICLE_NORMAL_SLOTS *
                       STF_COLLISION_PARTICLE_SLOT_SIZE
    };
    uint8_t staged_slots[ACTIVE_BYTES];
    stf_collision_particle_draw staged_draws[STF_COLLISION_PARTICLE_NORMAL_SLOTS];
    stf_collision_particle_frame_result local;

    if (slot_bytes == NULL || descriptors == NULL || result == NULL ||
        slot_bytes_size < sizeof(staged_slots) ||
        (draw_capacity != 0u && draws == NULL)) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    memcpy(staged_slots, slot_bytes, sizeof(staged_slots));

    if (!stf_collision_particle_update_model2(
            staged_slots, sizeof(staged_slots), descriptors, &local.update)) {
        return false;
    }
    local.updated = true;

    /* Bound internal writes to the 16 real visible slots, and only copy to
     * caller output when the requested number of draws fits. */
    if (!stf_collision_particle_build_draws_model2(
            staged_slots, sizeof(staged_slots),
            staged_draws, STF_COLLISION_PARTICLE_NORMAL_SLOTS,
            &local.render)) {
        return false;
    }
    if ((size_t)local.render.draw_count > draw_capacity) {
        return false;
    }
    local.draws_built = true;

    memcpy(slot_bytes, staged_slots, sizeof(staged_slots));
    if (local.render.draw_count != 0u) {
        memcpy(draws, staged_draws,
               (size_t)local.render.draw_count * sizeof(staged_draws[0]));
    }
    *result = local;
    return true;
}

bool stf_collision_particle_frame_step_combat_model2(
    const stf_collision_combat_model2 *combat,
    stf_collision_particle_draw *draws,
    size_t draw_capacity,
    stf_collision_particle_frame_result *result
)
{
    if (combat == NULL || !combat->recover_damage_particles) {
        return false;
    }
    return stf_collision_particle_frame_step_model2(
        combat->damage_particle_slots,
        combat->damage_particle_slots_size,
        combat->damage_particle_descriptors,
        draws, draw_capacity, result);
}
