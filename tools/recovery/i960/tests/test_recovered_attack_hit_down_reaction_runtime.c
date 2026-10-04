#include <stdint.h>
#include <string.h>

#include "attack_hit_down_reaction_runtime.h"
#include "attack_hit_guard_common_runtime.h"

typedef struct fixture {
    uint8_t selector;
    uint32_t words[64];
} fixture;

static bool resolve_table(
    uint8_t selector,
    const uint32_t **table_words,
    size_t *table_word_count,
    void *user_data
)
{
    fixture *fx = (fixture *)user_data;
    if (fx == NULL || table_words == NULL || table_word_count == NULL ||
        selector != fx->selector) {
        return false;
    }
    *table_words = fx->words;
    *table_word_count = sizeof(fx->words) / sizeof(fx->words[0]);
    return true;
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
    uint8_t attacker[STF_GUARD_COMMON_ATTACKER_MIN_SIZE];
    uint8_t defender[STF_GUARD_COMMON_DEFENDER_MIN_SIZE];
    uint8_t workspace[STF_GUARD_COMMON_WORKSPACE_MIN_SIZE];
    uint8_t enemy[STF_GUARD_COMMON_ENEMY_MIN_SIZE];
    fixture fx;
    stf_down_reaction_runtime_result down;
    stf_guard_common_runtime_result guard;

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    memset(enemy, 0, sizeof(enemy));
    memset(&fx, 0, sizeof(fx));

    fx.selector = UINT8_C(0);
    fx.words[34] = UINT32_C(0x44);
    fx.words[40] = UINT32_C(0x55);
    fx.words[2] = UINT32_C(0x222);

    if (!stf_attack_hit_generic_down_reaction_apply_resolved_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            UINT16_C(0), UINT8_C(0), UINT32_C(41), true,
            resolve_table, &fx, &down
        ) ||
        down.motion.table_index != UINT32_C(34) ||
        down.motion.motion != UINT32_C(0x44) ||
        down.reaction.selected_motion != UINT32_C(0x44) ||
        down.reaction.requires_sub_2b94c ||
        !down.reaction.requires_calc_mht ||
        down.reaction.defender_down_combo_6f5 != UINT8_C(1)) {
        return 1;
    }

    memset(defender, 0, sizeof(defender));
    if (!stf_attack_hit_special_bit16_reaction_apply_resolved_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            UINT16_C(0), UINT32_C(40),
            resolve_table, &fx, &down
        ) ||
        down.motion.table_index != UINT32_C(40) ||
        down.motion.motion != UINT32_C(0x55) ||
        down.reaction.selected_motion != UINT32_C(0x55) ||
        down.reaction.requires_sub_2b94c) {
        return 2;
    }

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    memset(enemy, 0, sizeof(enemy));

    attacker[0x822u] = UINT8_C(15);
    write_le32(attacker + 0x1234u, UINT32_C(5));
    write_le32(attacker + 0x1238u, UINT32_C(9));

    if (!stf_attack_hit_guard_common_apply_resolved_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            enemy, sizeof(enemy),
            UINT16_C(0), UINT8_C(0), UINT8_C(20),
            resolve_table, &fx, &guard
        ) ||
        guard.motion.table_index != UINT32_C(2) ||
        guard.motion.motion != UINT32_C(0x222) ||
        guard.guard.requires_sub_2b94c ||
        guard.guard.defender_198 != UINT32_C(0x0A000222) ||
        guard.guard.skill_amount != UINT32_C(7)) {
        return 3;
    }

    return 0;
}
