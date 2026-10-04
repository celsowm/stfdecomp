#include "collision_attack.h"

#include <string.h>

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

static void write_le16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
}

static void write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

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
    result->overlap_evaluated = true;

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


bool stf_collision_attack_apply_model2(
    uint8_t *fighter,
    size_t fighter_size,
    uint8_t *opponent,
    size_t opponent_size,
    uint8_t *workspace,
    size_t workspace_size,
    uint32_t attack_profile_bits,
    const uint16_t mapping[STF_COLLISION_ATTACK_MAPPING_COUNT],
    stf_collision_attack_result *result
)
{
    stf_collision_attack_inputs inputs;
    stf_collision_attack_result local_result;
    uint8_t fighter_index = 0u;

    if (fighter == NULL || opponent == NULL || workspace == NULL ||
        mapping == NULL ||
        fighter_size < STF_COLLISION_ATTACK_FIGHTER_MODEL2_MIN_SIZE ||
        opponent_size < STF_COLLISION_ATTACK_OPPONENT_MODEL2_MIN_SIZE ||
        workspace_size < STF_COLLISION_ATTACK_WORKSPACE_MODEL2_MIN_SIZE) {
        return false;
    }

    fighter_index = fighter[4u];
    if (fighter_index > 1u) {
        return false;
    }

    memset(&inputs, 0, sizeof(inputs));
    inputs.fighter_index = fighter_index;
    inputs.previous_motion =
        read_le16(workspace + 0x8Cu + (size_t)fighter_index * 2u);
    inputs.current_motion = read_le16(fighter + 0x1A8u);
    inputs.flags_1a4 = read_le32(fighter + 0x1A4u);
    inputs.flags_720 = read_le32(fighter + 0x720u);
    inputs.flags_860 = read_le32(fighter + 0x860u);
    inputs.field_1aa = read_le16(fighter + 0x1AAu);
    inputs.field_808 = read_le16(fighter + 0x808u);
    inputs.hit_history_090 = read_le16(workspace + 0x90u);
    inputs.lockout_2ac = read_le32(workspace + 0x2ACu);
    inputs.attack_profile_bits = attack_profile_bits;
    inputs.opponent_suppression_6f8 = read_le16(opponent + 0x6F8u);

    if (!stf_collision_attack_resolve(&inputs, mapping, &local_result)) {
        return false;
    }

    if (local_result.previous_motion_written) {
        write_le16(
            workspace + 0x8Cu + (size_t)fighter_index * 2u,
            local_result.next_previous_motion
        );
    }

    write_le16(workspace + 0x90u, local_result.next_hit_history_090);
    write_le16(
        workspace + 0x274u + (size_t)fighter_index * 2u,
        local_result.hit_latch
    );

    if (local_result.overlap_evaluated) {
        write_le16(opponent + 0x6F0u, local_result.overlap_mask);
    }

    if (local_result.hit) {
        write_le32(opponent + 0x7E0u, local_result.selected_unit);
        write_le32(workspace + 0x2ACu, local_result.next_lockout_2ac);
    }

    if (result != NULL) {
        *result = local_result;
    }
    return true;
}
