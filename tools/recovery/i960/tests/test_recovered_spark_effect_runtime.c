#include <stdint.h>
#include <string.h>

#include "spark_effect_runtime.h"

static uint16_t read_le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8u));
}

static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8u) |
           ((uint32_t)p[2] << 16u) |
           ((uint32_t)p[3] << 24u);
}

int main(void)
{
    uint8_t slots[STF_SPARK_EFFECT_POOL_SIZE];
    static const uint16_t frames0[] = { 10, 11, 12, 13 };
    stf_spark_effect_descriptor desc[STF_SPARK_EFFECT_SLOT_COUNT];
    uint32_t tokens[STF_SPARK_EFFECT_SLOT_COUNT];
    uint32_t pos[3] = {
        UINT32_C(0x11111111),
        UINT32_C(0x22222222),
        UINT32_C(0x33333333)
    };
    stf_spark_effect_update_result update;
    stf_spark_effect_render_result render;
    stf_spark_effect_draw draws[STF_SPARK_EFFECT_SLOT_COUNT];

    memset(slots, 0, sizeof(slots));
    memset(desc, 0, sizeof(desc));
    memset(tokens, 0, sizeof(tokens));
    memset(draws, 0, sizeof(draws));

    desc[0].duration = UINT16_C(3);
    desc[0].scale_x_bits = UINT32_C(0x3F800000);
    desc[0].scale_y_bits = UINT32_C(0x40000000);
    desc[0].scale_z_bits = UINT32_C(0x40400000);
    desc[0].frames = frames0;
    desc[0].frame_count = 4u;
    tokens[0] = UINT32_C(0x12345678);

    if (!stf_spark_effect_init_slot_model2(
            slots, STF_SPARK_EFFECT_SLOT_ACCESS_SIZE,
            pos, &desc[0], tokens[0]
        ) ||
        read_le32(slots + 0x00u) != pos[0] ||
        read_le32(slots + 0x04u) != pos[1] ||
        read_le32(slots + 0x08u) != pos[2] ||
        read_le16(slots + 0x18u) != UINT16_C(3) ||
        read_le16(slots + 0x1Au) != UINT16_C(0) ||
        read_le32(slots + 0x1Cu) != tokens[0]) {
        return 1;
    }

    if (!stf_spark_effect_build_draws_model2(
            slots, sizeof(slots),
            desc, tokens,
            draws, STF_SPARK_EFFECT_SLOT_COUNT,
            &render
        ) ||
        render.active_slots != UINT8_C(1) ||
        render.draw_count != UINT8_C(1) ||
        draws[0].frame_id != UINT16_C(10) ||
        draws[0].scale[0] != UINT32_C(0x3F800000) ||
        draws[0].scale[1] != UINT32_C(0x40000000) ||
        draws[0].scale[2] != UINT32_C(0x40400000)) {
        return 2;
    }

    if (read_le32(slots + STF_SPARK_EFFECT_SLOT_SIZE) != UINT32_C(0x40400000)) {
        return 7;
    }

    if (!stf_spark_effect_update_model2(
            slots, sizeof(slots), &update
        ) ||
        update.active_before != UINT8_C(1) ||
        update.advanced != UINT8_C(1) ||
        update.expired != UINT8_C(0) ||
        read_le16(slots + 0x18u) != UINT16_C(2) ||
        read_le16(slots + 0x1Au) != UINT16_C(1)) {
        return 3;
    }

    if (!stf_spark_effect_build_draws_model2(
            slots, sizeof(slots),
            desc, tokens,
            draws, STF_SPARK_EFFECT_SLOT_COUNT,
            &render
        ) ||
        draws[0].frame_id != UINT16_C(11)) {
        return 4;
    }

    if (!stf_spark_effect_update_model2(slots, sizeof(slots), &update) ||
        !stf_spark_effect_update_model2(slots, sizeof(slots), &update) ||
        read_le16(slots + 0x18u) != UINT16_C(0) ||
        read_le16(slots + 0x1Au) != UINT16_C(3) ||
        update.expired != UINT8_C(1)) {
        return 5;
    }

    if (!stf_spark_effect_build_draws_model2(
            slots, sizeof(slots),
            desc, tokens,
            draws, STF_SPARK_EFFECT_SLOT_COUNT,
            &render
        ) ||
        render.active_slots != UINT8_C(0) ||
        render.draw_count != UINT8_C(0)) {
        return 6;
    }

    return 0;
}
