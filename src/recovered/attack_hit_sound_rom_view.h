#ifndef STF_RECOVERED_ATTACK_HIT_SOUND_ROM_VIEW_H
#define STF_RECOVERED_ATTACK_HIT_SOUND_ROM_VIEW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attack_hit_sound.h"
#include "attack_hit_sound_runtime.h"

typedef struct stf_attack_hit_sound_rom_view {
    const uint8_t *image;
    size_t image_size;
    uint32_t base_address;
    bool use_schamp_addresses;
} stf_attack_hit_sound_rom_view;

/*
 * Resolve a sound plan directly from a caller-supplied addressable image.
 *
 * The table base is selected from stf_attack_hit_sound_source_info_get()
 * using sfight or Sonic Championship addresses, translated relative to
 * base_address, then consumed by stf_attack_hit_sound_resolve().
 */
bool stf_attack_hit_sound_resolve_rom(
    const stf_attack_hit_sound_plan *plan,
    const stf_attack_hit_sound_rom_view *view,
    stf_attack_hit_sound_resolved *result
);

#endif
