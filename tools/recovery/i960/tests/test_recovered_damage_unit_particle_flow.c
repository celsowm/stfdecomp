#include <stdint.h>
#include <string.h>

#include "damage_unit_particle_flow.h"

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8u) |
           ((uint32_t)data[2] << 16u) |
           ((uint32_t)data[3] << 24u);
}

int main(void)
{
    uint8_t slots[
        STF_COLLISION_PARTICLE_EXTENDED_SLOTS *
        STF_COLLISION_PARTICLE_SLOT_SIZE
    ];
    stf_collision_particle_descriptor desc[STF_COLLISION_PARTICLE_KIND_COUNT];
    stf_collision_particle_stage stage;
    stf_damage_unit_post_result post;
    stf_damage_unit_particle_flow_result result;

    memset(slots, 0, sizeof(slots));
    memset(desc, 0, sizeof(desc));
    memset(&stage, 0, sizeof(stage));
    memset(&post, 0, sizeof(post));

    desc[1].effect_address = UINT32_C(0x2000);
    desc[1].initial_value = UINT32_C(0x33445566);
    stage.position_a[0] = UINT32_C(0x11111111);
    stage.position_b[2] = UINT32_C(0x22222222);
    stage.flags = UINT8_C(0x08);

    if (!stf_damage_unit_particle_flow_apply_model2(
            &post,
            UINT8_C(0),
            desc,
            slots,
            sizeof(slots),
            &stage,
            &result
        ) ||
        result.requested ||
        stage.flags != UINT8_C(0x08) ||
        stage.position_a[0] != UINT32_C(0x11111111)) {
        return 1;
    }

    post.request_particle_setup = true;
    post.particle_draw_kind = UINT16_C(1);
    post.particle_flag_set_mask = UINT8_C(1);

    if (!stf_damage_unit_particle_flow_apply_model2(
            &post,
            UINT8_C(0),
            desc,
            slots,
            sizeof(slots),
            &stage,
            &result
        ) ||
        !result.requested ||
        !result.particle.requested ||
        !result.particle.allocated ||
        result.particle.slot_index != UINT8_C(0) ||
        read_le32(slots + 0x00u) != UINT32_C(0x11111111) ||
        read_le32(slots + 0x14u) != UINT32_C(0x22222222) ||
        slots[0x19u] != UINT8_C(0x09) ||
        read_le32(slots + 0x1Cu) != UINT32_C(0x2000) ||
        read_le32(slots + 0x20u) != UINT32_C(0x33445566) ||
        stage.draw_particle != UINT16_C(0) ||
        stage.flags != UINT8_C(0x09) ||
        stage.position_a[0] != UINT32_C(0) ||
        stage.position_b[2] != UINT32_C(0)) {
        return 2;
    }

    return 0;
}
