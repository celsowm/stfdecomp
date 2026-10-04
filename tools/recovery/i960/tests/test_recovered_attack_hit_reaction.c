#include <stdint.h>
#include <string.h>

#include "attack_hit_reaction.h"

int main(void)
{
    stf_attack_reaction_inputs in;
    stf_attack_reaction_path path;

    memset(&in, 0, sizeof(in));
    in.damage = UINT32_C(20);
    in.scaled_damage = UINT32_C(20);
    in.defender_energy_1ac = 100;
    in.attacker_kind_821 = UINT8_C(0);

    if (!stf_attack_hit_reaction_classify(&in, &path) ||
        path != STF_ATTACK_REACTION_NORMAL) {
        return 1;
    }

    in.defender_flags_1a4 = UINT32_C(1) << 16u;
    if (!stf_attack_hit_reaction_classify(&in, &path) ||
        path != STF_ATTACK_REACTION_SPECIAL_BIT16) {
        return 2;
    }

    memset(&in, 0, sizeof(in));
    in.defender_flags_1a4 = UINT32_C(1) << 14u;
    if (!stf_attack_hit_reaction_classify(&in, &path) ||
        path != STF_ATTACK_REACTION_DOWN_COMBO) {
        return 3;
    }

    memset(&in, 0, sizeof(in));
    in.defender_flags_70c = UINT32_C(1) << 2u;
    if (!stf_attack_hit_reaction_classify(&in, &path) ||
        path != STF_ATTACK_REACTION_DOWN_COMBO) {
        return 4;
    }

    memset(&in, 0, sizeof(in));
    in.damage = UINT32_C(50);
    in.scaled_damage = UINT32_C(20);
    in.defender_energy_1ac = 100;
    if (!stf_attack_hit_reaction_classify(&in, &path) ||
        path != STF_ATTACK_REACTION_GENERIC) {
        return 5;
    }

    memset(&in, 0, sizeof(in));
    in.damage = UINT32_C(10);
    in.scaled_damage = UINT32_C(25);
    in.defender_energy_1ac = 25;
    in.attacker_kind_821 = UINT8_C(0);
    if (!stf_attack_hit_reaction_classify(&in, &path) ||
        path != STF_ATTACK_REACTION_GENERIC) {
        return 6;
    }

    in.scaled_damage = UINT32_C(24);
    if (!stf_attack_hit_reaction_classify(&in, &path) ||
        path != STF_ATTACK_REACTION_NORMAL) {
        return 7;
    }

    memset(&in, 0, sizeof(in));
    in.damage = UINT32_C(10);
    in.scaled_damage = UINT32_C(5);
    in.defender_energy_1ac = 100;
    in.attacker_kind_821 = UINT8_C(1);
    in.defender_flags_1a4 = UINT32_C(1) << 24u;
    if (!stf_attack_hit_reaction_classify(&in, &path) ||
        path != STF_ATTACK_REACTION_GENERIC) {
        return 8;
    }

    memset(&in, 0, sizeof(in));
    in.damage = UINT32_C(100);
    in.scaled_damage = UINT32_C(5);
    in.defender_energy_1ac = 100;
    in.hit_flags_50fe00 = UINT16_C(1) << 10u;
    if (!stf_attack_hit_reaction_classify(&in, &path) ||
        path != STF_ATTACK_REACTION_NORMAL) {
        return 9;
    }

    return 0;
}
