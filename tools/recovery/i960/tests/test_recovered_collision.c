#include <stdint.h>
#include <string.h>

#include "collision_finish.h"

static void write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

int main(void)
{
    uint8_t fighter[STF_COLLISION_FINISH_FIGHTER_MODEL2_MIN_SIZE];
    uint8_t opponent[STF_COLLISION_FINISH_OPPONENT_MODEL2_MIN_SIZE];
    stf_collision_finish_inputs inputs;

    memset(fighter, 0xAA, sizeof(fighter));
    memset(opponent, 0x55, sizeof(opponent));

    fighter[0x1A18u] = 0u;
    fighter[0xA29u] = 0u;
    write_le32(fighter + 0x710u, UINT32_C(0x41000000)); /* 8.0f */
    opponent[0x7D2u] = 0u;
    if (!stf_collision_finish_apply_model2(
            fighter,
            sizeof(fighter),
            opponent,
            sizeof(opponent)
        ) ||
        fighter[0x1A18u] != 0u) {
        return 1;
    }

    opponent[0x7D2u] = 1u;
    if (!stf_collision_finish_apply_model2(
            fighter,
            sizeof(fighter),
            opponent,
            sizeof(opponent)
        ) ||
        fighter[0x1A18u] != 1u) {
        return 2;
    }

    write_le32(fighter + 0x710u, UINT32_C(0x41080000)); /* 8.5f */
    fighter[0xA29u] = 13u;
    if (!stf_collision_finish_apply_model2(
            fighter,
            sizeof(fighter),
            opponent,
            sizeof(opponent)
        ) ||
        fighter[0x1A18u] != 2u) {
        return 3;
    }

    fighter[0xA29u] = 14u;
    if (!stf_collision_finish_apply_model2(
            fighter,
            sizeof(fighter),
            opponent,
            sizeof(opponent)
        ) ||
        fighter[0x1A18u] != 3u) {
        return 4;
    }

    fighter[0x1A18u] = 1u;
    write_le32(fighter + 0x710u, UINT32_C(0x41100000)); /* 9.0f */
    fighter[0xA29u] = 14u;
    if (!stf_collision_finish_apply_model2(
            fighter,
            sizeof(fighter),
            opponent,
            sizeof(opponent)
        ) ||
        fighter[0x1A18u] != 3u) {
        return 5;
    }

    fighter[0x1A18u] = 7u;
    if (!stf_collision_finish_apply_model2(
            fighter,
            sizeof(fighter),
            opponent,
            sizeof(opponent)
        ) ||
        fighter[0x1A18u] != 7u) {
        return 6;
    }

    memset(&inputs, 0, sizeof(inputs));
    inputs.phase_1a18 = 0u;
    inputs.opponent_field_7d2 = 1u;
    inputs.field_710_bits = UINT32_C(0x41100000);
    inputs.field_a29 = 14u;
    if (stf_collision_finish_next_phase(&inputs) != 3u) {
        return 7;
    }

    if (stf_collision_finish_apply_model2(
            fighter,
            STF_COLLISION_FINISH_FIGHTER_MODEL2_MIN_SIZE - 1u,
            opponent,
            sizeof(opponent)
        )) {
        return 8;
    }

    if (stf_collision_finish_apply_model2(
            fighter,
            sizeof(fighter),
            opponent,
            STF_COLLISION_FINISH_OPPONENT_MODEL2_MIN_SIZE - 1u
        )) {
        return 9;
    }

    fighter[0x100u] = 0x33u;
    fighter[0x1A17u] = 0x44u;
    fighter[0x1A18u] = 2u;
    fighter[0xA29u] = 14u;
    if (!stf_collision_finish_apply_model2(
            fighter,
            sizeof(fighter),
            opponent,
            sizeof(opponent)
        ) ||
        fighter[0x100u] != 0x33u ||
        fighter[0x1A17u] != 0x44u) {
        return 10;
    }

    return 0;
}
