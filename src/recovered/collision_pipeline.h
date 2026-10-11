#ifndef STF_RECOVERED_COLLISION_PIPELINE_H
#define STF_RECOVERED_COLLISION_PIPELINE_H

#include <stdbool.h>
#include <stdint.h>

#include "collision_area.h"
#include "collision_combat_runtime.h"
#include "collision_frame.h"

/*
 * Compose the original collision corridor, after finish_val_cont,
 * rob_ball_data_make and support_rob_position have executed:
 *
 *   coli_cont_cop (area/collision kind stage)
 *   no_coli_unit_set(fighter0, fighter1)
 *   no_coli_unit_set(fighter1, fighter0)
 *   coli_attack_chk(fighter0, fighter1)
 *   coli_attack_chk(fighter1, fighter0)
 *   attack_hit + damage_unit for each ordered successful hit.
 *
 * The same fighter and workspace allocations MUST be used by all three
 * existing typed APIs. Buffers, required providers and the combat
 * continuation are validated before the first mutation.
 *
 * This is not a device emulator: area and distance coprocessor callbacks
 * remain authoritative; attack_hit still needs its recovered continuation
 * callback. If any callback fails, earlier writes remain visible and the
 * stage counters show the last completed boundary.
 */
typedef struct stf_collision_pipeline_model2 {
    const stf_collision_copro_area_model2 *area;
    const stf_collision_frame_model2 *frame;
    const stf_collision_combat_model2 *combat;
    uint32_t stage_floor_bits;
    uint32_t stage_extent_bits;
    uint32_t no_coli_low_bits;
} stf_collision_pipeline_model2;

typedef struct stf_collision_pipeline_result {
    stf_collision_copro_area_result area;
    uint8_t completed_area;
    uint8_t completed_suppressions;
    uint16_t suppression_mask[2];
    stf_collision_frame_result frame;
    bool completed_checks;
    stf_collision_combat_result combat;
    bool completed_combat;
} stf_collision_pipeline_result;

bool stf_collision_pipeline_apply_model2(
    const stf_collision_pipeline_model2 *pipeline,
    stf_collision_pipeline_result *result
);

#endif
