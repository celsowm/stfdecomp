#ifndef STF_RECOVERED_COLLISION_COPRO_ATTACK_H
#define STF_RECOVERED_COLLISION_COPRO_ATTACK_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_COPRO_ATTACK_FLAG_FIGHTER_MIN_SIZE = 0x1D1Cu,
    STF_COPRO_ATTACK_SELECTOR_COUNT = 32u,
    STF_COPRO_ATTACK_INACTIVE_KIND = 41u
};

/*
 * Read-only view of caller-owned Model 2 addressable data. The profile
 * table is dword_CE020 in the original program; the per-fighter selector
 * table is addressed through fighter +0x1D18. No ROM bytes are embedded.
 */
typedef struct stf_copro_attack_rom_view {
    const uint8_t *image;
    size_t image_size;
    uint32_t image_base;
    uint32_t profile_table_address;
    size_t profile_count;
} stf_copro_attack_rom_view;

typedef struct stf_copro_attack_flag_result {
    uint32_t attack_profile_0ac8;
    uint32_t attack_units_0768;
    uint8_t kind_0820;
    bool eligible;
} stf_copro_attack_flag_result;

/* The original calc_attack_flag body, without writes to fighter memory. */
bool stf_calc_attack_flag_read_model2(
    const uint8_t *fighter,
    size_t fighter_size,
    const stf_copro_attack_rom_view *view,
    stf_copro_attack_flag_result *result
);

/* Apply exactly the stores at fighter +0xAC8 and +0x768. */
bool stf_calc_attack_flag_apply_model2(
    uint8_t *fighter,
    size_t fighter_size,
    const stf_copro_attack_rom_view *view,
    stf_copro_attack_flag_result *result
);

/*
 * Compose calc_attack_flag(fighter0), calc_attack_flag(fighter1),
 * coli_ram_init(fighter0, fighter1). Requires the original leading
 * coprocessor 0x1F003E3E command to have completed first, and remains
 * separate from later area_coli/decide_coli_kind/TGP commands.
 * Both reads and sizes are prevalidated before the first state mutation.
 */
bool stf_collision_copro_attack_prepare_model2(
    uint8_t *workspace,
    size_t workspace_size,
    uint8_t *fighter0,
    size_t fighter0_size,
    uint8_t *fighter1,
    size_t fighter1_size,
    const stf_copro_attack_rom_view *view,
    stf_copro_attack_flag_result results[2]
);

#endif
