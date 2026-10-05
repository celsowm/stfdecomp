#include "crush_part_rom_view.h"

static uint16_t read_le16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8u));
}

bool stf_crush_part_spin_table_resolve_rom(
    stf_crush_part_rom_view *view,
    const int16_t **spin_table,
    size_t *spin_count
)
{
    uint64_t offset;
    size_t i;

    if (view == NULL || view->image == NULL ||
        spin_table == NULL || spin_count == NULL ||
        STF_CRUSH_PART_SPIN_TABLE_ADDRESS < view->base_address) {
        return false;
    }

    offset =
        (uint64_t)STF_CRUSH_PART_SPIN_TABLE_ADDRESS - view->base_address;

    if (offset > view->image_size ||
        STF_CRUSH_PART_SPIN_TABLE_COUNT * sizeof(uint16_t) >
            view->image_size - (size_t)offset) {
        return false;
    }

    for (i = 0u; i < STF_CRUSH_PART_SPIN_TABLE_COUNT; ++i) {
        view->spin_table[i] = (int16_t)read_le16(
            view->image + (size_t)offset + i * sizeof(uint16_t)
        );
    }

    *spin_table = view->spin_table;
    *spin_count = STF_CRUSH_PART_SPIN_TABLE_COUNT;
    return true;
}
