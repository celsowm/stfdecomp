#ifndef STF_RECOVERED_SPARK_EFFECT_RUNTIME_H
#define STF_RECOVERED_SPARK_EFFECT_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_SPARK_EFFECT_SLOT_COUNT = 6u,
    STF_SPARK_EFFECT_SLOT_SIZE = 0x20u,
    STF_SPARK_EFFECT_SLOT_ACCESS_SIZE = 0x24u,
    STF_SPARK_EFFECT_POOL_SIZE =
        STF_SPARK_EFFECT_SLOT_COUNT * STF_SPARK_EFFECT_SLOT_SIZE + 4u
};

typedef struct stf_spark_effect_descriptor {
    uint16_t duration;
    uint32_t scale_x_bits;
    uint32_t scale_y_bits;
    uint32_t scale_z_bits;
    const uint16_t *frames;
    size_t frame_count;
} stf_spark_effect_descriptor;

typedef struct stf_spark_effect_draw {
    uint8_t slot_index;
    uint32_t position[3];
    uint32_t scale[3];
    uint16_t frame_id;
} stf_spark_effect_draw;

typedef struct stf_spark_effect_update_result {
    uint8_t active_before;
    uint8_t advanced;
    uint8_t expired;
} stf_spark_effect_update_result;

typedef struct stf_spark_effect_render_result {
    uint8_t active_slots;
    uint8_t draw_count;
} stf_spark_effect_render_result;

/*
 * Recover the six 0x20-byte effect slots at mod_fa_effect+0x790.
 *
 * sub_327E8 owns the CPU lifecycle: active slots use +0x18 as a countdown and
 * +0x1A as a frame-table index. Each tick decrements the countdown and
 * increments the index. efc_disp resolves frame_table[index], uses +0x00 as
 * position, and +0x0C/+0x10/+0x20 as independent XYZ scales.
 *
 * The original stride is 0x20 even though +0x20 is accessed. This means each
 * slot's Z-scale aliases the next slot's +0x00 X-position; the final slot
 * reaches four bytes beyond the six-slot stride region. The portable model
 * intentionally preserves that layout and therefore requires POOL_SIZE bytes.
 */
bool stf_spark_effect_init_slot_model2(
    uint8_t *slot,
    size_t slot_size,
    const uint32_t position[3],
    const stf_spark_effect_descriptor *descriptor,
    uint32_t frame_table_token
);

bool stf_spark_effect_update_model2(
    uint8_t *slots,
    size_t slots_size,
    stf_spark_effect_update_result *result
);

bool stf_spark_effect_build_draws_model2(
    const uint8_t *slots,
    size_t slots_size,
    const stf_spark_effect_descriptor descriptors[STF_SPARK_EFFECT_SLOT_COUNT],
    const uint32_t frame_table_tokens[STF_SPARK_EFFECT_SLOT_COUNT],
    stf_spark_effect_draw *draws,
    size_t draw_capacity,
    stf_spark_effect_render_result *result
);

#endif
