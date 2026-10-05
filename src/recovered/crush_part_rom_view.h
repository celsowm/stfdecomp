#ifndef STF_RECOVERED_CRUSH_PART_ROM_VIEW_H
#define STF_RECOVERED_CRUSH_PART_ROM_VIEW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "crush_part_runtime.h"

enum {
    STF_CRUSH_PART_SPIN_TABLE_ADDRESS = 0x000CE340u,
    STF_CRUSH_PART_SPEED_RADIAL_SFIGHT = 0x00034A4Cu,
    STF_CRUSH_PART_SPEED_VERTICAL_SFIGHT = 0x00034A64u,
    STF_CRUSH_PART_SPEED_ANGLE_SFIGHT = 0x00034A7Cu,
    STF_CRUSH_PART_SPEED_JITTER_SFIGHT = 0x00034A9Cu,
    STF_CRUSH_PART_SPEED_RADIAL_SCHAMP = 0x00034A78u,
    STF_CRUSH_PART_SPEED_VERTICAL_SCHAMP = 0x00034A90u,
    STF_CRUSH_PART_SPEED_ANGLE_SCHAMP = 0x00034AA8u,
    STF_CRUSH_PART_SPEED_JITTER_SCHAMP = 0x00034AC8u
};

typedef enum stf_crush_part_program_variant {
    STF_CRUSH_PART_PROGRAM_SFIGHT = 0,
    STF_CRUSH_PART_PROGRAM_SCHAMP = 1
} stf_crush_part_program_variant;

typedef struct stf_crush_part_rom_view {
    const uint8_t *image;
    size_t image_size;
    uint32_t base_address;
    int16_t spin_table[STF_CRUSH_PART_SPIN_TABLE_COUNT];
    stf_crush_part_speed_tables speed_tables;
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

/*
 * Resolve efc_crushpts_speed_cont's four program-resident tables.
 *
 * Sonic Championship carries the same 0x90-byte table block relocated by
 * +0x2C from the sfight program addresses.
 */
bool stf_crush_part_speed_tables_resolve_rom(
    stf_crush_part_rom_view *view,
    stf_crush_part_program_variant variant,
    const stf_crush_part_speed_tables **tables
);

#endif
