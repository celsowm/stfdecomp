#include "collision_frame.h"

#include <string.h>

bool stf_collision_frame_check_model2(
    const stf_collision_frame_model2 *frame,
    stf_collision_frame_result *result
)
{
    stf_collision_frame_result local;
    uint8_t i;

    if (frame == NULL || result == NULL ||
        frame->fighters[0] == NULL || frame->fighters[1] == NULL ||
        frame->workspace == NULL || frame->mapping == NULL ||
        frame->effect == NULL || frame->copro_hit_xyz == NULL ||
        frame->fighter_sizes[0] < STF_COLLISION_ATTACK_FIGHTER_MODEL2_MIN_SIZE ||
        frame->fighter_sizes[1] < STF_COLLISION_ATTACK_FIGHTER_MODEL2_MIN_SIZE ||
        frame->workspace_size < STF_COLLISION_ATTACK_WORKSPACE_MODEL2_MIN_SIZE ||
        frame->effect_size < STF_COLLISION_ATTACK_EFFECT_MODEL2_SIZE ||
        frame->copro_hit_xyz_size < STF_COLLISION_ATTACK_EFFECT_XYZ_SIZE ||
        frame->fighters[0][4u] != 0u || frame->fighters[1][4u] != 1u) {
        return false;
    }
    for (i = 0u; i < 2u; ++i) {
        if (frame->r5_selected_xyz[i] == NULL ||
            frame->r5_selected_xyz_sizes[i] < STF_COLLISION_ATTACK_EFFECT_XYZ_SIZE) {
            return false;
        }
    }

    memset(&local, 0, sizeof(local));
    for (i = 0u; i < 2u; ++i) {
        const uint8_t other = (uint8_t)(1u - i);
        if (!stf_collision_attack_apply_model2(
                frame->fighters[i], frame->fighter_sizes[i],
                frame->fighters[other], frame->fighter_sizes[other],
                frame->workspace, frame->workspace_size,
                frame->attack_profile_bits[i], frame->mapping,
                &local.checks[i])) {
            return false;
        }
        if (!stf_collision_attack_effect_apply_model2(
                &local.checks[i],
                frame->fighters[other], frame->fighter_sizes[other],
                frame->r5_selected_xyz[i], frame->r5_selected_xyz_sizes[i],
                frame->effect, frame->effect_size,
                frame->copro_hit_xyz, frame->copro_hit_xyz_size)) {
            return false;
        }
    }

    if (!stf_collision_hit_order_read_model2(
            frame->fighters[0], frame->fighter_sizes[0],
            frame->fighters[1], frame->fighter_sizes[1],
            frame->workspace, frame->workspace_size, &local.order)) {
        return false;
    }
    *result = local;
    return true;
}

bool stf_collision_frame_dispatch(
    const stf_collision_hit_order *order,
    stf_collision_attack_dispatch_fn callback,
    void *user_data,
    size_t *completed
)
{
    uint8_t i;
    if (completed != NULL) {
        *completed = 0u;
    }
    if (order == NULL || callback == NULL || order->count > 2u) {
        return false;
    }
    for (i = 0u; i < order->count; ++i) {
        if (order->attacker[i] > 1u) {
            return false;
        }
    }
    if (order->count == 2u &&
        (order->attacker[0] != 1u || order->attacker[1] != 0u)) {
        return false;
    }

    for (i = 0u; i < order->count; ++i) {
        const uint8_t attacker = order->attacker[i];
        if (!callback(user_data, attacker, (uint8_t)(1u - attacker))) {
            return false;
        }
        if (completed != NULL) {
            *completed += 1u;
        }
    }
    return true;
}


bool stf_collision_frame_check_computed_model2(
    const stf_collision_frame_model2 *frame,
    stf_collision_frame_result *result
)
{
    stf_collision_frame_model2 copied;
    unsigned i;

    if (frame == NULL || result == NULL) {
        return false;
    }
    copied = *frame;
    for (i = 0u; i < 2u; ++i) {
        const uint8_t *fighter = frame->fighters[i];
        if (fighter == NULL || frame->fighter_sizes[i] < 0xACCu) {
            return false;
        }
        copied.attack_profile_bits[i] =
            (uint32_t)fighter[0xAC8u] |
            ((uint32_t)fighter[0xAC9u] << 8u) |
            ((uint32_t)fighter[0xACAu] << 16u) |
            ((uint32_t)fighter[0xACBu] << 24u);
    }
    return stf_collision_frame_check_model2(&copied, result);
}
