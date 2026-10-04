#include <stdint.h>
#include <string.h>

#include "attack_hit_guard_state_runtime.h"

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
    uint8_t attacker[STF_GUARD_STATE_ATTACKER_MIN_SIZE];
    uint8_t defender[STF_GUARD_STATE_DEFENDER_MIN_SIZE];
    uint8_t workspace[STF_GUARD_STATE_WORKSPACE_MIN_SIZE];
    fixture fx;
    stf_guard_state_runtime_result result;

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    memset(&fx, 0, sizeof(fx));

    fx.selector = UINT8_C(0);
    fx.words[10] = UINT32_C(0x123);

    if (!stf_attack_hit_guard_apply_resolved_model2(
            STF_GUARD_BLOCK_B,
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            UINT16_C(0),
            resolve_table, &fx,
            &result
        ) ||
        !result.resolved_motion ||
        result.motion.table_index != UINT32_C(10) ||
        result.motion.motion != UINT32_C(0x123) ||
        result.guard.requires_sub_2b94c ||
        result.guard.defender_198 != UINT32_C(0x0B000123)) {
        return 1;
    }

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));

    if (!stf_attack_hit_guard_apply_resolved_model2(
            STF_GUARD_BLOCK_A,
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            UINT16_C(0),
            NULL, NULL,
            &result
        ) ||
        result.resolved_motion ||
        result.guard.defender_198 != UINT32_C(0x0A00013D)) {
        return 2;
    }

    fx.selector = UINT8_C(9);
    if (stf_attack_hit_guard_apply_resolved_model2(
            STF_GUARD_BLOCK_B,
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            UINT16_C(0),
            resolve_table, &fx,
            &result
        )) {
        return 3;
    }

    return 0;
}
