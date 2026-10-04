#include "attack_hit_reaction.h"

#include <stddef.h>

static bool bit32(uint32_t value, unsigned bit)
{
    return (value & (UINT32_C(1) << bit)) != 0u;
}

static bool bit16(uint16_t value, unsigned bit)
{
    return (value & (uint16_t)(UINT16_C(1) << bit)) != 0u;
}

bool stf_attack_hit_reaction_classify(
    const stf_attack_reaction_inputs *inputs,
    stf_attack_reaction_path *path
)
{
    bool inspect_normal = false;

    if (inputs == NULL || path == NULL) {
        return false;
    }

    if (bit32(inputs->defender_flags_1a4, 16u)) {
        *path = STF_ATTACK_REACTION_SPECIAL_BIT16;
        return true;
    }

    if (bit32(inputs->defender_flags_1a4, 14u) ||
        bit32(inputs->defender_flags_70c, 2u)) {
        *path = STF_ATTACK_REACTION_DOWN_COMBO;
        return true;
    }

    if (bit32(inputs->defender_flags_1a4, 4u) ||
        bit16(inputs->hit_flags_50fe00, 14u)) {
        *path = STF_ATTACK_REACTION_GENERIC;
        return true;
    }

    if (bit16(inputs->hit_flags_50fe00, 10u)) {
        inspect_normal = true;
    } else if (inputs->damage >= UINT32_C(50)) {
        *path = STF_ATTACK_REACTION_GENERIC;
        return true;
    } else if (inputs->attacker_kind_821 != UINT8_C(1)) {
        inspect_normal = true;
    } else if (bit32(inputs->defender_flags_1a4, 24u)) {
        *path = STF_ATTACK_REACTION_GENERIC;
        return true;
    } else {
        inspect_normal = true;
    }

    if (inspect_normal) {
        if (inputs->scaled_damage >=
            (uint32_t)(uint16_t)inputs->defender_energy_1ac) {
            *path = STF_ATTACK_REACTION_GENERIC;
        } else {
            *path = STF_ATTACK_REACTION_NORMAL;
        }
        return true;
    }

    *path = STF_ATTACK_REACTION_GENERIC;
    return true;
}
