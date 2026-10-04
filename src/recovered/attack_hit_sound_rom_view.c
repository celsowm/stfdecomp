#include "attack_hit_sound_rom_view.h"

bool stf_attack_hit_sound_resolve_rom(
    const stf_attack_hit_sound_plan *plan,
    const stf_attack_hit_sound_rom_view *view,
    stf_attack_hit_sound_resolved *result
)
{
    stf_attack_hit_sound_source_info info;
    uint32_t table_address = 0u;
    size_t table_offset = 0u;

    if (plan == NULL || view == NULL || result == NULL) {
        return false;
    }

    if (plan->kind == STF_ATTACK_HIT_SOUND_NONE) {
        return stf_attack_hit_sound_resolve(plan, NULL, 0u, result);
    }

    if (view->image == NULL ||
        !stf_attack_hit_sound_source_info_get(plan->source, &info)) {
        return false;
    }

    table_address = view->use_schamp_addresses
        ? info.schamp_base_address
        : info.sfight_base_address;

    if (table_address < view->base_address) {
        return false;
    }

    table_offset = (size_t)(table_address - view->base_address);
    if (table_offset >= view->image_size) {
        return false;
    }

    return stf_attack_hit_sound_resolve(
        plan,
        view->image + table_offset,
        view->image_size - table_offset,
        result
    );
}
