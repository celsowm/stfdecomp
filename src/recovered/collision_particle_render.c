#include "collision_particle_render.h"

#include <string.h>

static uint16_t read_le16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8u));
}

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8u) |
           ((uint32_t)data[2] << 16u) |
           ((uint32_t)data[3] << 24u);
}

bool stf_collision_particle_build_draws_model2(
    const uint8_t *slot_bytes,
    size_t slot_bytes_size,
    stf_collision_particle_draw *draws,
    size_t draw_capacity,
    stf_collision_particle_render_result *result
)
{
    stf_collision_particle_render_result local;
    unsigned index = 0u;

    if (slot_bytes == NULL || result == NULL ||
        slot_bytes_size <
            (size_t)STF_COLLISION_PARTICLE_NORMAL_SLOTS *
            STF_COLLISION_PARTICLE_SLOT_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));

    for (index = 0u; index < STF_COLLISION_PARTICLE_NORMAL_SLOTS; ++index) {
        const uint8_t *slot =
            slot_bytes + (size_t)index * STF_COLLISION_PARTICLE_SLOT_SIZE;
        stf_collision_particle_draw *draw = NULL;
        const uint8_t flags = slot[0x19u];

        if (read_le32(slot + 0x1Cu) == UINT32_C(0)) {
            continue;
        }

        ++local.active_slots;

        if (draws == NULL || local.draw_count >= draw_capacity) {
            return false;
        }

        draw = &draws[local.draw_count++];
        memset(draw, 0, sizeof(*draw));
        draw->slot_index = (uint8_t)index;
        draw->flags = flags;
        draw->use_saved_graphics_state =
            (flags & (uint8_t)(UINT8_C(1) << 0u)) != 0u;
        draw->suppress_orientation =
            (flags & (uint8_t)(UINT8_C(1) << 2u)) != 0u;
        draw->position[0] = read_le32(slot + 0x00u);
        draw->position[1] = read_le32(slot + 0x04u);
        draw->position[2] = read_le32(slot + 0x08u);
        draw->scale_bits = read_le32(slot + 0x20u);
        draw->frame_id = read_le16(slot + 0x1Au);
    }

    *result = local;
    return true;
}
