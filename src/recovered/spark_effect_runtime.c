#include "spark_effect_runtime.h"

#include <string.h>

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

static void write_le16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8u);
}

static void write_le32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8u);
    p[2] = (uint8_t)(v >> 16u);
    p[3] = (uint8_t)(v >> 24u);
}

bool stf_spark_effect_init_slot_model2(
    uint8_t *slot,
    size_t slot_size,
    const uint32_t position[3],
    const stf_spark_effect_descriptor *descriptor,
    uint32_t frame_table_token
)
{
    if (slot == NULL || position == NULL || descriptor == NULL ||
        slot_size < STF_SPARK_EFFECT_SLOT_SIZE) {
        return false;
    }

    write_le32(slot + 0x00u, position[0]);
    write_le32(slot + 0x04u, position[1]);
    write_le32(slot + 0x08u, position[2]);
    write_le32(slot + 0x0Cu, descriptor->scale_x_bits);
    write_le32(slot + 0x10u, descriptor->scale_y_bits);
    write_le16(slot + 0x18u, descriptor->duration);
    write_le16(slot + 0x1Au, UINT16_C(0));
    write_le32(slot + 0x1Cu, frame_table_token);
    write_le32(slot + 0x20u, descriptor->scale_z_bits);
    return true;
}

bool stf_spark_effect_update_model2(
    uint8_t *slots,
    size_t slots_size,
    stf_spark_effect_update_result *result
)
{
    stf_spark_effect_update_result local;
    unsigned i;

    if (slots == NULL || result == NULL ||
        slots_size < STF_SPARK_EFFECT_SLOT_COUNT * STF_SPARK_EFFECT_SLOT_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    for (i = 0u; i < STF_SPARK_EFFECT_SLOT_COUNT; ++i) {
        uint8_t *slot = slots + (size_t)i * STF_SPARK_EFFECT_SLOT_SIZE;
        uint16_t countdown = read_le16(slot + 0x18u);
        uint16_t frame_index;

        if (countdown == UINT16_C(0)) {
            continue;
        }

        ++local.active_before;
        frame_index = read_le16(slot + 0x1Au);
        --countdown;
        ++frame_index;
        write_le16(slot + 0x18u, countdown);
        write_le16(slot + 0x1Au, frame_index);
        ++local.advanced;
        if (countdown == UINT16_C(0)) {
            ++local.expired;
        }
    }

    *result = local;
    return true;
}

bool stf_spark_effect_build_draws_model2(
    const uint8_t *slots,
    size_t slots_size,
    const stf_spark_effect_descriptor descriptors[STF_SPARK_EFFECT_SLOT_COUNT],
    const uint32_t frame_table_tokens[STF_SPARK_EFFECT_SLOT_COUNT],
    stf_spark_effect_draw *draws,
    size_t draw_capacity,
    stf_spark_effect_render_result *result
)
{
    stf_spark_effect_render_result local;
    unsigned i;

    if (slots == NULL || descriptors == NULL || frame_table_tokens == NULL ||
        result == NULL ||
        slots_size < STF_SPARK_EFFECT_SLOT_COUNT * STF_SPARK_EFFECT_SLOT_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    for (i = 0u; i < STF_SPARK_EFFECT_SLOT_COUNT; ++i) {
        const uint8_t *slot = slots + (size_t)i * STF_SPARK_EFFECT_SLOT_SIZE;
        uint16_t frame_index;
        const stf_spark_effect_descriptor *descriptor = &descriptors[i];

        if (read_le16(slot + 0x18u) == UINT16_C(0)) {
            continue;
        }
        ++local.active_slots;

        if (draws == NULL || local.draw_count >= draw_capacity ||
            read_le32(slot + 0x1Cu) != frame_table_tokens[i]) {
            return false;
        }

        frame_index = read_le16(slot + 0x1Au);
        if (descriptor->frames == NULL || frame_index >= descriptor->frame_count) {
            return false;
        }

        {
            stf_spark_effect_draw *draw = &draws[local.draw_count++];
            memset(draw, 0, sizeof(*draw));
            draw->slot_index = (uint8_t)i;
            draw->position[0] = read_le32(slot + 0x00u);
            draw->position[1] = read_le32(slot + 0x04u);
            draw->position[2] = read_le32(slot + 0x08u);
            draw->scale[0] = read_le32(slot + 0x0Cu);
            draw->scale[1] = read_le32(slot + 0x10u);
            draw->scale[2] = read_le32(slot + 0x20u);
            draw->frame_id = descriptor->frames[frame_index];
        }
    }

    *result = local;
    return true;
}
