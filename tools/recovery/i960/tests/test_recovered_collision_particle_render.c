#include <stdint.h>
#include <string.h>

#include "collision_particle_render.h"

static void write_le16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
}

static void write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

static int test_extracts_active_draws(void)
{
    uint8_t slots[
        STF_COLLISION_PARTICLE_EXTENDED_SLOTS *
        STF_COLLISION_PARTICLE_SLOT_SIZE
    ];
    stf_collision_particle_draw draws[STF_COLLISION_PARTICLE_NORMAL_SLOTS];
    stf_collision_particle_render_result result;
    uint8_t *slot;

    memset(slots, 0, sizeof(slots));
    memset(draws, 0, sizeof(draws));

    slot = slots + 2u * STF_COLLISION_PARTICLE_SLOT_SIZE;
    write_le32(slot + 0x00u, UINT32_C(0x11111111));
    write_le32(slot + 0x04u, UINT32_C(0x22222222));
    write_le32(slot + 0x08u, UINT32_C(0x33333333));
    slot[0x19u] = (uint8_t)((UINT8_C(1) << 0u) | (UINT8_C(1) << 2u));
    write_le16(slot + 0x1Au, UINT16_C(3501));
    write_le32(slot + 0x1Cu, UINT32_C(0x12340000));
    write_le32(slot + 0x20u, UINT32_C(0x3F800000));

    if (!stf_collision_particle_build_draws_model2(
            slots, sizeof(slots),
            draws, STF_COLLISION_PARTICLE_NORMAL_SLOTS,
            &result
        ) ||
        result.active_slots != UINT8_C(1) ||
        result.draw_count != UINT8_C(1) ||
        draws[0].slot_index != UINT8_C(2) ||
        !draws[0].use_saved_graphics_state ||
        !draws[0].suppress_orientation ||
        draws[0].position[0] != UINT32_C(0x11111111) ||
        draws[0].position[1] != UINT32_C(0x22222222) ||
        draws[0].position[2] != UINT32_C(0x33333333) ||
        draws[0].scale_bits != UINT32_C(0x3F800000) ||
        draws[0].frame_id != UINT16_C(3501)) {
        return 1;
    }

    return 0;
}

static int test_render_scans_only_first_16_slots(void)
{
    uint8_t slots[
        STF_COLLISION_PARTICLE_EXTENDED_SLOTS *
        STF_COLLISION_PARTICLE_SLOT_SIZE
    ];
    stf_collision_particle_draw draws[STF_COLLISION_PARTICLE_NORMAL_SLOTS];
    stf_collision_particle_render_result result;
    uint8_t *slot16;

    memset(slots, 0, sizeof(slots));
    memset(draws, 0, sizeof(draws));

    slot16 = slots + 16u * STF_COLLISION_PARTICLE_SLOT_SIZE;
    write_le32(slot16 + 0x1Cu, UINT32_C(1));

    if (!stf_collision_particle_build_draws_model2(
            slots, sizeof(slots),
            draws, STF_COLLISION_PARTICLE_NORMAL_SLOTS,
            &result
        ) ||
        result.active_slots != UINT8_C(0) ||
        result.draw_count != UINT8_C(0)) {
        return 1;
    }

    return 0;
}

static int test_requires_output_capacity(void)
{
    uint8_t slots[
        STF_COLLISION_PARTICLE_EXTENDED_SLOTS *
        STF_COLLISION_PARTICLE_SLOT_SIZE
    ];
    stf_collision_particle_draw draw;
    stf_collision_particle_render_result result;

    memset(slots, 0, sizeof(slots));
    memset(&draw, 0, sizeof(draw));
    write_le32(slots + 0x1Cu, UINT32_C(1));

    if (stf_collision_particle_build_draws_model2(
            slots, sizeof(slots),
            &draw, 0u,
            &result
        )) {
        return 1;
    }

    return 0;
}

int main(void)
{
    if (test_extracts_active_draws() != 0) return 1;
    if (test_render_scans_only_first_16_slots() != 0) return 1;
    if (test_requires_output_capacity() != 0) return 1;
    return 0;
}
