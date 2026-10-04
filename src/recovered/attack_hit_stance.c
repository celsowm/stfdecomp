#include "attack_hit_stance.h"

#include <string.h>

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
)
{
    stf_attack_hit_stance_result local;

    if (side == NULL) {
        return false;
    }

    memset(&local, 0, sizeof(local));

    if (side->path != STF_ATTACK_SIDE_EXIT_CONTINUE) {
        return false;
    }

    if (!side->request_set_kamae) {
        if (result != NULL) {
            *result = local;
        }
        return true;
    }

    if (!stf_set_kamae_runtime_apply(
            defender_flags_0,
            selector_block,
            selector_block_size,
            arithmetic_control,
            stance_ram,
            stance_ram_size,
            resolver,
            resolver_user_data,
            &local.kamae
        )) {
        return false;
    }

    local.executed = true;
    if (result != NULL) {
        *result = local;
    }
    return true;
}
