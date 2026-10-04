#ifndef STF_RECOVERED_KABE_PARTS_DAMAGE_H
#define STF_RECOVERED_KABE_PARTS_DAMAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "damage_unit.h"

typedef struct stf_kabe_parts_damage_result {
    uint16_t per_part_damage;
    stf_up_down_damage_result totals;
} stf_kabe_parts_damage_result;

/*
 * Recover kabe_parts_damage:
 *   per_part = floor(damage / 4), narrowed to the 16-bit part accumulator type
 *   add per_part to all 16 +0x1F00 accumulators with 16-bit modular wrap
 *   calc_up_down_damage from the stored post-wrap values
 */
bool stf_kabe_parts_damage_apply_model2(
    uint8_t *fighter,
    size_t fighter_size,
    uint32_t damage,
    stf_kabe_parts_damage_result *result
);

#endif
