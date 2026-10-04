#include <stdint.h>
#include <string.h>

#include "attack_hit_normal_reaction_runtime.h"

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

static void write_le16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
}

int main(void)
{
    uint8_t attacker[STF_NORMAL_REACTION_ATTACKER_MIN_SIZE];
    uint8_t defender[STF_NORMAL_REACTION_DEFENDER_MIN_SIZE];
    fixture fx;
    stf_normal_reaction_runtime_result result;

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(&fx, 0, sizeof(fx));

    fx.selector = UINT8_C(2);
    fx.words[18] = UINT32_C(0x10);

    write_le16(attacker + 0x80Cu, UINT16_C(100));
    write_le16(attacker + 0x808u, UINT16_C(0));
    write_le16(attacker + 0x1AAu, UINT16_C(0));
    write_le16(attacker + 0x82Au, UINT16_C(0));
    write_le16(attacker + 0x026u, UINT16_C(0));
    write_le16(defender + 0x5B4u, UINT16_C(0));

    if (!stf_attack_hit_normal_reaction_apply_resolved_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            UINT16_C(2),
            UINT8_C(0),
            UINT32_C(20),
            UINT32_C(2),
            resolve_table,
            &fx,
            &result
        ) ||
        result.motion.table_index != UINT32_C(18) ||
        result.motion.motion != UINT32_C(0x10) ||
        result.reaction.defender_198 != UINT32_C(0x0B000010) ||
        result.reaction.defender_5de != INT16_C(6) ||
        result.reaction.reaction_argument != 1 ||
        result.reaction.requires_sub_2b94c) {
        return 1;
    }

    fx.selector = UINT8_C(9);
    if (stf_attack_hit_normal_reaction_apply_resolved_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            UINT16_C(2),
            UINT8_C(0),
            UINT32_C(20),
            UINT32_C(2),
            resolve_table,
            &fx,
            &result
        )) {
        return 2;
    }

    return 0;
}
