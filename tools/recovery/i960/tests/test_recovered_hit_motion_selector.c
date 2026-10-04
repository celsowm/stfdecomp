#include <stdint.h>
#include <string.h>

#include "hit_motion_selector.h"

int main(void)
{
    uint32_t row[48];
    stf_hit_motion_selector_inputs in;
    stf_hit_motion_selector_result out;

    memset(row, 0, sizeof(row));
    memset(&in, 0, sizeof(in));

    row[2] = 100u;
    if (!stf_hit_motion_select_from_row(&in, row, 48u, &out) ||
        out.table_index != 2u || out.raw_motion != 100u || out.motion != 100u) {
        return 1;
    }

    memset(&in, 0, sizeof(in));
    row[0] = 101u;
    in.source_82a = INT16_C(0x4000);
    if (!stf_hit_motion_select_from_row(&in, row, 48u, &out) ||
        out.table_index != 0u || out.motion != 101u) {
        return 2;
    }

    memset(&in, 0, sizeof(in));
    row[1] = 102u;
    in.selector_word = UINT16_C(0x8000);
    in.source_82a = INT16_C(0x4000);
    if (!stf_hit_motion_select_from_row(&in, row, 48u, &out) ||
        out.table_index != 1u || out.motion != 102u) {
        return 3;
    }

    memset(&in, 0, sizeof(in));
    row[40] = UINT32_C(225);
    in.slot = UINT32_C(5);
    in.target_1f8_bits = UINT32_C(0x3F666666);
    if (!stf_hit_motion_select_from_row(&in, row, 48u, &out) ||
        out.table_index != 40u || out.raw_motion != UINT32_C(225) ||
        out.motion != UINT32_C(0x106)) {
        return 4;
    }

    in.target_1f8_bits = UINT32_C(0x3F800000);
    if (!stf_hit_motion_select_from_row(&in, row, 48u, &out) ||
        out.motion != UINT32_C(225)) {
        return 5;
    }

    in.slot = UINT32_C(8);
    if (stf_hit_motion_select_from_row(&in, row, 48u, &out)) {
        return 6;
    }

    memset(&in, 0, sizeof(in));
    row[2] = 103u;
    in.target_1f8_bits = UINT32_C(0x7FC00000);
    if (!stf_hit_motion_select_from_row(&in, row, 48u, &out) ||
        out.motion != 103u) {
        return 7;
    }

    row[2] = UINT32_C(225);
    if (stf_hit_motion_select_from_row(&in, row, 48u, &out)) {
        return 8;
    }

    return 0;
}
