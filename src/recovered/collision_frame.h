#ifndef STF_RECOVERED_COLLISION_FRAME_H
#define STF_RECOVERED_COLLISION_FRAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "collision_attack.h"
#include "collision_attack_effect.h"
#include "collision_hit_order.h"

/*
 * Composes the two coli_attack_chk calls in original order (0 then 1)
 * followed by collision's priority selection. Caller owns the data inputs:
 * attack profiles from dword_CE020, mapping at 0x90F600 and r5-selected
 * impact-position vectors. Suppression masks must already be computed by
 * no_coli_unit_set; full collision/TGP and attack_hit are outside this scope.
 *
 * Every output buffer is prevalidated before the first state mutation. Both
 * successful checks may write the same effect/global hit destination; fighter
 * 1 is last in that case, exactly as in the assembly.
 */
typedef struct stf_collision_frame_model2 {
    uint8_t *fighters[2];
    size_t fighter_sizes[2];
    uint8_t *workspace;
    size_t workspace_size;
    uint32_t attack_profile_bits[2];
    const uint16_t *mapping;
    const uint8_t *r5_selected_xyz[2];
    size_t r5_selected_xyz_sizes[2];
    uint8_t *effect;
    size_t effect_size;
    uint8_t *copro_hit_xyz;
    size_t copro_hit_xyz_size;
} stf_collision_frame_model2;

typedef struct stf_collision_frame_result {
    stf_collision_attack_result checks[2];
    stf_collision_hit_order order;
} stf_collision_frame_result;

bool stf_collision_frame_check_model2(
    const stf_collision_frame_model2 *frame,
    stf_collision_frame_result *result
);

/*
 * The returned order selects which attacker gets attack_hit then damage_unit.
 * The callback performs that pair of calls; this function cannot make the
 * target backend itself portable. False stops the sequence and reports how
 * many callbacks were completed. Reject malformed event lists.
 */
typedef bool (*stf_collision_attack_dispatch_fn)(
    void *user_data,
    uint8_t attacker_index,
    uint8_t defender_index
);

bool stf_collision_frame_dispatch(
    const stf_collision_hit_order *order,
    stf_collision_attack_dispatch_fn callback,
    void *user_data,
    size_t *completed
);

#endif
