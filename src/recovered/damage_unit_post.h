#ifndef STF_RECOVERED_DAMAGE_UNIT_POST_H
#define STF_RECOVERED_DAMAGE_UNIT_POST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "damage_unit.h"

typedef struct stf_damage_unit_post_result {
    bool crush_table_inert;
    bool early_mode_return;
    bool request_particle_setup;
    uint16_t effect_flags_908;
    uint16_t defender_75c;
    uint16_t defender_75e;
    uint32_t defender_flags_7f0;
    uint32_t defender_flags_0;
} stf_damage_unit_post_result;

/*
 * Recover the STF-specific post-calc_up_down_damage tail of damage_unit.
 *
 * In this program every ptr_CE1CC character entry points to word_CE1CC and
 * all 16 8-byte crush records at its head are zero, so the crush loop cannot
 * call efc_crush_parts_put_cont. The helper therefore recovers the observable
 * CPU state that follows that inert loop.
 */
bool stf_damage_unit_post_apply_model2(
    uint8_t *defender,
    size_t defender_size,
    const uint8_t *workspace,
    size_t workspace_size,
    stf_damage_unit_effect_state *effect,
    uint8_t also_mode,
    uint8_t also_sub_mode,
    stf_damage_unit_post_result *result
);

#endif
