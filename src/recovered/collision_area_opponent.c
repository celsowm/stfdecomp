#include "collision_area_opponent.h"

#include <math.h>
#include <string.h>

static uint32_t read32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8u) |
           ((uint32_t)data[2] << 16u) | ((uint32_t)data[3] << 24u);
}
static void write16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
}
static void write32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}
static float as_float(uint32_t bits)
{
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

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
)
{
    stf_collision_area_opponent_result local;
    float current_min;
    uint32_t selected_bits, angle, pair_index, orientation_bit, table_index;
    unsigned i;

    if (fighter == NULL || opponent == NULL || workspace == NULL ||
        table_2e250 == NULL || table_2e230 == NULL || result == NULL ||
        fighter_size < STF_COLLISION_AREA_OPPONENT_FIGHTER_MIN_SIZE ||
        opponent_size < STF_COLLISION_AREA_OPPONENT_FIGHTER_MIN_SIZE ||
        workspace_size < STF_COLLISION_AREA_OPPONENT_WORKSPACE_MIN_SIZE ||
        table_2e250_count < STF_COLLISION_AREA_TABLE_2E250_COUNT ||
        table_2e230_count < STF_COLLISION_AREA_TABLE_2E230_COUNT ||
        fighter[4u] > 1u || opponent[4u] != (uint8_t)(1u-fighter[4u])) {
        return false;
    }
    memset(&local, 0, sizeof(local));
    local.fighter_index = fighter[4u];
    local.opponent_index = opponent[4u];

    selected_bits = read32(opponent + 0x650u);
    current_min = as_float(selected_bits);
    if (!isfinite(current_min)) {
        return false;
    }
    /* 0x2E0AC..0x2E0CC: in-place minimum; equal values take later index. */
    for (i=1u; i<4u; ++i) {
        const uint32_t candidate_bits = read32(opponent+0x650u+i*4u);
        const float candidate = as_float(candidate_bits);
        if (!isfinite(candidate)) {
            return false;
        }
        if (!(current_min < candidate)) {
            current_min = candidate;
            selected_bits = candidate_bits;
            local.minimum_plane_index = (uint8_t)i;
        }
    }

    /* i960 for player 1: (min_index & 2) | ((~min_index) & 1). */
    pair_index = local.minimum_plane_index;
    if (local.fighter_index == 1u) {
        pair_index = (pair_index & 2u) | ((~pair_index) & 1u);
    }
    angle = read32(workspace+0x1E0u);
    local.angle_1e0 = angle;
    local.angle_table_index = (uint8_t)(angle >> 13u);
    if ((angle >> 13u) >= STF_COLLISION_AREA_TABLE_2E250_COUNT) {
        return false;
    }

    orientation_bit = ((angle ^ (angle << 1u)) >> 12u) & 4u;
    table_index = pair_index | orientation_bit |
        table_2e250[local.angle_table_index];
    if (table_index >= STF_COLLISION_AREA_TABLE_2E230_COUNT) {
        return false;
    }
    local.raw_direction_index = (uint8_t)pair_index;
    local.minimum_plane_bits = selected_bits;
    local.table_2e230_index = (uint8_t)table_index;
    local.kind_200 = table_2e230[table_index];

    /* No writes until both table lookups and all source floats are valid. */
    write32(workspace+0x204u+4u*local.fighter_index, selected_bits);
    write16(workspace+0x1FCu+2u*local.fighter_index,
            (uint16_t)table_index);
    write16(workspace+0x200u+2u*local.fighter_index,local.kind_200);
    *result=local;
    return true;
}
