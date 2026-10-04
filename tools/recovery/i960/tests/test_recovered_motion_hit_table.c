#include <stdint.h>
#include <string.h>

#include "motion_hit_table.h"

int main(void)
{
    uint32_t offsets[0x124];
    uint8_t blob[64];
    uint8_t sizes[32];
    uint32_t found = 0u;

    memset(offsets, 0, sizeof(offsets));
    memset(blob, 0, sizeof(blob));
    memset(sizes, 0, sizeof(sizes));

    offsets[0x123u] = 10u;
    sizes[5u] = 4u;
    sizes[7u] = 3u;

    blob[23u] = 5u;  /* 10 + 0x0d */
    blob[27u] = 7u;
    blob[30u] = 17u;

    if (stf_motion_hit_table_find_offset(
            UINT32_C(0x2123),
            UINT8_C(17),
            offsets,
            sizeof(offsets) / sizeof(offsets[0]),
            blob,
            sizeof(blob),
            sizes,
            sizeof(sizes),
            &found
        ) != STF_MOTION_HIT_FOUND ||
        found != UINT32_C(30)) {
        return 1;
    }

    blob[30u] = 8u;
    if (stf_motion_hit_table_find_offset(
            UINT32_C(0x0123),
            UINT8_C(17),
            offsets,
            sizeof(offsets) / sizeof(offsets[0]),
            blob,
            sizeof(blob),
            sizes,
            sizeof(sizes),
            &found
        ) != STF_MOTION_HIT_NOT_FOUND) {
        return 2;
    }

    blob[30u] = 9u; /* no stride */
    if (stf_motion_hit_table_find_offset(
            UINT32_C(0x0123),
            UINT8_C(17),
            offsets,
            sizeof(offsets) / sizeof(offsets[0]),
            blob,
            sizeof(blob),
            sizes,
            sizeof(sizes),
            &found
        ) != STF_MOTION_HIT_INVALID) {
        return 3;
    }

    if (stf_motion_hit_table_find_offset(
            UINT32_C(0x1FFF),
            UINT8_C(1),
            offsets,
            sizeof(offsets) / sizeof(offsets[0]),
            blob,
            sizeof(blob),
            sizes,
            sizeof(sizes),
            &found
        ) != STF_MOTION_HIT_INVALID) {
        return 4;
    }

    return 0;
}
