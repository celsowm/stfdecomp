#include "crush_part_rom_view.h"

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

static bool resolve_offset(
    const stf_crush_part_rom_view *view,
    uint32_t address,
    size_t size,
    size_t *offset
)
{
    uint64_t delta;

    if (view == NULL || view->image == NULL || offset == NULL ||
        address < view->base_address) {
        return false;
    }

    delta = (uint64_t)address - view->base_address;
    if (delta > view->image_size || size > view->image_size - (size_t)delta) {
        return false;
    }

    *offset = (size_t)delta;
    return true;
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


bool stf_crush_part_speed_tables_resolve_rom(
    stf_crush_part_rom_view *view,
    stf_crush_part_program_variant variant,
    const stf_crush_part_speed_tables **tables
)
{
    uint32_t radial_address;
    uint32_t vertical_address;
    uint32_t angle_address;
    uint32_t jitter_address;
    size_t radial_offset;
    size_t vertical_offset;
    size_t angle_offset;
    size_t jitter_offset;
    size_t i;

    if (view == NULL || tables == NULL) {
        return false;
    }

    switch (variant) {
    case STF_CRUSH_PART_PROGRAM_SFIGHT:
        radial_address = STF_CRUSH_PART_SPEED_RADIAL_SFIGHT;
        vertical_address = STF_CRUSH_PART_SPEED_VERTICAL_SFIGHT;
        angle_address = STF_CRUSH_PART_SPEED_ANGLE_SFIGHT;
        jitter_address = STF_CRUSH_PART_SPEED_JITTER_SFIGHT;
        break;
    case STF_CRUSH_PART_PROGRAM_SCHAMP:
        radial_address = STF_CRUSH_PART_SPEED_RADIAL_SCHAMP;
        vertical_address = STF_CRUSH_PART_SPEED_VERTICAL_SCHAMP;
        angle_address = STF_CRUSH_PART_SPEED_ANGLE_SCHAMP;
        jitter_address = STF_CRUSH_PART_SPEED_JITTER_SCHAMP;
        break;
    default:
        return false;
    }

    if (!resolve_offset(view, radial_address, 6u * sizeof(uint32_t), &radial_offset) ||
        !resolve_offset(view, vertical_address, 6u * sizeof(uint32_t), &vertical_offset) ||
        !resolve_offset(view, angle_address, 16u * sizeof(uint16_t), &angle_offset) ||
        !resolve_offset(view, jitter_address, 16u * sizeof(uint32_t), &jitter_offset)) {
        return false;
    }

    for (i = 0u; i < 6u; ++i) {
        view->speed_tables.radial_profile_bits[i] =
            read_le32(view->image + radial_offset + i * sizeof(uint32_t));
        view->speed_tables.vertical_profile_bits[i] =
            read_le32(view->image + vertical_offset + i * sizeof(uint32_t));
    }

    for (i = 0u; i < 16u; ++i) {
        view->speed_tables.angle_offsets[i] = (int16_t)read_le16(
            view->image + angle_offset + i * sizeof(uint16_t)
        );
        view->speed_tables.jitter_bits[i] =
            read_le32(view->image + jitter_offset + i * sizeof(uint32_t));
    }

    *tables = &view->speed_tables;
    return true;
}
