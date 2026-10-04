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

int main(void)
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
        result.totals.down_total_1f78 != UINT32_C(112) ||
        (result.totals.flags_7f0 & UINT32_C(1)) == 0u ||
        (result.totals.flags_7f0 & (UINT32_C(1) << 1u)) == 0u) {
        return 1;
    }

    return 0;
}
