#ifndef STF_RECOVERED_HIT_MOTION_ROM_VIEW_H
#define STF_RECOVERED_HIT_MOTION_ROM_VIEW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attack_hit_motion_select.h"

enum {
    STF_HIT_MOTION_TABLE_WORD_COUNT = 44u
};

typedef struct stf_hit_motion_rom_view {
    const uint8_t *image;
    size_t image_size;
    uint32_t base_address;
    uint32_t character_table_address;
    size_t character_count;
    size_t selector_count;
    uint8_t character;
    uint32_t scratch_words[STF_HIT_MOTION_TABLE_WORD_COUNT];
} stf_hit_motion_rom_view;

/*
 * Resolver compatible with stf_hit_motion_table_resolver.
 *
 * Reproduces the original chain:
 *   ptr_DA0B4[character] -> selector table
 *   selector_table[hit_selector] -> 44-word hit-motion row
 *
 * The selected row is copied into aligned scratch storage so callers may use
 * the existing uint32_t-table selector API without depending on ROM alignment.
 */
bool stf_hit_motion_resolve_rom(
    uint8_t table_selector,
    const uint32_t **table_words,
    size_t *table_word_count,
    void *user_data
);

#endif
