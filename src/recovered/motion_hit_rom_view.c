#include "motion_hit_rom_view.h"

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8u) |
           ((uint32_t)data[2] << 16u) |
           ((uint32_t)data[3] << 24u);
}

stf_motion_hit_lookup_status stf_motion_hit_rom_find(
    const stf_motion_hit_rom_view *view,
    uint32_t selector,
    uint8_t target_tag,
    uint32_t *record_address
)
{
    uint32_t index = selector & UINT32_C(0x1FFF);
    uint64_t table_offset = 0u;
    uint64_t pointer_offset = 0u;
    uint32_t motion_address = 0u;
    uint64_t motion_offset = 0u;
    uint32_t relative_record = 0u;
    const uint8_t *sizes = NULL;
    size_t size_count = 0u;
    uint32_t one_offset = 0u;
    stf_motion_hit_lookup_status status;

    if (view == NULL || view->image == NULL || record_address == NULL ||
        index >= view->animation_count ||
        view->animation_related_address < view->base_address) {
        return STF_MOTION_HIT_INVALID;
    }

    table_offset =
        (uint64_t)view->animation_related_address - view->base_address;
    pointer_offset = table_offset + (uint64_t)index * 4u;
    if (pointer_offset > view->image_size ||
        4u > view->image_size - (size_t)pointer_offset) {
        return STF_MOTION_HIT_INVALID;
    }

    motion_address = read_le32(view->image + (size_t)pointer_offset);
    if (motion_address < view->base_address) {
        return STF_MOTION_HIT_INVALID;
    }

    motion_offset = (uint64_t)motion_address - view->base_address;
    if (motion_offset >= view->image_size) {
        return STF_MOTION_HIT_INVALID;
    }

    one_offset = (uint32_t)motion_offset;
    sizes = stf_motion_hit_default_record_sizes(&size_count);
    status = stf_motion_hit_table_find_offset(
        UINT32_C(0),
        target_tag,
        &one_offset,
        1u,
        view->image,
        view->image_size,
        sizes,
        size_count,
        &relative_record
    );

    if (status != STF_MOTION_HIT_FOUND) {
        return status;
    }

    *record_address = view->base_address + relative_record;
    return STF_MOTION_HIT_FOUND;
}
