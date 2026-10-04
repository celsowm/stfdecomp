#include "hit_motion_selector.h"

#include <math.h>
#include <string.h>

static float bits_to_float(uint32_t bits)
{
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static bool remap_down_motion(
    uint32_t motion,
    uint32_t target_1f8_bits,
    uint32_t *mapped_motion
)
{
    float target_1f8 = 0.0f;

    if (mapped_motion == NULL) {
        return false;
    }

    if (motion != UINT32_C(225)) {
        *mapped_motion = motion;
        return true;
    }

    target_1f8 = bits_to_float(target_1f8_bits);
    if (!isfinite(target_1f8)) {
        return false;
    }

    *mapped_motion = target_1f8 <= 0.9f ? UINT32_C(0x106) : motion;
    return true;
}

bool stf_hit_motion_select_from_row(
    const stf_hit_motion_selector_inputs *inputs,
    const uint32_t *motion_row,
    size_t motion_row_count,
    stf_hit_motion_selector_result *result
)
{
    uint64_t index = 0u;
    uint32_t raw_motion = 0u;
    uint32_t motion = 0u;

    if (inputs == NULL || motion_row == NULL || result == NULL) {
        return false;
    }

    if (inputs->slot == UINT32_C(5)) {
        index = UINT64_C(40);
    } else {
        uint32_t parity = 0u;
        int32_t side = 0;

        parity =
            ((uint32_t)inputs->selector_word >> 15u) ^
            ((inputs->source_flags_0 ^ inputs->target_flags_0) >> 6u) ^
            (inputs->target_flags_1a4 >> 21u);
        parity &= UINT32_C(1);

        index = (uint64_t)inputs->slot * UINT64_C(8) + parity;

        side =
            (int32_t)inputs->source_82a +
            (int32_t)inputs->source_26 -
            (int32_t)inputs->target_5b4 +
            INT32_C(0x4000);
        if (((uint32_t)side & UINT32_C(0x8000)) == 0u) {
            index += UINT64_C(2);
        }
    }

    if (index >= motion_row_count) {
        return false;
    }

    raw_motion = motion_row[(size_t)index];
    if (!remap_down_motion(raw_motion, inputs->target_1f8_bits, &motion)) {
        return false;
    }

    result->table_index = (size_t)index;
    result->raw_motion = raw_motion;
    result->motion = motion;
    return true;
}
