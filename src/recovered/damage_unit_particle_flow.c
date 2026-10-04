#include "damage_unit_particle_flow.h"

#include <string.h>

bool stf_damage_unit_particle_flow_apply_model2(
    const stf_damage_unit_post_result *post,
    uint8_t also_sub_mode,
    const stf_collision_particle_descriptor
        descriptors[STF_COLLISION_PARTICLE_KIND_COUNT],
    uint8_t *slot_bytes,
    size_t slot_bytes_size,
    stf_collision_particle_stage *stage,
    stf_damage_unit_particle_flow_result *result
)
{
    stf_damage_unit_particle_flow_result local;

    if (post == NULL || descriptors == NULL || slot_bytes == NULL ||
        stage == NULL || result == NULL) {
        return false;
    }

    memset(&local, 0, sizeof(local));

    if (!post->request_particle_setup) {
        *result = local;
        return true;
    }

    local.requested = true;
    stage->draw_particle = post->particle_draw_kind;
    stage->flags = (uint8_t)(stage->flags | post->particle_flag_set_mask);

    if (!stf_collision_particle_put_model2(
            slot_bytes,
            slot_bytes_size,
            also_sub_mode,
            descriptors,
            stage,
            &local.particle
        )) {
        return false;
    }

    *result = local;
    return true;
}
