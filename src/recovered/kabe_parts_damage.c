#include "kabe_parts_damage.h"

#include <string.h>

static uint16_t read_le16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8u));
}

static void write_le16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
}

bool stf_kabe_parts_damage_apply_model2(
    uint8_t *fighter,
    size_t fighter_size,
    uint32_t damage,
    stf_kabe_parts_damage_result *result
)
{
    stf_kabe_parts_damage_result local;
    uint16_t per_part = 0u;
    unsigned index = 0u;

    if (fighter == NULL || result == NULL ||
        fighter_size < STF_DAMAGE_UNIT_DEFENDER_MIN_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    per_part = (uint16_t)(damage / UINT32_C(4));
    local.per_part_damage = per_part;

    for (index = 0u; index < 16u; ++index) {
        const size_t offset = 0x1F00u + index * 2u;
        const uint16_t value = read_le16(fighter + offset);
        write_le16(fighter + offset, (uint16_t)(value + per_part));
    }

    if (!stf_calc_up_down_damage_apply_model2(
            fighter, fighter_size, &local.totals
        )) {
        return false;
    }

    *result = local;
    return true;
}
