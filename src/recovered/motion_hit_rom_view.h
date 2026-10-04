#ifndef STF_RECOVERED_MOTION_HIT_ROM_VIEW_H
#define STF_RECOVERED_MOTION_HIT_ROM_VIEW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "motion_hit_table.h"

typedef struct stf_motion_hit_rom_view {
    const uint8_t *image;
    size_t image_size;
    uint32_t base_address;
    uint32_t animation_related_address;
    size_t animation_count;
} stf_motion_hit_rom_view;

/*
 * ROM-backed calc_mht_adr adapter.
 *
 * The original animation_related table stores absolute 32-bit addresses.
 * This helper reads the selected pointer, converts it to an offset relative
 * to base_address, and scans the motion stream using the recovered canonical
 * record strides.
 */
stf_motion_hit_lookup_status stf_motion_hit_rom_find(
    const stf_motion_hit_rom_view *view,
    uint32_t selector,
    uint8_t target_tag,
    uint32_t *record_address
);

#endif
