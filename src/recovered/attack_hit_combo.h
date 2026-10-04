#ifndef STF_RECOVERED_ATTACK_HIT_COMBO_H
#define STF_RECOVERED_ATTACK_HIT_COMBO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_ATTACK_HIT_COMBO_ATTACKER_MIN_SIZE = 0x861u,
    STF_ATTACK_HIT_COMBO_DEFENDER_MIN_SIZE = 0xA10u,
    STF_ATTACK_HIT_COMBO_WORKSPACE_MIN_SIZE = 0x270u
};

typedef struct stf_attack_hit_combo_result {
    bool skipped;
    bool requires_set_kamae;
    uint32_t bonus_skill;
    uint32_t hit_skill;
    uint32_t workspace_26c;
    uint8_t defender_combo_6f4;
} stf_attack_hit_combo_result;

/*
 * Recover attack_hit 0x2AE40..0x2B018.
 *
 * set_kamae_ram and total_skill_adder_g7 are reported as events/amounts rather
 * than invoked. Debug hit-combo printing is intentionally omitted.
 */
bool stf_attack_hit_combo_apply_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint8_t *workspace,
    size_t workspace_size,
    uint8_t hit_kind_50fe02,
    stf_attack_hit_combo_result *result
);

#endif
