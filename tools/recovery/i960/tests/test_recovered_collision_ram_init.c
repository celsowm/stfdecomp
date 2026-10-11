#include <stdint.h>
#include <string.h>
#include "collision_ram_init.h"

int main(void)
{
    uint8_t ram[STF_COLLISION_RAM_INIT_WORKSPACE_MIN_SIZE];
    uint8_t f0[STF_COLLISION_RAM_INIT_FIGHTER_MIN_SIZE];
    uint8_t f1[STF_COLLISION_RAM_INIT_FIGHTER_MIN_SIZE];
    size_t i;

    memset(ram, 0xA5, sizeof(ram));
    memset(f0, 0x5A, sizeof(f0));
    memset(f1, 0xC3, sizeof(f1));

    if (stf_collision_ram_init_apply_model2(
            ram, sizeof(ram)-1u, f0, sizeof(f0), f1, sizeof(f1)) ||
        stf_collision_ram_init_apply_model2(
            ram, sizeof(ram), f0, sizeof(f0), f1, sizeof(f1)-1u) ||
        stf_collision_ram_init_apply_model2(
            NULL, sizeof(ram), f0, sizeof(f0), f1, sizeof(f1))) return 1;
    if (ram[0x170u] != 0xA5u || f0[0x75Cu] != 0x5Au) return 2;

    if (!stf_collision_ram_init_apply_model2(
            ram, sizeof(ram), f0, sizeof(f0), f1, sizeof(f1))) return 3;
    for (i=0u;i<sizeof(ram);++i) {
        const uint8_t expected =
            (i>=0x170u && i<0x1B0u) || (i>=0x98u && i<0xACu)
                ? 0u : UINT8_C(0xA5);
        if (ram[i] != expected) return 4;
    }
    for (i=0u;i<sizeof(f0);++i) {
        const bool clear = (i>=0x75Cu && i<0x760u) ||
                           (i>=0xA68u && i<0xA70u);
        if (f0[i] != (clear ? 0u : UINT8_C(0x5A)) ||
            f1[i] != (clear ? 0u : UINT8_C(0xC3))) return 5;
    }
    return 0;
}
