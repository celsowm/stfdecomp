#ifndef STF_RECOVERED_COLLISION_BALL_H
#define STF_RECOVERED_COLLISION_BALL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_COLLISION_BALL_COUNT = 32u,
    STF_COLLISION_BALL_POSITION_SIZE = 12u,
    STF_COLLISION_BALL_RECORD_SIZE = 16u,
    STF_COLLISION_BALL_MODEL2_OFFSET = 0xD00u,
    STF_COLLISION_BALL_MODEL2_FIGHTER_MIN_SIZE =
        STF_COLLISION_BALL_MODEL2_OFFSET +
        STF_COLLISION_BALL_COUNT * STF_COLLISION_BALL_RECORD_SIZE
};

/*
 * Recovered rob_ball_data_make layout:
 *
 *   destination record +0x00..+0x0B = three 32-bit position words
 *   destination record +0x0C..+0x0F = one 32-bit radius/attribute word
 *
 * The source tables stay external: the original routine selects different
 * Model 2 addresses for fighter 0/1. Portable callers provide the already
 * selected tables instead of embedding arcade addresses.
 */
bool stf_collision_balls_build(
    uint8_t *destination,
    size_t destination_size,
    const uint8_t *positions,
    size_t positions_size,
    const uint8_t *radius_words,
    size_t radius_words_size
);

bool stf_collision_balls_apply_model2(
    uint8_t *fighter,
    size_t fighter_size,
    const uint8_t *positions,
    size_t positions_size,
    const uint8_t *radius_words,
    size_t radius_words_size
);

#endif
