#include "collision_attack.h"

#include <string.h>

static bool bit_is_set32(uint32_t value, unsigned bit)
{
    return bit < 32u && (value & (UINT32_C(1) << bit)) != 0u;
}

static bool bit_is_set16(uint16_t value, unsigned bit)
{
    return bit < 16u && (value & (uint16_t)(UINT16_C(1) << bit)) != 0u;
}

static unsigned highest_set_bit16(uint16_t value)
{
    unsigned bit = 0u;

    for (bit = 16u; bit > 0u; --bit) {
        if (bit_is_set16(value, bit - 1u)) {
            return bit - 1u;
        }
    }
    return 0u;
}

static uint16_t resolve_overlap(
    uint8_t fighter_index,
    uint32_t attack_profile_bits,
    const uint16_t mapping[STF_COLLISION_ATTACK_MAPPING_COUNT]
)
{
    uint16_t overlap = 0u;
    unsigned profile_bit = 0u;

    if (fighter_index == 0u) {
        for (profile_bit = 0u; profile_bit < 32u; ++profile_bit) {
            if (bit_is_set32(attack_profile_bits, profile_bit) &&
                profile_bit < STF_COLLISION_ATTACK_MAPPING_COUNT) {
                overlap = (uint16_t)(overlap | mapping[profile_bit]);
            }
        }
        return overlap;
    }

    for (profile_bit = 0u; profile_bit < 32u; ++profile_bit) {
        unsigned unit = 0u;

        if (!bit_is_set32(attack_profile_bits, profile_bit) ||
            profile_bit >= 16u) {
            continue;
        }

        for (unit = 0u; unit < STF_COLLISION_ATTACK_MAPPING_COUNT; ++unit) {
            if (bit_is_set16(mapping[unit], profile_bit)) {
                overlap = (uint16_t)(
                    overlap | (uint16_t)(UINT16_C(1) << unit)
                );
            }
        }
    }

    return overlap;
}

bool stf_collision_attack_resolve(
    const stf_collision_attack_inputs *inputs,
    const uint16_t mapping[STF_COLLISION_ATTACK_MAPPING_COUNT],
    stf_collision_attack_result *result
)
{
    uint16_t overlap = 0u;
    const unsigned fighter_bit =
        inputs != NULL ? (unsigned)(inputs->fighter_index & UINT8_C(0x0F)) : 0u;

    if (inputs == NULL || mapping == NULL || result == NULL ||
        inputs->fighter_index > 1u) {
        return false;
    }

    memset(result, 0, sizeof(*result));
    result->next_previous_motion = inputs->previous_motion;
    result->next_hit_history_090 = inputs->hit_history_090;
    result->next_lockout_2ac = inputs->lockout_2ac;

    if (bit_is_set32(inputs->flags_720, 15u)) {
        return true;
    }

    result->previous_motion_written = true;
    result->next_previous_motion = inputs->current_motion;

    if (!bit_is_set32(inputs->flags_1a4, 8u)) {
        result->next_hit_history_090 = (uint16_t)(
            result->next_hit_history_090 &
            (uint16_t)~(uint16_t)(UINT16_C(1) << fighter_bit)
        );
        return true;
    }

    if (!bit_is_set32(inputs->flags_860, 22u)) {
        if (inputs->lockout_2ac != 0u) {
            result->next_hit_history_090 = (uint16_t)(
                result->next_hit_history_090 &
                (uint16_t)~(uint16_t)(UINT16_C(1) << fighter_bit)
            );
            return true;
        }
    } else {
        if (inputs->previous_motion != inputs->current_motion) {
            result->next_hit_history_090 = (uint16_t)(
                result->next_hit_history_090 &
                (uint16_t)~(uint16_t)(UINT16_C(1) << fighter_bit)
            );
            return true;
        }

        if (bit_is_set16(result->next_hit_history_090, fighter_bit)) {
            return true;
        }
    }

    if (inputs->field_1aa < inputs->field_808) {
        return true;
    }

    overlap = resolve_overlap(
        inputs->fighter_index,
        inputs->attack_profile_bits,
        mapping
    );
    overlap = (uint16_t)(overlap & (uint16_t)~inputs->opponent_suppression_6f8);
    result->overlap_mask = overlap;

    if (overlap == 0u) {
        return true;
    }

    result->next_hit_history_090 = (uint16_t)(
        result->next_hit_history_090 |
        (uint16_t)(UINT16_C(1) << fighter_bit)
    );
    result->selected_unit = highest_set_bit16(overlap);
    result->hit_latch = UINT16_C(1);
    result->next_lockout_2ac = UINT32_C(8);
    result->hit = true;
    return true;
}
