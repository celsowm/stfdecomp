#include <stdint.h>
#include <string.h>

#include "collision_attack.h"

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


static void reset(
    stf_collision_attack_inputs *inputs,
    uint16_t mapping[STF_COLLISION_ATTACK_MAPPING_COUNT]
)
{
    memset(inputs, 0, sizeof(*inputs));
    memset(mapping, 0, sizeof(uint16_t) * STF_COLLISION_ATTACK_MAPPING_COUNT);
    inputs->flags_1a4 = UINT32_C(1) << 8u;
    inputs->field_1aa = UINT16_C(10);
    inputs->field_808 = UINT16_C(5);
}

int main(void)
{
    stf_collision_attack_inputs inputs;
    stf_collision_attack_result result;
    uint16_t mapping[STF_COLLISION_ATTACK_MAPPING_COUNT];

    reset(&inputs, mapping);

    /* Fighter 0: OR selected mapping entries. */
    inputs.fighter_index = 0u;
    inputs.attack_profile_bits = (UINT32_C(1) << 1u) | (UINT32_C(1) << 3u);
    mapping[1] = UINT16_C(0x0003);
    mapping[3] = UINT16_C(0x0024);
    if (!stf_collision_attack_resolve(&inputs, mapping, &result) ||
        !result.hit ||
        result.overlap_mask != UINT16_C(0x0027) ||
        result.selected_unit != 5u ||
        result.next_hit_history_090 != UINT16_C(1) ||
        result.hit_latch != UINT16_C(1) ||
        result.next_lockout_2ac != UINT32_C(8)) {
        return 1;
    }

    /* Suppression removes units before choosing the highest surviving unit. */
    inputs.opponent_suppression_6f8 = UINT16_C(0x0020);
    if (!stf_collision_attack_resolve(&inputs, mapping, &result) ||
        !result.hit ||
        result.overlap_mask != UINT16_C(0x0007) ||
        result.selected_unit != 2u) {
        return 2;
    }

    /* Fighter 1 transposes the mapping relation. */
    reset(&inputs, mapping);
    inputs.fighter_index = 1u;
    inputs.attack_profile_bits = UINT32_C(1) << 2u;
    mapping[0] = UINT16_C(1) << 2u;
    mapping[4] = UINT16_C(1) << 2u;
    mapping[7] = UINT16_C(1) << 1u;
    if (!stf_collision_attack_resolve(&inputs, mapping, &result) ||
        !result.hit ||
        result.overlap_mask != UINT16_C(0x0011) ||
        result.selected_unit != 4u ||
        result.next_hit_history_090 != UINT16_C(2)) {
        return 3;
    }

    /* Disabled attack updates the motion and clears this fighter's history. */
    reset(&inputs, mapping);
    inputs.fighter_index = 1u;
    inputs.flags_1a4 = 0u;
    inputs.previous_motion = UINT16_C(4);
    inputs.current_motion = UINT16_C(7);
    inputs.hit_history_090 = UINT16_C(3);
    if (!stf_collision_attack_resolve(&inputs, mapping, &result) ||
        result.hit ||
        !result.previous_motion_written ||
        result.next_previous_motion != UINT16_C(7) ||
        result.next_hit_history_090 != UINT16_C(1)) {
        return 4;
    }

    /* flags_720 bit 15 exits before writing previous motion. */
    reset(&inputs, mapping);
    inputs.flags_720 = UINT32_C(1) << 15u;
    inputs.previous_motion = UINT16_C(9);
    inputs.current_motion = UINT16_C(10);
    if (!stf_collision_attack_resolve(&inputs, mapping, &result) ||
        result.previous_motion_written ||
        result.next_previous_motion != UINT16_C(9) ||
        result.hit) {
        return 5;
    }

    /* Lockout rejects when flags_860 bit 22 is clear and clears history. */
    reset(&inputs, mapping);
    inputs.hit_history_090 = UINT16_C(1);
    inputs.lockout_2ac = UINT32_C(3);
    if (!stf_collision_attack_resolve(&inputs, mapping, &result) ||
        result.next_hit_history_090 != 0u ||
        result.hit) {
        return 6;
    }

    /* With bit 22 set, motion changes clear history and reject. */
    reset(&inputs, mapping);
    inputs.flags_860 = UINT32_C(1) << 22u;
    inputs.previous_motion = UINT16_C(2);
    inputs.current_motion = UINT16_C(3);
    inputs.hit_history_090 = UINT16_C(1);
    if (!stf_collision_attack_resolve(&inputs, mapping, &result) ||
        result.next_hit_history_090 != 0u ||
        result.hit) {
        return 7;
    }

    /* Same motion plus already-hit bit rejects without clearing it. */
    reset(&inputs, mapping);
    inputs.flags_860 = UINT32_C(1) << 22u;
    inputs.previous_motion = UINT16_C(3);
    inputs.current_motion = UINT16_C(3);
    inputs.hit_history_090 = UINT16_C(1);
    if (!stf_collision_attack_resolve(&inputs, mapping, &result) ||
        result.next_hit_history_090 != UINT16_C(1) ||
        result.hit) {
        return 8;
    }

    /* Timing gate rejects before overlap resolution. */
    reset(&inputs, mapping);
    inputs.field_1aa = UINT16_C(4);
    inputs.field_808 = UINT16_C(5);
    mapping[0] = UINT16_MAX;
    inputs.attack_profile_bits = UINT32_C(1);
    if (!stf_collision_attack_resolve(&inputs, mapping, &result) ||
        result.overlap_mask != 0u ||
        result.hit) {
        return 9;
    }

    if (stf_collision_attack_resolve(NULL, mapping, &result) ||
        stf_collision_attack_resolve(&inputs, NULL, &result) ||
        stf_collision_attack_resolve(&inputs, mapping, NULL)) {
        return 10;
    }

    inputs.fighter_index = 2u;
    if (stf_collision_attack_resolve(&inputs, mapping, &result)) {
        return 11;
    }

    {
        uint8_t fighter[STF_COLLISION_ATTACK_FIGHTER_MODEL2_MIN_SIZE];
        uint8_t opponent[STF_COLLISION_ATTACK_OPPONENT_MODEL2_MIN_SIZE];
        uint8_t workspace[STF_COLLISION_ATTACK_WORKSPACE_MODEL2_MIN_SIZE];

        memset(fighter, 0, sizeof(fighter));
        memset(opponent, 0, sizeof(opponent));
        memset(workspace, 0, sizeof(workspace));
        memset(mapping, 0, sizeof(mapping));

        fighter[4u] = 1u;
        write_le16(fighter + 0x1A8u, UINT16_C(12));
        write_le32(fighter + 0x1A4u, UINT32_C(1) << 8u);
        write_le16(fighter + 0x1AAu, UINT16_C(10));
        write_le16(fighter + 0x808u, UINT16_C(5));
        write_le16(workspace + 0x8Eu, UINT16_C(12));
        mapping[3] = UINT16_C(1) << 2u;
        mapping[6] = UINT16_C(1) << 2u;

        if (!stf_collision_attack_apply_model2(
                fighter,
                sizeof(fighter),
                opponent,
                sizeof(opponent),
                workspace,
                sizeof(workspace),
                UINT32_C(1) << 2u,
                mapping,
                &result
            )) {
            return 12;
        }

        if (!result.hit ||
            read_le16(opponent + 0x6F0u) != UINT16_C(0x0048) ||
            read_le32(opponent + 0x7E0u) != UINT32_C(6) ||
            read_le16(workspace + 0x90u) != UINT16_C(2) ||
            read_le16(workspace + 0x276u) != UINT16_C(1) ||
            read_le32(workspace + 0x2ACu) != UINT32_C(8)) {
            return 13;
        }

        /* A gated miss still clears the per-fighter hit latch. */
        write_le32(fighter + 0x720u, UINT32_C(1) << 15u);
        write_le16(workspace + 0x276u, UINT16_C(0xFFFF));
        if (!stf_collision_attack_apply_model2(
                fighter,
                sizeof(fighter),
                opponent,
                sizeof(opponent),
                workspace,
                sizeof(workspace),
                UINT32_C(1) << 2u,
                mapping,
                &result
            ) ||
            read_le16(workspace + 0x276u) != 0u) {
            return 14;
        }
    }

    return 0;
}
