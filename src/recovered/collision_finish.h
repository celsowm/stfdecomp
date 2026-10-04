#ifndef STF_RECOVERED_COLLISION_FINISH_H
#define STF_RECOVERED_COLLISION_FINISH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_COLLISION_FINISH_FIGHTER_MODEL2_MIN_SIZE = 0x1A19u,
    STF_COLLISION_FINISH_OPPONENT_MODEL2_MIN_SIZE = 0x7D3u
};

/*
 * Evidence-preserving representation of finish_val_cont.
 *
 * The original routine advances byte 0x1A18 in the fighter workspace through
 * phases 0 -> 1 -> 2 -> 3. Field names remain offset based until their gameplay
 * meaning is established independently.
 */
typedef struct stf_collision_finish_inputs {
    uint8_t phase_1a18;
    uint32_t field_710_bits;
    uint8_t opponent_field_7d2;
    uint8_t field_a29;
} stf_collision_finish_inputs;

uint8_t stf_collision_finish_next_phase(
    const stf_collision_finish_inputs *inputs
);

/*
 * Apply only the recovered finish_val_cont phase transition to Model 2 fighter
 * workspaces. All bytes except fighter[0x1A18] are preserved.
 */
bool stf_collision_finish_apply_model2(
    uint8_t *fighter,
    size_t fighter_size,
    const uint8_t *opponent,
    size_t opponent_size
);

#endif
