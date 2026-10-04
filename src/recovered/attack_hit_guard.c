#include "attack_hit_guard.h"

static bool bit32(uint32_t value, unsigned bit)
{
    return (value & (UINT32_C(1) << bit)) != 0u;
}

static bool bit16(uint16_t value, unsigned bit)
{
    return (value & (uint16_t)(UINT16_C(1) << bit)) != 0u;
}

bool stf_attack_hit_guard_classify(
    const stf_attack_guard_inputs *inputs,
    stf_attack_guard_path *path
)
{
    if (inputs == NULL || path == NULL) {
        return false;
    }

    if (!bit32(inputs->opponent_flags_1a4, 13u) ||
        bit32(inputs->opponent_flags_5b8, 0u)) {
        *path = STF_ATTACK_GUARD_EXIT;
        return true;
    }

    if (bit32(inputs->attacker_flags_000, 18u) &&
        inputs->attacker_field_c7c < 640) {
        *path = STF_ATTACK_GUARD_BRANCH_2AC74;
        return true;
    }

    if (inputs->opponent_field_c70 > 1) {
        if (bit16(inputs->hit_flags_50fe00, 12u)) {
            *path = STF_ATTACK_GUARD_BLOCK_A_2AA70;
            return true;
        }
        if (bit16(inputs->hit_flags_50fe00, 13u)) {
            *path = STF_ATTACK_GUARD_BLOCK_B_2AB54;
            return true;
        }
    } else {
        if (bit16(inputs->hit_flags_50fe00, 12u) ||
            bit16(inputs->hit_flags_50fe00, 13u)) {
            *path = STF_ATTACK_GUARD_BLOCK_B_2AB54;
            return true;
        }
    }

    if (!bit16(inputs->hit_flags_50fe00, 9u)) {
        *path = STF_ATTACK_GUARD_BRANCH_2AC74;
    } else {
        *path = STF_ATTACK_GUARD_EXIT;
    }
    return true;
}
