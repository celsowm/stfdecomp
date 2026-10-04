#ifndef STF_RECOVERED_ATTACK_HIT_GUARD_STATE_RUNTIME_H
#define STF_RECOVERED_ATTACK_HIT_GUARD_STATE_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attack_hit_guard_state.h"
#include "attack_hit_motion_select.h"

typedef struct stf_guard_state_runtime_result {
    bool resolved_motion;
    stf_hit_motion_select_result motion;
    stf_guard_state_result guard;
} stf_guard_state_runtime_result;

bool stf_attack_hit_guard_apply_resolved_model2(
    stf_guard_block_kind kind,
    uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint8_t *workspace,
    size_t workspace_size,
    uint16_t hit_flags_50fe00,
    stf_hit_motion_table_resolver resolver,
    void *resolver_user_data,
    stf_guard_state_runtime_result *result
);

#endif
