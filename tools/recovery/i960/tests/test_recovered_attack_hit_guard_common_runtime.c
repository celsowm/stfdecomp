#include <stdint.h>
#include <string.h>

#include "attack_hit_guard_common_runtime.h"

typedef struct fixture {
    uint8_t selector;
    uint32_t words[64];
    unsigned calls;
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
    ++fx->calls;
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
    stf_guard_common_runtime_result result;
    stf_guard_common_transaction_result transaction;

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    memset(enemy, 0, sizeof(enemy));
    memset(&fx, 0, sizeof(fx));

    fx.selector = UINT8_C(0);
    fx.words[2] = UINT32_C(0x222);

    attacker[0x822u] = UINT8_C(15);
    write_le32(attacker + 0x1234u, UINT32_C(5));
    write_le32(attacker + 0x1238u, UINT32_C(9));

    if (!stf_attack_hit_guard_common_apply_resolved_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            enemy, sizeof(enemy),
            UINT16_C(0), UINT8_C(0), UINT8_C(20),
            resolve_table, &fx, &result
        ) ||
        result.motion.table_index != UINT32_C(2) ||
        result.motion.motion != UINT32_C(0x222) ||
        result.guard.requires_sub_2b94c ||
        result.guard.defender_198 != UINT32_C(0x0A000222) ||
        result.guard.skill_amount != UINT32_C(7)) {
        return 1;
    }

    fx.selector = UINT8_C(9);
    if (stf_attack_hit_guard_common_apply_resolved_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            enemy, sizeof(enemy),
            UINT16_C(0), UINT8_C(0), UINT8_C(20),
            resolve_table, &fx, &result
        )) {
        return 2;
    }


    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    memset(enemy, 0, sizeof(enemy));
    fx.selector = UINT8_C(0);
    fx.words[2] = UINT32_C(0x222);
    attacker[0x822u] = UINT8_C(15);
    attacker[4u] = UINT8_C(0);
    write_le32(attacker + 0x1234u, UINT32_C(5));
    write_le32(attacker + 0x1238u, UINT32_C(9));

    if (!stf_attack_hit_guard_common_apply_transaction_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            enemy, sizeof(enemy),
            UINT16_C(0), UINT8_C(0), UINT8_C(20),
            resolve_table, &fx,
            UINT16_C(1),
            UINT32_C(1) << 2u,
            UINT32_C(0),
            UINT32_C(100),
            &transaction
        ) ||
        transaction.runtime.guard.skill_amount != UINT32_C(7) ||
        !transaction.skill.applied ||
        transaction.skill.total_skill_after != UINT32_C(107)) {
        return 3;
    }

    /* ROM loc_2AC74 early abort occurs before sub_2B94C lookup.
     * Both direct and transaction API must skip the resolver and writes. */
    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    memset(enemy, 0, sizeof(enemy));
    write_le32(attacker + 0x860u, UINT32_C(1) << 19u);
    write_le32(attacker + 0x1234u, UINT32_C(17));
    attacker[0x822u] = 0u;
    fx.selector = UINT8_C(9); /* deliberately invalid selector */
    fx.calls = 0u;
    if (!stf_attack_hit_guard_common_apply_resolved_model2(
            attacker, sizeof(attacker), defender, sizeof(defender),
            workspace, sizeof(workspace), enemy, sizeof(enemy),
            UINT16_C(0), UINT8_C(0), UINT8_C(20),
            resolve_table, &fx, &result) ||
        !result.guard.skipped || result.motion.motion != 0u ||
        fx.calls != 0u || enemy[0x109u] != 0u ||
        attacker[0x194u] != 0u ||
        attacker[0x1234u] != 17u) {
        return 4;
    }
    if (!stf_attack_hit_guard_common_apply_transaction_model2(
            attacker, sizeof(attacker), defender, sizeof(defender),
            workspace, sizeof(workspace), enemy, sizeof(enemy),
            UINT16_C(0), UINT8_C(0), UINT8_C(20),
            resolve_table, &fx, UINT16_C(1),
            UINT32_C(1) << 2u, 0u, UINT32_C(100), &transaction) ||
        !transaction.runtime.guard.skipped ||
        transaction.skill.applied ||
        transaction.skill.total_skill_after != UINT32_C(100) ||
        fx.calls != 0u) {
        return 5;
    }

    /* Even a skipped route must reject insufficient buffers. */
    if (stf_attack_hit_guard_common_apply_resolved_model2(
            attacker, sizeof(attacker), defender, sizeof(defender),
            workspace, sizeof(workspace), enemy, sizeof(enemy) - 1u,
            0u, 0u, 20u, NULL, NULL, &result)) {
        return 6;
    }

    return 0;
}
