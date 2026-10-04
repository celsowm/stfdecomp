#include <stdint.h>
#include <string.h>

#include "motion_hit_rom_view.h"

static void write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

int main(void)
{
    uint8_t image[256];
    stf_motion_hit_rom_view view;
    uint32_t record_address = 0u;

    memset(image, 0, sizeof(image));
    memset(&view, 0, sizeof(view));

    view.image = image;
    view.image_size = sizeof(image);
    view.base_address = UINT32_C(0x00100000);
    view.animation_related_address = UINT32_C(0x00100020);
    view.animation_count = 8u;

    write_le32(
        image + 0x20u + 3u * 4u,
        UINT32_C(0x00100080)
    );

    image[0x80u + 0x0Du] = UINT8_C(5);
    image[0x80u + 0x0Du + 0x0Eu] = UINT8_C(0x11);

    if (stf_motion_hit_rom_find(
            &view,
            UINT32_C(3),
            UINT8_C(0x11),
            &record_address
        ) != STF_MOTION_HIT_FOUND ||
        record_address != UINT32_C(0x0010009B)) {
        return 1;
    }

    image[0x80u + 0x0Du + 0x0Eu] = UINT8_C(8);
    if (stf_motion_hit_rom_find(
            &view,
            UINT32_C(3),
            UINT8_C(0x11),
            &record_address
        ) != STF_MOTION_HIT_NOT_FOUND) {
        return 2;
    }

    write_le32(
        image + 0x20u + 3u * 4u,
        UINT32_C(0x000FFFFF)
    );
    if (stf_motion_hit_rom_find(
            &view,
            UINT32_C(3),
            UINT8_C(0x11),
            &record_address
        ) != STF_MOTION_HIT_INVALID) {
        return 3;
    }

    return 0;
}
