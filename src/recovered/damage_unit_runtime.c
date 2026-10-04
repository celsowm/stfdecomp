#include "damage_unit_runtime.h"

#include <string.h>

bool stf_damage_unit_runtime_apply_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    const uint8_t *workspace,
    size_t workspace_size,
    uint8_t num_rounds_to_win,
    stf_damage_unit_effect_state *effect,
    uint8_t also_mode,
    uint8_t also_sub_mode,
    stf_damage_unit_runtime_result *result
)
{
    stf_damage_unit_runtime_result local;

    if (result == NULL) {
        return false;
    }

    memset(&local, 0, sizeof(local));

    if (!stf_damage_unit_apply_model2(
            attacker,
            attacker_size,
            defender,
            defender_size,
            workspace,
            workspace_size,
            num_rounds_to_win,
            effect,
            &local.damage
        )) {
        return false;
    }

    if (local.damage.skipped) {
        *result = local;
        return true;
    }

    if (!stf_damage_unit_post_apply_model2(
            defender,
            defender_size,
            workspace,
            workspace_size,
            effect,
            also_mode,
            also_sub_mode,
            &local.post
        )) {
        return false;
    }

    local.post_applied = true;
    *result = local;
    return true;
}
