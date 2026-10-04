#include <stdint.h>
#include <string.h>

#include "collision_ball.h"

int main(void)
{
    uint8_t fighter[STF_COLLISION_BALL_MODEL2_FIGHTER_MIN_SIZE];
    uint8_t positions[STF_COLLISION_BALL_COUNT * STF_COLLISION_BALL_POSITION_SIZE];
    uint8_t radii[STF_COLLISION_BALL_COUNT * sizeof(uint32_t)];
    size_t i = 0u;

    memset(fighter, 0xAA, sizeof(fighter));
    for (i = 0u; i < sizeof(positions); ++i) {
        positions[i] = (uint8_t)(i & 0xFFu);
    }
    for (i = 0u; i < sizeof(radii); ++i) {
        radii[i] = (uint8_t)(0x80u + (i & 0x7Fu));
    }

    if (!stf_collision_balls_apply_model2(
            fighter,
            sizeof(fighter),
            positions,
            sizeof(positions),
            radii,
            sizeof(radii)
        )) {
        return 1;
    }

    for (i = 0u; i < STF_COLLISION_BALL_COUNT; ++i) {
        const uint8_t *record =
            fighter + STF_COLLISION_BALL_MODEL2_OFFSET +
            i * STF_COLLISION_BALL_RECORD_SIZE;
        const uint8_t *position =
            positions + i * STF_COLLISION_BALL_POSITION_SIZE;
        const uint8_t *radius = radii + i * sizeof(uint32_t);

        if (memcmp(record, position, STF_COLLISION_BALL_POSITION_SIZE) != 0) {
            return 2;
        }
        if (memcmp(
                record + STF_COLLISION_BALL_POSITION_SIZE,
                radius,
                sizeof(uint32_t)
            ) != 0) {
            return 3;
        }
    }

    if (fighter[0x100u] != 0xAAu ||
        fighter[STF_COLLISION_BALL_MODEL2_OFFSET - 1u] != 0xAAu) {
        return 4;
    }

    if (stf_collision_balls_apply_model2(
            fighter,
            STF_COLLISION_BALL_MODEL2_FIGHTER_MIN_SIZE - 1u,
            positions,
            sizeof(positions),
            radii,
            sizeof(radii)
        )) {
        return 5;
    }

    if (stf_collision_balls_build(
            fighter + STF_COLLISION_BALL_MODEL2_OFFSET,
            STF_COLLISION_BALL_COUNT * STF_COLLISION_BALL_RECORD_SIZE,
            positions,
            sizeof(positions) - 1u,
            radii,
            sizeof(radii)
        )) {
        return 6;
    }

    if (stf_collision_balls_build(
            fighter + STF_COLLISION_BALL_MODEL2_OFFSET,
            STF_COLLISION_BALL_COUNT * STF_COLLISION_BALL_RECORD_SIZE,
            positions,
            sizeof(positions),
            radii,
            sizeof(radii) - 1u
        )) {
        return 7;
    }

    return 0;
}
