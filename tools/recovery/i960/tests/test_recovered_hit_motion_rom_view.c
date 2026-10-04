#include <stdint.h>
#include <string.h>

#include "hit_motion_rom_view.h"

static void write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

int main(void)
{
    uint8_t image[1024];
    stf_hit_motion_rom_view view;
    const uint32_t *table = NULL;
    size_t count = 0u;

    memset(image, 0, sizeof(image));
    memset(&view, 0, sizeof(view));

    view.image = image;
    view.image_size = sizeof(image);
    view.base_address = UINT32_C(0x00300000);
    view.character_table_address = UINT32_C(0x00300020);
    view.character_count = 4u;
    view.selector_count = 8u;
    view.character = UINT8_C(2);

    write_le32(
        image + 0x20u + 2u * 4u,
        UINT32_C(0x00300080)
    );
    write_le32(
        image + 0x80u + 3u * 4u,
        UINT32_C(0x00300180)
    );

    write_le32(image + 0x180u + 0u * 4u, UINT32_C(108));
    write_le32(image + 0x180u + 10u * 4u, UINT32_C(0x123));
    write_le32(image + 0x180u + 40u * 4u, UINT32_C(59));

    if (!stf_hit_motion_resolve_rom(
            UINT8_C(3), &table, &count, &view
        ) ||
        count != STF_HIT_MOTION_TABLE_WORD_COUNT ||
        table[0] != UINT32_C(108) ||
        table[10] != UINT32_C(0x123) ||
        table[40] != UINT32_C(59)) {
        return 1;
    }

    view.character = UINT8_C(4);
    if (stf_hit_motion_resolve_rom(
            UINT8_C(3), &table, &count, &view
        )) {
        return 2;
    }

    view.character = UINT8_C(2);
    write_le32(
        image + 0x80u + 3u * 4u,
        UINT32_C(0x002FFFFF)
    );
    if (stf_hit_motion_resolve_rom(
            UINT8_C(3), &table, &count, &view
        )) {
        return 3;
    }

    return 0;
}
