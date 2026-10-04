#ifndef STF_RECOVERED_KAMAE_MOTION_ROM_VIEW_H
#define STF_RECOVERED_KAMAE_MOTION_ROM_VIEW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "set_kamae_runtime.h"

typedef struct stf_kamae_motion_rom_view {
    const uint8_t *image;
    size_t image_size;
    uint32_t base_address;
    uint32_t offset_list_address;
    size_t motion_count;
} stf_kamae_motion_rom_view;

/*
 * Resolver compatible with stf_kamae_motion_resolver.
 *
 * The original get_kamae_value performs:
 *   record = offset_list_motions[selector]
 * using an absolute 32-bit pointer table. This view translates that pointer
 * into caller-supplied image bytes without embedding the original data.
 */
bool stf_kamae_motion_resolve_rom(
    uint16_t selector,
    const uint8_t **motion_record,
    size_t *motion_record_size,
    void *user_data
);

#endif
