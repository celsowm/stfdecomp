#include <stdint.h>
#include <string.h>

#include "damage_unit_runtime.h"

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

static int test_gate_returns_before_post(void)
{
    uint8_t attacker[STF_DAMAGE_UNIT_ATTACKER_MIN_SIZE];
    uint8_t defender[STF_DAMAGE_UNIT_DEFENDER_MIN_SIZE];
    uint8_t workspace[STF_DAMAGE_UNIT_WORKSPACE_MIN_SIZE];
    stf_damage_unit_effect_state effect;
    stf_damage_unit_runtime_result result;

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    memset(&effect, 0, sizeof(effect));

    write_le16(defender + 0x6F0u, UINT16_C(0x0040));
    write_le16(defender + 0x75Cu, UINT16_C(0xCAFE));

    if (!stf_damage_unit_runtime_apply_model2(
            attacker,
            sizeof(attacker),
            defender,
            sizeof(defender),
            workspace,
            sizeof(workspace),
            UINT8_C(2),
            &effect,
            UINT8_C(0),
            UINT8_C(0),
            &result
        ) ||
        !result.damage.skipped ||
        result.post_applied ||
        read_le16(defender + 0x75Cu) != UINT16_C(0xCAFE)) {
        return 1;
    }

    return 0;
}

static int test_prefix_flows_into_post_tail(void)
{
    uint8_t attacker[STF_DAMAGE_UNIT_ATTACKER_MIN_SIZE];
    uint8_t defender[STF_DAMAGE_UNIT_DEFENDER_MIN_SIZE];
    uint8_t workspace[STF_DAMAGE_UNIT_WORKSPACE_MIN_SIZE];
    stf_damage_unit_effect_state effect;
    stf_damage_unit_runtime_result result;

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    memset(&effect, 0, sizeof(effect));

    attacker[0x821u] = UINT8_C(0);
    attacker[0x822u] = UINT8_C(4);
    write_le16(defender + 0x6F0u, UINT16_C(1));
    write_le32(defender + 0xAF0u, UINT32_MAX);
    write_le32(defender + 0xAF4u, UINT32_MAX);
    write_le32(
        workspace + 0x26Cu,
        UINT32_C(1) | (UINT32_C(1) << 5u)
    );

    if (!stf_damage_unit_runtime_apply_model2(
            attacker,
            sizeof(attacker),
            defender,
            sizeof(defender),
            workspace,
            sizeof(workspace),
            UINT8_C(2),
            &effect,
            UINT8_C(0),
            UINT8_C(0),
            &result
        ) ||
        result.damage.skipped ||
        !result.damage.matched_slot ||
        result.damage.selected_slot != UINT8_C(0) ||
        result.damage.accumulator_after != UINT16_C(4) ||
        !result.post_applied ||
        result.post.defender_75c != UINT16_C(1) ||
        read_le16(defender + 0x75Cu) != UINT16_C(1) ||
        (result.post.effect_flags_908 & (UINT16_C(1) << 7u)) == 0u ||
        read_le16(defender + 0x75Eu) != (UINT16_C(1) << 7u)) {
        return 1;
    }

    return 0;
}

int main(void)
{
    if (test_gate_returns_before_post() != 0) {
        return 1;
    }
    if (test_prefix_flows_into_post_tail() != 0) {
        return 1;
    }
    return 0;
}
