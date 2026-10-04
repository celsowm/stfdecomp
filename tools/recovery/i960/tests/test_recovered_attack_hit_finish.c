#include <stdint.h>
#include <string.h>

#include "attack_hit_finish.h"

static void write_le16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
}

int main(void)
{
    stf_attack_finish_inputs in;
    stf_attack_finish_result out;
    uint8_t attacker[STF_ATTACK_FINISH_ATTACKER_MIN_SIZE];
    uint8_t defender[STF_ATTACK_FINISH_DEFENDER_MIN_SIZE];

    memset(&in, 0, sizeof(in));
    in.attacker_energy_1ac = 50;
    in.defender_energy_1ac = 80;
    in.energy_max = 100;
    in.damage = 20;

    if (!stf_attack_hit_finish_compute(&in, &out) ||
        out.energy_difference_percent != 30 ||
        out.scaled_damage != UINT32_C(26) ||
        out.defender_energy_after != 54 ||
        out.finish_blow ||
        out.bonus_skill != 0u) {
        return 1;
    }

    in.attacker_energy_1ac = 100;
    in.defender_energy_1ac = 50;
    in.energy_max = 100;
    in.damage = 20;

    if (!stf_attack_hit_finish_compute(&in, &out) ||
        out.energy_difference_percent != -30 ||
        out.scaled_damage != UINT32_C(14) ||
        out.defender_energy_after != 36) {
        return 2;
    }

    in.attacker_energy_1ac = 0;
    in.defender_energy_1ac = 90;
    in.energy_max = 100;
    in.damage = 20;

    if (!stf_attack_hit_finish_compute(&in, &out) ||
        out.energy_difference_percent != 45 ||
        out.scaled_damage != UINT32_C(29)) {
        return 3;
    }

    in.disable_energy_scaling = true;
    in.damage = 23;

    if (!stf_attack_hit_finish_compute(&in, &out) ||
        out.scaled_damage != UINT32_C(23)) {
        return 4;
    }

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    write_le16(attacker + 0x1ACu, UINT16_C(50));
    write_le16(defender + 0x1ACu, UINT16_C(10));
    attacker[0x7D2u] = UINT8_C(0x80);

    if (!stf_attack_hit_finish_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            100, false, UINT32_C(20), &out
        ) ||
        !out.finish_blow ||
        out.scaled_damage != UINT32_C(14) ||
        out.bonus_skill != UINT32_C(500) ||
        attacker[0x7D2u] != UINT8_C(0x81)) {
        return 5;
    }

    return 0;
}
