#include <stdint.h>
#include <string.h>

#include "attack_hit_sound.h"

static void write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

int main(void)
{
    uint8_t attacker[STF_ATTACK_HIT_SOUND_ATTACKER_MIN_SIZE];
    uint8_t defender[STF_ATTACK_HIT_SOUND_DEFENDER_MIN_SIZE];
    stf_attack_hit_sound_plan plan;

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));

    if (!stf_attack_hit_sound_plan_model2(
            attacker, sizeof(attacker), defender, sizeof(defender), 0u, &plan
        ) ||
        plan.kind != STF_ATTACK_HIT_SOUND_NONE) {
        return 1;
    }

    write_le32(attacker + 0x1A4u, UINT32_C(1) << 18u);
    if (!stf_attack_hit_sound_plan_model2(
            attacker, sizeof(attacker), defender, sizeof(defender), 20u, &plan
        ) ||
        plan.kind != STF_ATTACK_HIT_SOUND_NONE) {
        return 2;
    }

    write_le32(attacker + 0x1A4u, 0u);
    attacker[0x823u] = 4u;
    defender[0x1B1u] = 3u;
    if (!stf_attack_hit_sound_plan_model2(
            attacker, sizeof(attacker), defender, sizeof(defender), 20u, &plan
        ) ||
        plan.kind != STF_ATTACK_HIT_SOUND_LIST ||
        plan.source != STF_ATTACK_HIT_SOUND_SOURCE_OFF_DBF4C ||
        plan.source_index != 3u ||
        !plan.zero_terminated_list) {
        return 3;
    }

    defender[0x1B1u] = UINT8_C(0x0C);
    if (!stf_attack_hit_sound_plan_model2(
            attacker, sizeof(attacker), defender, sizeof(defender), 20u, &plan
        ) ||
        plan.source != STF_ATTACK_HIT_SOUND_SOURCE_AUDIO_LIST) {
        return 4;
    }

    defender[0x1B1u] = 1u;
    if (!stf_attack_hit_sound_plan_model2(
            attacker, sizeof(attacker), defender, sizeof(defender), 20u, &plan
        ) ||
        plan.source != STF_ATTACK_HIT_SOUND_SOURCE_OFF_DBE44) {
        return 5;
    }

    attacker[0x823u] = 0u;
    attacker[0x820u] = 5u;
    defender[0x1B0u] = 2u;

    if (!stf_attack_hit_sound_plan_model2(
            attacker, sizeof(attacker), defender, sizeof(defender), 14u, &plan
        ) ||
        plan.kind != STF_ATTACK_HIT_SOUND_SINGLE ||
        plan.source != STF_ATTACK_HIT_SOUND_SOURCE_DWORD_DB964 ||
        plan.tier != STF_ATTACK_HIT_SOUND_TIER_LIGHT ||
        plan.source_index != 6u) {
        return 6;
    }

    if (!stf_attack_hit_sound_plan_model2(
            attacker, sizeof(attacker), defender, sizeof(defender), 15u, &plan
        ) ||
        plan.tier != STF_ATTACK_HIT_SOUND_TIER_MEDIUM ||
        plan.source_index != 7u) {
        return 7;
    }

    if (!stf_attack_hit_sound_plan_model2(
            attacker, sizeof(attacker), defender, sizeof(defender), 30u, &plan
        ) ||
        plan.tier != STF_ATTACK_HIT_SOUND_TIER_HEAVY ||
        plan.source_index != 8u) {
        return 8;
    }

    attacker[0x820u] = 2u;
    if (!stf_attack_hit_sound_plan_model2(
            attacker, sizeof(attacker), defender, sizeof(defender), 30u, &plan
        ) ||
        plan.source != STF_ATTACK_HIT_SOUND_SOURCE_DWORD_DB6F4) {
        return 9;
    }

    if (stf_attack_hit_sound_plan_model2(
            attacker,
            STF_ATTACK_HIT_SOUND_ATTACKER_MIN_SIZE - 1u,
            defender,
            sizeof(defender),
            20u,
            &plan
        )) {
        return 10;
    }

    return 0;
}
