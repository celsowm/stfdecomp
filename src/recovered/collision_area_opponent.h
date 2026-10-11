#ifndef STF_RECOVERED_COLLISION_AREA_OPPONENT_H
#define STF_RECOVERED_COLLISION_AREA_OPPONENT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_COLLISION_AREA_OPPONENT_FIGHTER_MIN_SIZE = 0x660u,
    STF_COLLISION_AREA_OPPONENT_WORKSPACE_MIN_SIZE = 0x20Cu,
    STF_COLLISION_AREA_TABLE_2E250_COUNT = 8u,
    STF_COLLISION_AREA_TABLE_2E230_COUNT = 16u
};

typedef struct stf_collision_area_opponent_result {
    uint8_t fighter_index;
    uint8_t opponent_index;
    uint8_t minimum_plane_index;
    uint32_t minimum_plane_bits;
    uint32_t angle_1e0;
    uint8_t angle_table_index;
    uint8_t raw_direction_index;
    uint8_t table_2e230_index;
    uint16_t kind_200;
} stf_collision_area_opponent_result;

/*
 * Tail of area_coli, loc_2E074..loc_2E0F4:
 * scan the OTHER fighter's +0x650/+0x654/+0x658/+0x65C planes, select
 * the lowest finite float (ties prefer the later plane), then resolve
 * task +0x1E0 through word_2E250[8] and word_2E230[16].
 *
 * The two tables remain explicit caller-provided program-data views.
 * All reads/indices are validated before any of the three writes:
 *   workspace +0x204[fighter_id*4] = opposing minimum float bits;
 *   workspace +0x1FC[fighter_id*2] = composed table index;
 *   workspace +0x200[fighter_id*2] = kind from word_2E230.
 *
 * In the original execution, area_coli(fighter0) scans fighter1
 * BEFORE area_coli(fighter1) refreshes its 0x650 planes. Integrate this
 * function immediately after the active fighter's side processing.
 */
bool stf_collision_area_opponent_apply_model2(
    const uint8_t *fighter,
    size_t fighter_size,
    const uint8_t *opponent,
    size_t opponent_size,
    uint8_t *workspace,
    size_t workspace_size,
    const uint16_t *table_2e250,
    size_t table_2e250_count,
    const uint16_t *table_2e230,
    size_t table_2e230_count,
    stf_collision_area_opponent_result *result
);

#endif
