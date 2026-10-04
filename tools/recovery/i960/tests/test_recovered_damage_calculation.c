#include <stdint.h>
#include <string.h>

#include "damage_calculation.h"

static void write_le16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
}

static uint16_t read_le16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8u));
}

int main(void)
{
    stf_damage_calculation_inputs in;
    stf_damage_calculation_result out;

    memset(&in, 0, sizeof(in));
    in.receiver_energy = 100;
    in.dealer_energy = 100;
    in.energy_max = 200;
    in.game_timer = 1;
    in.damage = 20u;

    if (!stf_damage_calculation_compute(&in, &out) ||
        out.scaled_damage != 20u ||
        !out.energy_applied ||
        out.receiver_energy_after != 80 ||
        out.receiver_knocked_out ||
        !out.request_ring_scatter) {
        return 1;
    }

    in.receiver_energy = 150;
    in.dealer_energy = 50;
    if (!stf_damage_calculation_compute(&in, &out) ||
        out.scaled_damage != 25u ||
        out.receiver_energy_after != 125) {
        return 2;
    }

    in.receiver_energy = 10;
    in.dealer_energy = 10;
    in.damage = 20u;
    if (!stf_damage_calculation_compute(&in, &out) ||
        out.receiver_energy_after != 0 ||
        !out.receiver_knocked_out) {
        return 3;
    }

    in.game_timer = 0;
    if (!stf_damage_calculation_compute(&in, &out) ||
        out.energy_applied ||
        out.receiver_energy_after != 10 ||
        out.request_ring_scatter) {
        return 4;
    }

    in.game_timer = 1;
    in.receiver_character = UINT8_C(16);
    if (!stf_damage_calculation_compute(&in, &out) || out.energy_applied) {
        return 5;
    }

    in.receiver_character = 0u;
    in.disable_energy_scaling = true;
    in.receiver_energy = 100;
    in.dealer_energy = 0;
    in.damage = 20u;
    if (!stf_damage_calculation_compute(&in, &out) ||
        out.scaled_damage != 20u) {
        return 6;
    }

    {
        uint8_t receiver[STF_DAMAGE_RECEIVER_MIN_SIZE];
        uint8_t dealer[STF_DAMAGE_DEALER_MIN_SIZE];

        memset(receiver, 0, sizeof(receiver));
        memset(dealer, 0, sizeof(dealer));
        write_le16(receiver + 0x1ACu, 10u);
        write_le16(dealer + 0x1ACu, 10u);

        if (!stf_damage_calculation_apply_model2(
                receiver,
                sizeof(receiver),
                dealer,
                sizeof(dealer),
                200,
                false,
                1,
                20u,
                &out
            )) {
            return 7;
        }

        if (read_le16(dealer + 0x1F70u) != 20u ||
            read_le16(receiver + 0x0C54u) != 20u ||
            read_le16(receiver + 0x1ACu) != 0u ||
            (receiver[0] & UINT8_C(0x20)) == 0u ||
            !out.request_ring_scatter) {
            return 8;
        }
    }

    return 0;
}
