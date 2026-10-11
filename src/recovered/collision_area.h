#ifndef STF_RECOVERED_COLLISION_AREA_H
#define STF_RECOVERED_COLLISION_AREA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "collision_copro_attack.h"
#include "collision_area_distance.h"
#include "collision_area_side.h"
#include "collision_area_opponent.h"

enum {
    STF_COLLISION_AREA_QUERY_WORDS = 6u,
    STF_COLLISION_AREA_RESPONSE_WORDS = 18u,
    STF_COLLISION_AREA_FIGHTER_MIN_SIZE = 0x1A19u,
    STF_COLLISION_AREA_WORKSPACE_MIN_SIZE = 0x128u
};

typedef struct stf_collision_area_query {
    uint32_t words[STF_COLLISION_AREA_QUERY_WORDS];
} stf_collision_area_query;

typedef struct stf_collision_area_result {
    uint32_t plane_644;
    uint32_t plane_64c;
    uint32_t radius_624;
    uint8_t fighter_index;
    bool inverted_phase;
} stf_collision_area_result;

/*
 * Recover area_coli from 0x2DCB0 through the CPU write section immediately
 * before its 0x15802B2B coprocessor query at 0x2DEE8.
 *
 * The request is exactly the FIFO writes from command 0x38007070:
 * opcode, fighter +0x1C, stage_soko, stage_x, adjusted cage_height,
 * fighter id with bit 31 set for phase 3. The three-word FIFO reset/read
 * sequence that precedes this packet belongs to the transport.
 */
bool stf_collision_area_query_build_model2(
    const uint8_t *fighter,
    size_t fighter_size,
    uint32_t stage_soko_bits,
    uint32_t stage_x_bits,
    uint32_t cage_height_bits,
    stf_collision_area_query *query
);

/*
 * Consume the 18 words returned by a *real* Model 2 geometry provider,
 * in original read order. Writes workspace +0x118..+0x124 and the
 * fighter's +0x670/+0x710/+0x660..+0x66F/+0xA58..+0xA67/
 * +0x650..+0x65F/+0x614/+0x674/+0x624/+0x618 plus normalized
 * +0x644/+0x64C and relevant signed plane selectors.
 *
 * force_flat_mode represents the external (also_mode, also_sub_mode) game
 * mode predicate at 0x2DE9C. The caller must establish that predicate.
 * Later TGP commands and angle/side selection are not emulated here.
 */
bool stf_collision_area_response_apply_model2(
    uint8_t *fighter,
    size_t fighter_size,
    uint8_t *workspace,
    size_t workspace_size,
    const uint32_t *response_words,
    size_t response_count,
    bool force_flat_mode,
    stf_collision_area_result *result
);

/*
 * Provider observes one request and supplies exactly one FIFO response.
 * The callback must represent a real device/emulator or a controlled
 * test fixture, not an invented physics approximation.
 */
typedef bool (*stf_collision_area_geometry_fn)(
    void *user_data,
    uint8_t fighter_index,
    const stf_collision_area_query *query,
    uint32_t response_words[STF_COLLISION_AREA_RESPONSE_WORDS]
);

typedef struct stf_collision_copro_area_model2 {
    uint8_t *fighter[2];
    size_t fighter_size[2];
    uint8_t *workspace;
    size_t workspace_size;
    const stf_copro_attack_rom_view *rom_view;
    uint32_t stage_soko_bits;
    uint32_t stage_x_bits;
    uint32_t cage_height_bits;
    bool force_flat_mode;
    stf_collision_area_geometry_fn geometry;
    void *geometry_user_data;
    /* Optional measured 0x15802B2B FIFO query after each area. */
    stf_collision_area_distance_fn distance;
    void *distance_user_data;
    /* Optional 16-byte ROM byte_2E260 table for the side selection tail. */
    const uint8_t *angle_lut;
    size_t angle_lut_size;
    /* Optional pair of original lookup tables for the opponent scan. */
    const uint16_t *table_2e250;
    size_t table_2e250_count;
    const uint16_t *table_2e230;
    size_t table_2e230_count;
} stf_collision_copro_area_model2;

typedef struct stf_collision_copro_area_result {
    stf_copro_attack_flag_result attack[2];
    stf_collision_area_result area[2];
    uint8_t completed_areas;
    uint8_t completed_distances;
    stf_collision_area_plane plane[2];
    uint8_t completed_sides;
    stf_collision_area_side_result side[2];
    uint8_t completed_opponents;
    stf_collision_area_opponent_result opponent[2];
} stf_collision_copro_area_result;

/*
 * Integrate recovered calc_attack_flag for both fighters, coli_ram_init,
 * then both area_coli query/response prefix calls in exact fighter order.
 * The original 0x1F003E3E and 0x1D003A3A FIFO commands happen before this
 * integration seam and must be supplied by the caller.
 *
 * When a distance provider is given, the recovered +0x680 scalar query
 * follows each area response before the next fighter is processed.
 * An optional angle LUT adds the selected X/Z orientation and minimum
 * collision-plane scan after the scalar distance. No device data is invented.
 * The optional pair of word_2E250/word_2E230 tables closes the second
 * (opponent) plane scan before moving to the next fighter. Consequently
 * fighter0 reads fighter1 previous-frame planes, not its updated planes.
 * Static buffers and ROM lookups are validated before any writes.
 * Callback failure is fail-closed but *not* transactional; completed_areas
 * records observable partial progress.
 */
bool stf_collision_copro_area_stage_model2(
    const stf_collision_copro_area_model2 *model,
    stf_collision_copro_area_result *result
);

#endif
