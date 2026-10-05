#include <stdint.h>
#include <string.h>

#include "crush_part_rom_view.h"

static void write_le16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
}

static void write_le32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
    p[2] = (uint8_t)(value >> 16u);
    p[3] = (uint8_t)(value >> 24u);
}

int main(void)
{
    uint8_t image[0x80];
    stf_crush_part_rom_view view;
    const int16_t *table = NULL;
    size_t count = 0u;
    size_t i;

    memset(image, 0, sizeof(image));
    memset(&view, 0, sizeof(view));

    for (i = 0u; i < STF_CRUSH_PART_SPIN_TABLE_COUNT; ++i) {
        write_le16(image + 0x20u + i * 2u, (uint16_t)(0x1000u + i));
    }

    view.image = image;
    view.image_size = sizeof(image);
    view.base_address = STF_CRUSH_PART_SPIN_TABLE_ADDRESS - 0x20u;

    if (!stf_crush_part_spin_table_resolve_rom(&view, &table, &count) ||
        table == NULL ||
        count != STF_CRUSH_PART_SPIN_TABLE_COUNT ||
        table[0] != INT16_C(0x1000) ||
        table[15] != INT16_C(0x100F)) {
        return 1;
    }

    view.base_address = STF_CRUSH_PART_SPIN_TABLE_ADDRESS + 1u;
    if (stf_crush_part_spin_table_resolve_rom(&view, &table, &count)) {
        return 2;
    }

    {
        uint8_t program[0x90];
        stf_crush_part_rom_view program_view;
        const stf_crush_part_speed_tables *tables = NULL;

        memset(program, 0, sizeof(program));
        memset(&program_view, 0, sizeof(program_view));

        write_le32(program + 0x00u, UINT32_C(0x3D75C28F));
        write_le32(program + 0x14u, UINT32_C(0x3DCCCCCD));
        write_le32(program + 0x18u, UINT32_C(0x3D23D70A));
        write_le16(program + 0x30u, UINT16_C(0xE940));
        write_le16(program + 0x4Eu, UINT16_C(0x1110));
        write_le32(program + 0x50u, UINT32_C(0x3D0B4396));
        write_le32(program + 0x8Cu, UINT32_C(0x3D8B4396));

        program_view.image = program;
        program_view.image_size = sizeof(program);
        program_view.base_address = STF_CRUSH_PART_SPEED_RADIAL_SCHAMP;

        if (!stf_crush_part_speed_tables_resolve_rom(
                &program_view,
                STF_CRUSH_PART_PROGRAM_SCHAMP,
                &tables
            ) ||
            tables == NULL ||
            tables->radial_profile_bits[0] != UINT32_C(0x3D75C28F) ||
            tables->radial_profile_bits[5] != UINT32_C(0x3DCCCCCD) ||
            tables->vertical_profile_bits[0] != UINT32_C(0x3D23D70A) ||
            (uint16_t)tables->angle_offsets[0] != UINT16_C(0xE940) ||
            (uint16_t)tables->angle_offsets[15] != UINT16_C(0x1110) ||
            tables->jitter_bits[0] != UINT32_C(0x3D0B4396) ||
            tables->jitter_bits[15] != UINT32_C(0x3D8B4396)) {
            return 3;
        }

        if (STF_CRUSH_PART_SPEED_RADIAL_SCHAMP -
                STF_CRUSH_PART_SPEED_RADIAL_SFIGHT != UINT32_C(0x2C) ||
            STF_CRUSH_PART_SPEED_VERTICAL_SCHAMP -
                STF_CRUSH_PART_SPEED_VERTICAL_SFIGHT != UINT32_C(0x2C) ||
            STF_CRUSH_PART_SPEED_ANGLE_SCHAMP -
                STF_CRUSH_PART_SPEED_ANGLE_SFIGHT != UINT32_C(0x2C) ||
            STF_CRUSH_PART_SPEED_JITTER_SCHAMP -
                STF_CRUSH_PART_SPEED_JITTER_SFIGHT != UINT32_C(0x2C)) {
            return 4;
        }
    }

    return 0;
}
