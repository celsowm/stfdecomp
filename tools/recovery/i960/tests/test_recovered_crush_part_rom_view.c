#include <stdint.h>
#include <string.h>

#include "crush_part_rom_view.h"

static void write_le16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
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

    return 0;
}
