#include <stdint.h>
#include <string.h>

#include "attack_hit_sound_runtime.h"

static void write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

int main(void)
{
    uint8_t table[64];
    stf_attack_hit_sound_plan plan;
    stf_attack_hit_sound_resolved resolved;
    size_t index = 0u;

    memset(table, 0, sizeof(table));
    memset(&plan, 0, sizeof(plan));

    plan.kind = STF_ATTACK_HIT_SOUND_NONE;
    if (!stf_attack_hit_sound_resolve(
            &plan, NULL, 0u, &resolved
        ) ||
        resolved.id_count != 0u) {
        return 1;
    }

    for (index = 0u; index < 8u; ++index) {
        write_le32(table + index * 4u, UINT32_C(100) + (uint32_t)index);
    }

    plan.kind = STF_ATTACK_HIT_SOUND_SINGLE;
    plan.source_index = 3u;
    if (!stf_attack_hit_sound_resolve(
            &plan, table, sizeof(table), &resolved
        ) ||
        resolved.id_count != 1u ||
        resolved.ids[0] != UINT32_C(103)) {
        return 2;
    }

    plan.source_index = 100u;
    if (stf_attack_hit_sound_resolve(
            &plan, table, sizeof(table), &resolved
        )) {
        return 3;
    }

    memset(table, 0, sizeof(table));
    write_le32(table + 3u, UINT32_C(0x11111111));
    write_le32(table + 7u, UINT32_C(0x22222222));
    write_le32(table + 11u, UINT32_C(0x33333333));
    write_le32(table + 15u, 0u);

    plan.kind = STF_ATTACK_HIT_SOUND_LIST;
    plan.zero_terminated_list = true;
    plan.source_index = 3u;

    if (!stf_attack_hit_sound_resolve(
            &plan, table, sizeof(table), &resolved
        ) ||
        resolved.id_count != 3u ||
        resolved.ids[0] != UINT32_C(0x11111111) ||
        resolved.ids[1] != UINT32_C(0x22222222) ||
        resolved.ids[2] != UINT32_C(0x33333333)) {
        return 4;
    }

    plan.zero_terminated_list = false;
    if (stf_attack_hit_sound_resolve(
            &plan, table, sizeof(table), &resolved
        )) {
        return 5;
    }

    memset(table, 0xFF, sizeof(table));
    plan.zero_terminated_list = true;
    plan.source_index = 0u;
    if (stf_attack_hit_sound_resolve(
            &plan, table, sizeof(table), &resolved
        )) {
        return 6;
    }

    return 0;
}
