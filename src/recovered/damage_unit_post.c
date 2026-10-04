#include "damage_unit_post.h"

#include <string.h>

enum {
    STF_DAMAGE_UNIT_POST_DEFENDER_MIN_SIZE = 0x7F4u
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

bool stf_damage_unit_post_apply_model2(
    uint8_t *defender,
    size_t defender_size,
    const uint8_t *workspace,
    size_t workspace_size,
    stf_damage_unit_effect_state *effect,
    uint8_t also_mode,
    uint8_t also_sub_mode,
    stf_damage_unit_post_result *result
)
{
    stf_damage_unit_post_result local;
    uint32_t flags0 = 0u;
    uint32_t flags7f0 = 0u;
    uint32_t workspace_flags = 0u;
    uint16_t effect_flags = 0u;

    if (defender == NULL || workspace == NULL || effect == NULL ||
        result == NULL ||
        defender_size < STF_DAMAGE_UNIT_POST_DEFENDER_MIN_SIZE ||
        workspace_size < STF_DAMAGE_UNIT_WORKSPACE_MIN_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    local.crush_table_inert = true;

    flags0 = read_le32(defender + 0x000u);
    flags7f0 = read_le32(defender + 0x7F0u);
    workspace_flags = read_le32(workspace + 0x26Cu);

    if (effect->active_914 != 0u) {
        flags7f0 |= (UINT32_C(1) << 2u) | (UINT32_C(1) << 3u);
        flags0 |= UINT32_C(1) << 28u;
        write_le32(defender + 0x7F0u, flags7f0);
        write_le32(defender + 0x000u, flags0);
    }

    local.defender_75c = read_le16(defender + 0x6F0u);
    write_le16(defender + 0x75Cu, local.defender_75c);

    effect_flags = effect->flags_908;
    if ((workspace_flags & (UINT32_C(1) << 5u)) != 0u) {
        effect_flags |= UINT16_C(1) << 7u;
    }

    if ((workspace_flags & (UINT32_C(1) << 4u)) != 0u) {
        effect_flags |= UINT16_C(1) << 6u;
        local.request_particle_setup = true;
    } else if ((workspace_flags & (UINT32_C(1) << 3u)) != 0u) {
        effect_flags |= UINT16_C(1) << 4u;
    } else if ((workspace_flags & (UINT32_C(1) << 2u)) != 0u) {
        effect_flags |= UINT16_C(1) << 2u;
    } else if ((workspace_flags & (UINT32_C(1) << 1u)) != 0u) {
        const bool restricted_mode =
            also_mode == UINT8_C(8) || also_mode == UINT8_C(9);
        const bool allowed_submode =
            also_sub_mode == UINT8_C(8) || also_sub_mode == UINT8_C(9);

        if (restricted_mode && !allowed_submode) {
            effect->flags_908 = effect_flags;
            local.early_mode_return = true;
            local.effect_flags_908 = effect_flags;
            local.defender_flags_7f0 = flags7f0;
            local.defender_flags_0 = flags0;
            *result = local;
            return true;
        }

        effect_flags |= UINT16_C(1) << 1u;
        local.request_particle_setup = true;
    }

    effect->flags_908 = effect_flags;
    local.defender_75e = effect_flags;
    write_le16(defender + 0x75Eu, effect_flags);

    local.effect_flags_908 = effect_flags;
    local.defender_flags_7f0 = flags7f0;
    local.defender_flags_0 = flags0;

    *result = local;
    return true;
}
