#include "motion_hit_table.h"

#include <stddef.h>

static const uint8_t default_record_sizes[STF_MOTION_HIT_RECORD_SIZE_COUNT] = {
    UINT8_C(0x0D), UINT8_C(0x03), UINT8_C(0x07), UINT8_C(0x0E),
    UINT8_C(0x12), UINT8_C(0x0E), UINT8_C(0x06), UINT8_C(0x03),
    UINT8_C(0x00), UINT8_C(0x0F), UINT8_C(0x06), UINT8_C(0x0B),
    UINT8_C(0x04), UINT8_C(0x03), UINT8_C(0x0F), UINT8_C(0x03),
    UINT8_C(0x02), UINT8_C(0x07), UINT8_C(0x07), UINT8_C(0x07),
    UINT8_C(0x03), UINT8_C(0x06), UINT8_C(0x05), UINT8_C(0x03),
    UINT8_C(0x0D),
};

const uint8_t *stf_motion_hit_default_record_sizes(size_t *count)
{
    if (count != NULL) {
        *count = STF_MOTION_HIT_RECORD_SIZE_COUNT;
    }
    return default_record_sizes;
}

stf_motion_hit_lookup_status stf_motion_hit_table_find_offset(
    uint32_t selector,
    uint8_t target_tag,
    const uint32_t *animation_offsets,
    size_t animation_count,
    const uint8_t *motion_blob,
    size_t motion_blob_size,
    const uint8_t *record_size_by_tag,
    size_t record_size_count,
    uint32_t *record_offset
)
{
    const uint32_t index = selector & UINT32_C(0x1FFF);
    uint64_t cursor = 0u;
    size_t steps = 0u;

    if (animation_offsets == NULL || motion_blob == NULL ||
        record_size_by_tag == NULL || record_offset == NULL ||
        index >= animation_count) {
        return STF_MOTION_HIT_INVALID;
    }

    cursor = (uint64_t)animation_offsets[index] + UINT64_C(0x0D);
    if (cursor >= motion_blob_size) {
        return STF_MOTION_HIT_INVALID;
    }

    for (steps = 0u; steps < motion_blob_size; ++steps) {
        const uint8_t tag = motion_blob[cursor];
        uint8_t stride = 0u;

        if (tag == target_tag) {
            *record_offset = (uint32_t)cursor;
            return STF_MOTION_HIT_FOUND;
        }

        if (tag == 0u || tag == 8u) {
            return STF_MOTION_HIT_NOT_FOUND;
        }

        if ((size_t)tag >= record_size_count) {
            return STF_MOTION_HIT_INVALID;
        }

        stride = record_size_by_tag[tag];
        if (stride == 0u) {
            return STF_MOTION_HIT_INVALID;
        }

        cursor += stride;
        if (cursor >= motion_blob_size) {
            return STF_MOTION_HIT_INVALID;
        }
    }

    return STF_MOTION_HIT_INVALID;
}
