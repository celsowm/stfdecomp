#include "kamae_motion_rom_view.h"

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8u) |
           ((uint32_t)data[2] << 16u) |
           ((uint32_t)data[3] << 24u);
}

bool stf_kamae_motion_resolve_rom(
    uint16_t selector,
    const uint8_t **motion_record,
    size_t *motion_record_size,
    void *user_data
)
{
    const stf_kamae_motion_rom_view *view =
        (const stf_kamae_motion_rom_view *)user_data;
    uint64_t table_offset = 0u;
    uint64_t entry_offset = 0u;
    uint32_t record_address = 0u;
    size_t record_offset = 0u;

    if (view == NULL || view->image == NULL ||
        motion_record == NULL || motion_record_size == NULL ||
        (size_t)selector >= view->motion_count ||
        view->offset_list_address < view->base_address) {
        return false;
    }

    table_offset =
        (uint64_t)view->offset_list_address - view->base_address;
    entry_offset = table_offset + (uint64_t)selector * 4u;

    if (entry_offset > view->image_size ||
        4u > view->image_size - (size_t)entry_offset) {
        return false;
    }

    record_address = read_le32(view->image + (size_t)entry_offset);
    if (record_address < view->base_address) {
        return false;
    }

    record_offset = (size_t)(record_address - view->base_address);
    if (record_offset >= view->image_size) {
        return false;
    }

    *motion_record = view->image + record_offset;
    *motion_record_size = view->image_size - record_offset;
    return true;
}
