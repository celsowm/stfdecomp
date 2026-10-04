#include "collision_no_unit.h"

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

static float bits_to_float(uint32_t bits)
{
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static uint32_t clear_sign_bit(uint32_t bits)
{
    return bits & UINT32_C(0x7FFFFFFF);
}

static bool bit_is_set(uint32_t value, unsigned bit)
{
    return (value & (UINT32_C(1) << bit)) != 0u;
}

bool stf_collision_no_unit_mask_model2(
    const uint8_t *fighter,
    size_t fighter_size,
    const uint8_t *opponent,
    size_t opponent_size,
    uint32_t stage_floor_bits,
    uint32_t stage_extent_bits,
    uint32_t no_coli_low_bits,
    uint16_t *mask
)
{
    uint32_t self_flags = 0u;
    uint32_t other_flags = 0u;
    uint8_t other_kind = 0u;
    uint16_t result = 0u;

    if (fighter == NULL || opponent == NULL || mask == NULL ||
        fighter_size < STF_COLLISION_NO_UNIT_SELF_MIN_SIZE ||
        opponent_size < STF_COLLISION_NO_UNIT_OTHER_MIN_SIZE) {
        return false;
    }

    self_flags = read_le32(fighter + 0x1A4u);
    other_flags = read_le32(opponent + 0x1A4u);

    if (bit_is_set(self_flags, 8u) &&
        bit_is_set(other_flags, 8u) &&
        fighter[0xA29u] >= UINT8_C(0x1E) &&
        opponent[0xA29u] <= UINT8_C(5)) {
        *mask = UINT16_MAX;
        return true;
    }

    other_kind = opponent[0x821u];

    if (!bit_is_set(other_flags, 8u) || bit_is_set(other_flags, 1u)) {
        *mask = 0u;
        return true;
    }

    if (bit_is_set(self_flags, 15u) && other_kind != UINT8_C(4)) {
        *mask = UINT16_MAX;
        return true;
    }

    if (bit_is_set(self_flags, 16u) &&
        other_kind != UINT8_C(4) &&
        other_kind != UINT8_C(8)) {
        *mask = UINT16_MAX;
        return true;
    }

    if (bit_is_set(self_flags, 30u)) {
        *mask = UINT16_MAX;
        return true;
    }

    if (bit_is_set(self_flags, 14u) &&
        read_le16(fighter + 0x61Cu) != 0u &&
        other_kind != UINT8_C(4) &&
        other_kind != UINT8_C(8) &&
        other_kind != UINT8_C(5)) {
        uint32_t effective_floor_bits = stage_floor_bits;
        const float stage_extent = bits_to_float(stage_extent_bits);
        const float abs_x =
            bits_to_float(clear_sign_bit(read_le32(fighter + 0x1F4u)));
        const float abs_z =
            bits_to_float(clear_sign_bit(read_le32(fighter + 0x1FCu)));
        float threshold = 0.0f;
        size_t index = 0u;

        if (abs_x < stage_extent && abs_z < stage_extent) {
            effective_floor_bits = 0u;
        }

        threshold =
            bits_to_float(effective_floor_bits) +
            bits_to_float(no_coli_low_bits);

        for (index = 0u; index < 16u; ++index) {
            const float value =
                bits_to_float(read_le32(fighter + 0x1F8u + index * 0xCu));
            if (value >= threshold) {
                result = (uint16_t)(result | (uint16_t)(UINT16_C(1) << index));
            }
        }

        *mask = result;
        return true;
    }

    if (other_kind == UINT8_C(0) && bit_is_set(self_flags, 3u)) {
        *mask = UINT16_MAX;
        return true;
    }

    if (read_le16(fighter + 0x61Cu) != 0u ||
        bit_is_set(self_flags, 14u) ||
        !bit_is_set(self_flags, 4u) ||
        other_kind == UINT8_C(0) ||
        other_kind == UINT8_C(7)) {
        *mask = 0u;
        return true;
    }

    if (bits_to_float(read_le32(fighter + 0x1F8u)) >=
        bits_to_float(read_le32(fighter + 0x1E10u))) {
        *mask = UINT16_MAX;
        return true;
    }

    *mask = 0u;
    return true;
}

bool stf_collision_no_unit_apply_model2(
    uint8_t *fighter,
    size_t fighter_size,
    const uint8_t *opponent,
    size_t opponent_size,
    uint32_t stage_floor_bits,
    uint32_t stage_extent_bits,
    uint32_t no_coli_low_bits
)
{
    uint16_t mask = 0u;

    if (!stf_collision_no_unit_mask_model2(
            fighter,
            fighter_size,
            opponent,
            opponent_size,
            stage_floor_bits,
            stage_extent_bits,
            no_coli_low_bits,
            &mask
        )) {
        return false;
    }

    write_le16(fighter + 0x6F8u, mask);
    return true;
}
