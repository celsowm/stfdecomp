#include <stdint.h>
#include <string.h>

#include "kabe_parts_damage.h"

static uint16_t read_le16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8u));
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

static int test_nominal(void)
{
    uint8_t fighter[STF_DAMAGE_UNIT_DEFENDER_MIN_SIZE];
    stf_kabe_parts_damage_result result;
    unsigned index = 0u;

    memset(fighter, 0, sizeof(fighter));
    for (index = 0u; index < 16u; ++index) {
        write_le16(
            fighter + 0x1F00u + index * 2u,
            (uint16_t)index
        );
    }

    write_le32(fighter + 0xAF0u, UINT32_C(30));
    write_le32(fighter + 0xAF4u, UINT32_C(60));

    if (!stf_kabe_parts_damage_apply_model2(
            fighter, sizeof(fighter), UINT32_C(20), &result
        ) ||
        result.per_part_damage != UINT16_C(5) ||
        read_le16(fighter + 0x1F00u) != UINT16_C(5) ||
        read_le16(fighter + 0x1F1Eu) != UINT16_C(20) ||
        result.totals.up_total_1f74 != UINT32_C(81) ||
        result.totals.down_total_1f78 != UINT32_C(119) ||
        (result.totals.flags_7f0 & UINT32_C(1)) == 0u ||
        (result.totals.flags_7f0 & (UINT32_C(1) << 1u)) == 0u) {
        return 1;
    }

    return 0;
}

static int test_damage_division_truncates(void)
{
    uint8_t fighter[STF_DAMAGE_UNIT_DEFENDER_MIN_SIZE];
    stf_kabe_parts_damage_result result;
    unsigned index = 0u;

    memset(fighter, 0, sizeof(fighter));
    for (index = 0u; index < 16u; ++index) {
        write_le16(
            fighter + 0x1F00u + index * 2u,
            UINT16_C(10)
        );
    }

    write_le32(fighter + 0xAF0u, UINT32_MAX);
    write_le32(fighter + 0xAF4u, UINT32_MAX);

    if (!stf_kabe_parts_damage_apply_model2(
            fighter, sizeof(fighter), UINT32_C(23), &result
        ) ||
        result.per_part_damage != UINT16_C(5) ||
        read_le16(fighter + 0x1F00u) != UINT16_C(15) ||
        read_le16(fighter + 0x1F1Eu) != UINT16_C(15) ||
        result.totals.up_total_1f74 != UINT32_C(135) ||
        result.totals.down_total_1f78 != UINT32_C(105)) {
        return 1;
    }

    return 0;
}

static int test_accumulator_wraps_at_16_bits(void)
{
    uint8_t fighter[STF_DAMAGE_UNIT_DEFENDER_MIN_SIZE];
    stf_kabe_parts_damage_result result;
    unsigned index = 0u;

    memset(fighter, 0, sizeof(fighter));
    for (index = 0u; index < 16u; ++index) {
        write_le16(
            fighter + 0x1F00u + index * 2u,
            UINT16_C(0xFFFE)
        );
    }

    write_le32(fighter + 0xAF0u, UINT32_MAX);
    write_le32(fighter + 0xAF4u, UINT32_MAX);

    if (!stf_kabe_parts_damage_apply_model2(
            fighter, sizeof(fighter), UINT32_C(8), &result
        ) ||
        result.per_part_damage != UINT16_C(2) ||
        read_le16(fighter + 0x1F00u) != UINT16_C(0) ||
        read_le16(fighter + 0x1F1Eu) != UINT16_C(0) ||
        result.totals.up_total_1f74 != UINT32_C(0) ||
        result.totals.down_total_1f78 != UINT32_C(0)) {
        return 1;
    }

    return 0;
}

int main(void)
{
    if (test_nominal() != 0) {
        return 1;
    }
    if (test_damage_division_truncates() != 0) {
        return 1;
    }
    if (test_accumulator_wraps_at_16_bits() != 0) {
        return 1;
    }
    return 0;
}
