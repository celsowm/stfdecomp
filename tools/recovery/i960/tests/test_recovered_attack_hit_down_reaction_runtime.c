#include <stdint.h>
#include <string.h>

#include "attack_hit_down_reaction_runtime.h"

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

int main(void)
{
    uint8_t attacker[0x82Cu];
    uint8_t defender[STF_DOWN_REACTION_DEFENDER_MIN_SIZE];
    fixture fx;
    stf_down_reaction_runtime_result result;

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(&fx, 0, sizeof(fx));

    fx.selector = UINT8_C(0);
    fx.words[34] = UINT32_C(0x44);
    fx.words[40] = UINT32_C(0x55);

    if (!stf_attack_hit_generic_down_reaction_apply_resolved_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            UINT16_C(0), UINT8_C(0), UINT32_C(41), true,
            resolve_table, &fx, &result
        ) ||
        result.motion.table_index != UINT32_C(34) ||
        result.motion.motion != UINT32_C(0x44) ||
        result.reaction.selected_motion != UINT32_C(0x44) ||
        result.reaction.requires_sub_2b94c ||
        !result.reaction.requires_calc_mht ||
        result.reaction.defender_down_combo_6f5 != UINT8_C(1)) {
        return 1;
    }

    memset(defender, 0, sizeof(defender));
    if (!stf_attack_hit_special_bit16_reaction_apply_resolved_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            UINT16_C(0), UINT32_C(40),
            resolve_table, &fx, &result
        ) ||
        result.motion.table_index != UINT32_C(40) ||
        result.motion.motion != UINT32_C(0x55) ||
        result.reaction.selected_motion != UINT32_C(0x55) ||
        result.reaction.requires_sub_2b94c) {
        return 2;
    }

    fx.selector = UINT8_C(9);
    if (stf_attack_hit_generic_down_reaction_apply_resolved_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            UINT16_C(0), UINT8_C(0), UINT32_C(41), false,
            resolve_table, &fx, &result
        )) {
        return 3;
    }

    return 0;
}
