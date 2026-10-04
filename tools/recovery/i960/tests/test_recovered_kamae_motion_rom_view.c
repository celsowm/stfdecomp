#include <stdint.h>
#include <string.h>

#include "kamae_motion_rom_view.h"

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
    stf_kamae_motion_rom_view view;
    const uint8_t *record = NULL;
    size_t record_size = 0u;

    memset(image, 0, sizeof(image));
    memset(&view, 0, sizeof(view));

    view.image = image;
    view.image_size = sizeof(image);
    view.base_address = UINT32_C(0x00200000);
    view.offset_list_address = UINT32_C(0x00200020);
    view.motion_count = 8u;

    write_le32(image + 0x20u + 3u * 4u, UINT32_C(0x00200180));
    image[0x180u] = UINT8_C(0xAB);

    if (!stf_kamae_motion_resolve_rom(
            UINT16_C(3), &record, &record_size, &view
        ) ||
        record != image + 0x180u ||
        record_size != sizeof(image) - 0x180u ||
        record[0] != UINT8_C(0xAB)) {
        return 1;
    }

    if (stf_kamae_motion_resolve_rom(
            UINT16_C(8), &record, &record_size, &view
        )) {
        return 2;
    }

    write_le32(image + 0x20u + 2u * 4u, UINT32_C(0x001FFFFF));
    if (stf_kamae_motion_resolve_rom(
            UINT16_C(2), &record, &record_size, &view
        )) {
        return 3;
    }

    return 0;
}
