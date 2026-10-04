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
 *   per_part = damage / 4
 *   add per_part to all 16 +0x1F00 accumulators
 *   calc_up_down_damage
 */
bool stf_kabe_parts_damage_apply_model2(
    uint8_t *fighter,
    size_t fighter_size,
    uint32_t damage,
    stf_kabe_parts_damage_result *result
);

#endif
