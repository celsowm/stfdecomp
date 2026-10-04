#ifndef STF_RECOVERED_ATTACK_HIT_STANCE_H
#define STF_RECOVERED_ATTACK_HIT_STANCE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attack_hit_side_exit.h"
#include "set_kamae_runtime.h"

typedef struct stf_attack_hit_stance_result {
    bool executed;
    stf_set_kamae_runtime_result kamae;
} stf_attack_hit_stance_result;

/*
 * Consume the set_kamae_ram event emitted by the recovered attack_hit side
 * classifier.  The classifier has already applied the original bit-29 clear
 * and abort predicates; this helper only executes the requested stance refresh.
 */
bool stf_attack_hit_apply_stance_event(
    const stf_attack_side_exit_result *side,
    uint32_t defender_flags_0,
    const uint8_t *selector_block,
    size_t selector_block_size,
    uint32_t arithmetic_control,
    uint8_t *stance_ram,
    size_t stance_ram_size,
    stf_kamae_motion_resolver resolver,
    void *resolver_user_data,
    stf_attack_hit_stance_result *result
);

#endif
