#include "damage_unit.h"

#include <string.h>

static const uint8_t kind_categories[STF_DAMAGE_UNIT_KIND_CATEGORY_COUNT] = {
    UINT8_C(0), UINT8_C(0), UINT8_C(1), UINT8_C(0),
    UINT8_C(1), UINT8_C(1), UINT8_C(1), UINT8_C(0),
    UINT8_C(1), UINT8_C(0), UINT8_C(0), UINT8_C(0),
};

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

static int highest_set_bit(uint16_t value)
{
    int bit = 15;
    for (bit = 15; bit >= 0; --bit) {
        if ((value & (uint16_t)(UINT16_C(1) << (unsigned)bit)) != 0u) {
            return bit;
        }
    }
    return -1;
}

const uint8_t *stf_damage_unit_kind_categories(size_t *count)
{
    if (count != NULL) {
        *count = STF_DAMAGE_UNIT_KIND_CATEGORY_COUNT;
    }
    return kind_categories;
}

bool stf_damage_unit_apply_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    const uint8_t *workspace,
    size_t workspace_size,
    uint8_t num_rounds_to_win,
    stf_damage_unit_effect_state *effect,
    stf_damage_unit_result *result
)
{
    stf_damage_unit_result local;
    uint16_t pending = 0u;
    uint8_t attacker_kind = 0u;
    uint8_t raw_strength = 0u;
    uint32_t defender_flags = 0u;
    uint32_t sum_low = 0u;
    uint32_t sum_high = 0u;
    uint32_t flags_7f0 = 0u;
    int slot = -1;
    unsigned index = 0u;

    if (attacker == NULL || defender == NULL || workspace == NULL ||
        effect == NULL || result == NULL ||
        attacker_size < STF_DAMAGE_UNIT_ATTACKER_MIN_SIZE ||
        defender_size < STF_DAMAGE_UNIT_DEFENDER_MIN_SIZE ||
        workspace_size < STF_DAMAGE_UNIT_WORKSPACE_MIN_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));

    if ((read_le32(workspace + 0x26Cu) & UINT32_C(1)) == 0u) {
        local.skipped = true;
        *result = local;
        return true;
    }

    attacker_kind = attacker[0x821u];
    if (attacker_kind >= STF_DAMAGE_UNIT_KIND_CATEGORY_COUNT) {
        return false;
    }

    raw_strength = attacker[0x822u];
    defender_flags = read_le32(defender + 0x1A4u);
    pending = read_le16(defender + 0x6F0u);

    while (pending != 0u) {
        uint8_t category = 0u;
        uint16_t before = 0u;
        uint16_t after = 0u;

        slot = highest_set_bit(pending);
        if (slot < 0) {
            return false;
        }

        category = slot >= 9 ? UINT8_C(1) : UINT8_C(0);
        if (kind_categories[attacker_kind] != category) {
            pending = (uint16_t)(
                pending & (uint16_t)~(uint16_t)(UINT16_C(1) << (unsigned)slot)
            );
            continue;
        }

        before = read_le16(defender + 0x1F00u + (size_t)slot * 2u);
        after = (uint16_t)(before + raw_strength);
        write_le16(
            defender + 0x1F00u + (size_t)slot * 2u,
            after
        );

        if ((defender_flags & (UINT32_C(1) << 26u)) != 0u) {
            after = (uint16_t)(after + raw_strength);
            write_le16(
                defender + 0x1F00u + (size_t)slot * 2u,
                after
            );
        } else if ((defender_flags & (UINT32_C(1) << 13u)) != 0u) {
            after = (uint16_t)(after + (uint16_t)(raw_strength >> 1u));
            write_le16(
                defender + 0x1F00u + (size_t)slot * 2u,
                after
            );
        }

        local.matched_slot = true;
        local.selected_slot = (uint8_t)slot;
        local.selected_category = category;
        local.accumulator_before = before;
        local.accumulator_after = after;
        break;
    }

    if (num_rounds_to_win != 0u &&
        attacker[0x1F5Cu] == (uint8_t)(num_rounds_to_win - UINT8_C(1)) &&
        effect->active_914 == 0u &&
        (attacker[0x7D2u] & UINT8_C(1)) != 0u) {
        effect->active_914 = UINT32_C(1);
        local.final_round_override = true;
        for (index = 0u; index < 16u; ++index) {
            write_le16(
                defender + 0x1F00u + index * 2u,
                UINT16_C(0x1000)
            );
        }
    }

    write_le32(defender + 0x1F7Cu, read_le32(defender + 0x1F74u));
    write_le32(defender + 0x1F80u, read_le32(defender + 0x1F78u));

    for (index = 0u; index < 16u; ++index) {
        const uint32_t value =
            (uint32_t)read_le16(defender + 0x1F00u + index * 2u);
        if (index < 9u) {
            sum_low += value;
        } else {
            sum_high += value;
        }
    }

    write_le32(defender + 0x1F74u, sum_low);
    write_le32(defender + 0x1F78u, sum_high);

    flags_7f0 = read_le32(defender + 0x7F0u);
    if ((flags_7f0 & (UINT32_C(1) << 2u)) == 0u &&
        sum_low >= read_le32(defender + 0xAF0u)) {
        flags_7f0 |= UINT32_C(1);
    }
    if ((flags_7f0 & (UINT32_C(1) << 3u)) == 0u &&
        sum_high >= read_le32(defender + 0xAF4u)) {
        flags_7f0 |= UINT32_C(1) << 1u;
    }
    write_le32(defender + 0x7F0u, flags_7f0);

    local.up_total_1f74 = sum_low;
    local.down_total_1f78 = sum_high;
    local.flags_7f0 = flags_7f0;

    *result = local;
    return true;
}
