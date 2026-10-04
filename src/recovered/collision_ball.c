#include "collision_ball.h"

#include <string.h>

bool stf_collision_balls_build(
    uint8_t *destination,
    size_t destination_size,
    const uint8_t *positions,
    size_t positions_size,
    const uint8_t *radius_words,
    size_t radius_words_size
)
{
    size_t index = 0u;

    if (destination == NULL ||
        positions == NULL ||
        radius_words == NULL ||
        destination_size <
            STF_COLLISION_BALL_COUNT * STF_COLLISION_BALL_RECORD_SIZE ||
        positions_size <
            STF_COLLISION_BALL_COUNT * STF_COLLISION_BALL_POSITION_SIZE ||
        radius_words_size < STF_COLLISION_BALL_COUNT * sizeof(uint32_t)) {
        return false;
    }

    for (index = 0u; index < STF_COLLISION_BALL_COUNT; ++index) {
        uint8_t *record =
            destination + index * STF_COLLISION_BALL_RECORD_SIZE;
        const uint8_t *position =
            positions + index * STF_COLLISION_BALL_POSITION_SIZE;
        const uint8_t *radius =
            radius_words + index * sizeof(uint32_t);

        memcpy(record, position, STF_COLLISION_BALL_POSITION_SIZE);
        memcpy(
            record + STF_COLLISION_BALL_POSITION_SIZE,
            radius,
            sizeof(uint32_t)
        );
    }

    return true;
}

bool stf_collision_balls_apply_model2(
    uint8_t *fighter,
    size_t fighter_size,
    const uint8_t *positions,
    size_t positions_size,
    const uint8_t *radius_words,
    size_t radius_words_size
)
{
    if (fighter == NULL ||
        fighter_size < STF_COLLISION_BALL_MODEL2_FIGHTER_MIN_SIZE) {
        return false;
    }

    return stf_collision_balls_build(
        fighter + STF_COLLISION_BALL_MODEL2_OFFSET,
        fighter_size - STF_COLLISION_BALL_MODEL2_OFFSET,
        positions,
        positions_size,
        radius_words,
        radius_words_size
    );
}
