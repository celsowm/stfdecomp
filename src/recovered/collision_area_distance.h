#ifndef STF_RECOVERED_COLLISION_AREA_DISTANCE_H
#define STF_RECOVERED_COLLISION_AREA_DISTANCE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_COLLISION_AREA_DISTANCE_WORDS = 7u,
    STF_COLLISION_AREA_DISTANCE_FIGHTER_MIN_SIZE = 0x684u
};

typedef struct stf_collision_area_distance_query {
    uint32_t words[STF_COLLISION_AREA_DISTANCE_WORDS];
} stf_collision_area_distance_query;

typedef enum stf_collision_area_plane {
    STF_COLLISION_AREA_PLANE_NONE = 0,
    STF_COLLISION_AREA_PLANE_X = 1,
    STF_COLLISION_AREA_PLANE_Z = 2
} stf_collision_area_plane;

/*
 * area_coli 0x2DEE8: the 0x15802B2B request sends fighter +0x1F4
 * (XYZ) against the all-zero XYZ origin. The single FIFO reply is
 * written verbatim to fighter +0x680.
 */
bool stf_collision_area_distance_query_build_model2(
    const uint8_t *fighter,
    size_t fighter_size,
    stf_collision_area_distance_query *query
);
bool stf_collision_area_distance_response_apply_model2(
    uint8_t *fighter,
    size_t fighter_size,
    uint32_t response_bits
);

/*
 * Semantic 3D distance helper for host experiments. It is not bit-exact to
 * the original Model 2 coprocessor's internal rounding.
 */
bool stf_collision_area_distance_semantic_bits(
    const stf_collision_area_distance_query *query,
    uint32_t *distance_bits
);

/*
 * Expose the recovered CPU branch choice between X, Z and no side from
 * fighter +0x644/+0x64C (0x2DF18..0x2DFA8). This does not synthesize the
 * later angle LUT or other TGP calls.
 */
bool stf_collision_area_plane_choose_model2(
    const uint8_t *fighter,
    size_t fighter_size,
    stf_collision_area_plane *plane
);

typedef bool (*stf_collision_area_distance_fn)(
    void *user_data,
    uint8_t fighter_index,
    const stf_collision_area_distance_query *query,
    uint32_t *response_bits
);
#endif
