#include <stdint.h>
#include <string.h>

#include "damage_unit_post.h"

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

int main(void)
{
    uint8_t defender[STF_DAMAGE_UNIT_DEFENDER_MIN_SIZE];
    uint8_t workspace[STF_DAMAGE_UNIT_WORKSPACE_MIN_SIZE];
    stf_damage_unit_effect_state effect;
    stf_damage_unit_post_result result;

    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    memset(&effect, 0, sizeof(effect));

    write_le16(defender + 0x6F0u, UINT16_C(0x1234));
    write_le32(workspace + 0x26Cu, UINT32_C(1) << 3u);

    if (!stf_damage_unit_post_apply_model2(
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            &effect,
            UINT8_C(0), UINT8_C(0),
            &result
        ) ||
        !result.crush_table_inert ||
        result.early_mode_return ||
        result.request_particle_setup ||
        result.effect_flags_908 != (UINT16_C(1) << 4u) ||
        read_le16(defender + 0x75Cu) != UINT16_C(0x1234) ||
        read_le16(defender + 0x75Eu) != (UINT16_C(1) << 4u)) {
        return 1;
    }

    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    memset(&effect, 0, sizeof(effect));
    effect.active_914 = UINT32_C(1);
    write_le16(defender + 0x6F0u, UINT16_C(0x00AA));
    write_le32(workspace + 0x26Cu, (UINT32_C(1) << 5u) | (UINT32_C(1) << 4u));

    if (!stf_damage_unit_post_apply_model2(
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            &effect,
            UINT8_C(0), UINT8_C(0),
            &result
        ) ||
        !result.request_particle_setup ||
        result.particle_draw_kind != UINT16_C(1) ||
        result.particle_flag_set_mask != UINT8_C(1) ||
        (result.effect_flags_908 & (UINT16_C(1) << 7u)) == 0u ||
        (result.effect_flags_908 & (UINT16_C(1) << 6u)) == 0u ||
        (read_le32(defender) & (UINT32_C(1) << 28u)) == 0u ||
        (read_le32(defender + 0x7F0u) &
            ((UINT32_C(1) << 2u) | (UINT32_C(1) << 3u))) !=
            ((UINT32_C(1) << 2u) | (UINT32_C(1) << 3u))) {
        return 2;
    }

    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    memset(&effect, 0, sizeof(effect));
    effect.flags_908 = UINT16_C(0x20);
    write_le16(defender + 0x6F0u, UINT16_C(7));
    write_le32(workspace + 0x26Cu, UINT32_C(1) << 1u);

    if (!stf_damage_unit_post_apply_model2(
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            &effect,
            UINT8_C(8), UINT8_C(3),
            &result
        ) ||
        !result.early_mode_return ||
        result.request_particle_setup ||
        effect.flags_908 != UINT16_C(0x20) ||
        read_le16(defender + 0x75Cu) != UINT16_C(7) ||
        read_le16(defender + 0x75Eu) != UINT16_C(0)) {
        return 3;
    }

    if (!stf_damage_unit_post_apply_model2(
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            &effect,
            UINT8_C(8), UINT8_C(9),
            &result
        ) ||
        result.early_mode_return ||
        !result.request_particle_setup ||
        result.particle_draw_kind != UINT16_C(1) ||
        result.particle_flag_set_mask != UINT8_C(1) ||
        (result.effect_flags_908 & (UINT16_C(1) << 1u)) == 0u ||
        read_le16(defender + 0x75Eu) != result.effect_flags_908) {
        return 4;
    }

    return 0;
}
