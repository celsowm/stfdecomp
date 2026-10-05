#ifndef STF_RECOVERED_CRUSH_PART_ROM_VIEW_H
#define STF_RECOVERED_CRUSH_PART_ROM_VIEW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "crush_part_runtime.h"

enum {
    STF_CRUSH_PART_SPIN_TABLE_ADDRESS = 0x000CE340u
};

typedef struct stf_crush_part_rom_view {
    const uint8_t *image;
    size_t image_size;
    uint32_t base_address;
    int16_t spin_table[STF_CRUSH_PART_SPIN_TABLE_COUNT];
} stf_crush_part_rom_view;

/*
 * Resolve word_CE340 from a reconstructed STF data-ROM image.
 *
 * The table contains 16 signed 16-bit spin increments used by
 * efc_crush_parts_set after its low-nibble seed calculation.
 */
bool stf_crush_part_spin_table_resolve_rom(
    stf_crush_part_rom_view *view,
    const int16_t **spin_table,
    size_t *spin_count
);

#endif
