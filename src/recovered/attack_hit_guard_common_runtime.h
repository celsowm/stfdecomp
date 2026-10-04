#ifndef STF_RECOVERED_ATTACK_HIT_GUARD_COMMON_RUNTIME_H
#define STF_RECOVERED_ATTACK_HIT_GUARD_COMMON_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attack_hit_guard_common.h"
#include "attack_hit_motion_select.h"
#include "skill_accounting.h"

typedef struct stf_guard_common_runtime_result {
    stf_hit_motion_select_result motion;
    stf_guard_common_result guard;
} stf_guard_common_runtime_result;

typedef struct stf_guard_common_transaction_result {
    stf_guard_common_runtime_result runtime;
    stf_skill_accounting_result skill;
} stf_guard_common_transaction_result;

bool stf_attack_hit_guard_common_apply_transaction_model2(
    uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint8_t *workspace,
    size_t workspace_size,
    uint8_t *enemy_slot,
    size_t enemy_slot_size,
    uint16_t hit_flags_50fe00,
    uint8_t hit_flags_50fe03,
    uint8_t guard_limit,
    stf_hit_motion_table_resolver resolver,
    void *resolver_user_data,
    uint16_t rank_mode,
    uint32_t select0_flag,
    uint32_t select1_flag,
    uint32_t total_skill_before,
    stf_guard_common_transaction_result *result
);

bool stf_attack_hit_guard_common_apply_resolved_model2(
    uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint8_t *workspace,
    size_t workspace_size,
    uint8_t *enemy_slot,
    size_t enemy_slot_size,
    uint16_t hit_flags_50fe00,
    uint8_t hit_flags_50fe03,
    uint8_t guard_limit,
    stf_hit_motion_table_resolver resolver,
    void *resolver_user_data,
    stf_guard_common_runtime_result *result
);

#endif
