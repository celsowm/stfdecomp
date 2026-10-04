#include <stdint.h>
#include <string.h>

#include "attack_hit_sound_rom_view.h"

static void write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

int main(void)
{
    uint8_t image[0x400];
    stf_attack_hit_sound_plan plan;
    stf_attack_hit_sound_rom_view view;
    stf_attack_hit_sound_resolved resolved;

    memset(image, 0, sizeof(image));
    memset(&plan, 0, sizeof(plan));
    memset(&view, 0, sizeof(view));

    view.image = image;
    view.image_size = sizeof(image);
    view.base_address = UINT32_C(0x000DB600);
    view.use_schamp_addresses = false;

    plan.kind = STF_ATTACK_HIT_SOUND_SINGLE;
    plan.source = STF_ATTACK_HIT_SOUND_SOURCE_DWORD_DB6F4;
    plan.source_index = UINT32_C(1);

    write_le32(image + (0xDB6F4u - 0xDB600u) + 0u, UINT32_C(0x111));
    write_le32(image + (0xDB6F4u - 0xDB600u) + 4u, UINT32_C(0x222));

    if (!stf_attack_hit_sound_resolve_rom(&plan, &view, &resolved) ||
        resolved.id_count != 1u ||
        resolved.ids[0] != UINT32_C(0x222)) {
        return 1;
    }

    view.use_schamp_addresses = true;
    view.base_address = UINT32_C(0x000DB800);
    write_le32(image + (0xDB82Cu - 0xDB800u) + 0u, UINT32_C(0x333));
    write_le32(image + (0xDB82Cu - 0xDB800u) + 4u, UINT32_C(0x444));

    if (!stf_attack_hit_sound_resolve_rom(&plan, &view, &resolved) ||
        resolved.id_count != 1u ||
        resolved.ids[0] != UINT32_C(0x444)) {
        return 2;
    }

    plan.kind = STF_ATTACK_HIT_SOUND_NONE;
    if (!stf_attack_hit_sound_resolve_rom(&plan, &view, &resolved) ||
        resolved.id_count != 0u) {
        return 3;
    }

    return 0;
}
