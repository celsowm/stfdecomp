#include "collision_finish.h"

#include <string.h>

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8u) |
           ((uint32_t)data[2] << 16u) |
           ((uint32_t)data[3] << 24u);
}

static float bits_to_float(uint32_t bits)
{
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

uint8_t stf_collision_finish_next_phase(
    const stf_collision_finish_inputs *inputs
)
{
    uint8_t phase = 0u;

    if (inputs == NULL) {
        return 0u;
    }

    phase = inputs->phase_1a18;

    switch (phase) {
    case 0u:
        if ((inputs->opponent_field_7d2 & UINT8_C(1)) == 0u) {
            return phase;
        }
        phase = 1u;
        /* fall through */
    case 1u:
        if (8.5f > bits_to_float(inputs->field_710_bits)) {
            return phase;
        }
        phase = 2u;
        /* fall through */
    case 2u:
        if (UINT8_C(0x0E) > inputs->field_a29) {
            return phase;
        }
        return 3u;
    default:
        return phase;
    }
}

bool stf_collision_finish_apply_model2(
    uint8_t *fighter,
    size_t fighter_size,
    const uint8_t *opponent,
    size_t opponent_size
)
{
    stf_collision_finish_inputs inputs;

    if (fighter == NULL ||
        opponent == NULL ||
        fighter_size < STF_COLLISION_FINISH_FIGHTER_MODEL2_MIN_SIZE ||
        opponent_size < STF_COLLISION_FINISH_OPPONENT_MODEL2_MIN_SIZE) {
        return false;
    }

    inputs.phase_1a18 = fighter[0x1A18u];
    inputs.field_710_bits = read_le32(fighter + 0x710u);
    inputs.opponent_field_7d2 = opponent[0x7D2u];
    inputs.field_a29 = fighter[0xA29u];

    fighter[0x1A18u] = stf_collision_finish_next_phase(&inputs);
    return true;
}
