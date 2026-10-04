#include <stddef.h>
#include <stdint.h>

#include "attack_hit_guard.h"

int main(void)
{
    stf_attack_guard_inputs in = {0};
    stf_attack_guard_path path = STF_ATTACK_GUARD_CONTINUE;

    in.opponent_flags_1a4 = UINT32_C(1) << 13u;

    if (!stf_attack_hit_guard_classify(&in, &path) ||
        path != STF_ATTACK_GUARD_BRANCH_2AC74) {
        return 1;
    }

    in.hit_flags_50fe00 = UINT16_C(1) << 9u;
    if (!stf_attack_hit_guard_classify(&in, &path) ||
        path != STF_ATTACK_GUARD_EXIT) {
        return 2;
    }

    in.opponent_field_c70 = 2;
    in.hit_flags_50fe00 = UINT16_C(1) << 12u;
    if (!stf_attack_hit_guard_classify(&in, &path) ||
        path != STF_ATTACK_GUARD_BLOCK_A_2AA70) {
        return 3;
    }

    in.hit_flags_50fe00 = UINT16_C(1) << 13u;
    if (!stf_attack_hit_guard_classify(&in, &path) ||
        path != STF_ATTACK_GUARD_BLOCK_B_2AB54) {
        return 4;
    }

    in.opponent_field_c70 = 1;
    in.hit_flags_50fe00 = UINT16_C(1) << 12u;
    if (!stf_attack_hit_guard_classify(&in, &path) ||
        path != STF_ATTACK_GUARD_BLOCK_B_2AB54) {
        return 5;
    }

    in.attacker_flags_000 = UINT32_C(1) << 18u;
    in.attacker_field_c7c = 639;
    in.hit_flags_50fe00 = UINT16_C(1) << 12u;
    if (!stf_attack_hit_guard_classify(&in, &path) ||
        path != STF_ATTACK_GUARD_BRANCH_2AC74) {
        return 6;
    }

    in.attacker_field_c7c = 640;
    if (!stf_attack_hit_guard_classify(&in, &path) ||
        path != STF_ATTACK_GUARD_BLOCK_B_2AB54) {
        return 7;
    }

    in.opponent_flags_5b8 = UINT32_C(1);
    if (!stf_attack_hit_guard_classify(&in, &path) ||
        path != STF_ATTACK_GUARD_EXIT) {
        return 8;
    }

    in.opponent_flags_5b8 = 0u;
    in.opponent_flags_1a4 = 0u;
    if (!stf_attack_hit_guard_classify(&in, &path) ||
        path != STF_ATTACK_GUARD_EXIT) {
        return 9;
    }

    if (stf_attack_hit_guard_classify(NULL, &path) ||
        stf_attack_hit_guard_classify(&in, NULL)) {
        return 10;
    }

    return 0;
}
