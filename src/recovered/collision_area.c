#include "collision_area.h"

#include "collision_ram_init.h"

#include <math.h>
#include <string.h>

static uint32_t read_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) |
           ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}

static void write_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8u);
    p[2] = (uint8_t)(v >> 16u);
    p[3] = (uint8_t)(v >> 24u);
}

static float bits_float(uint32_t v)
{
    float f;
    memcpy(&f, &v, sizeof(f));
    return f;
}

static uint32_t float_bits(float f)
{
    uint32_t v;
    memcpy(&v, &f, sizeof(v));
    return v;
}

bool stf_collision_area_query_build_model2(
    const uint8_t *fighter,
    size_t fighter_size,
    uint32_t stage_soko_bits,
    uint32_t stage_x_bits,
    uint32_t cage_height_bits,
    stf_collision_area_query *query
)
{
    float adjusted;
    const uint8_t phase = fighter != NULL && fighter_size > 0x1A18u
        ? fighter[0x1A18u] : 0u;
    if (fighter == NULL || query == NULL ||
        fighter_size < STF_COLLISION_AREA_FIGHTER_MIN_SIZE ||
        fighter[4u] > 1u) {
        return false;
    }
    adjusted = bits_float(cage_height_bits) - bits_float(UINT32_C(0x3E428F5C));
    /* cmpoble 2,phase skips the extra 10 when phase >= 2. */
    if (phase < 2u &&
        (read_u32(fighter + 0x70Cu) & (UINT32_C(1) << 9u)) == 0u) {
        adjusted += 10.0f;
    }
    if (!isfinite(adjusted)) {
        return false;
    }

    query->words[0] = UINT32_C(0x38007070);
    query->words[1] = read_u32(fighter + 0x1Cu);
    query->words[2] = stage_soko_bits;
    query->words[3] = stage_x_bits;
    query->words[4] = float_bits(adjusted);
    query->words[5] = (uint32_t)fighter[4u] |
        (phase == 3u ? UINT32_C(0x80000000) : 0u);
    return true;
}

bool stf_collision_area_response_apply_model2(
    uint8_t *fighter,
    size_t fighter_size,
    uint8_t *workspace,
    size_t workspace_size,
    const uint32_t *response_words,
    size_t response_count,
    bool force_flat_mode,
    stf_collision_area_result *result
)
{
    stf_collision_area_result local;
    uint32_t tangent[4];
    float sum0, sum1;
    uint8_t phase;
    unsigned i;

    if (fighter == NULL || workspace == NULL || response_words == NULL ||
        fighter_size < STF_COLLISION_AREA_FIGHTER_MIN_SIZE ||
        workspace_size < STF_COLLISION_AREA_WORKSPACE_MIN_SIZE ||
        response_count < STF_COLLISION_AREA_RESPONSE_WORDS ||
        fighter[4u] > 1u) {
        return false;
    }

    phase = fighter[0x1A18u];
    for (i = 0u; i < 4u; ++i) {
        tangent[i] = response_words[6u + i];
    }
    if (phase == 3u) {
        for (i = 0u; i < 4u; ++i) {
            tangent[i] = response_words[10u + i];
        }
        tangent[0] ^= UINT32_C(0x80000000);
        tangent[2] ^= UINT32_C(0x80000000);
    } else {
        tangent[1] ^= UINT32_C(0x80000000);
        tangent[3] ^= UINT32_C(0x80000000);
    }
    /* ac_make_vect_outset normalizes all exact -0 bit patterns to +0. */
    for (i = 0u; i < 4u; ++i) {
        if (tangent[i] == UINT32_C(0x80000000)) {
            tangent[i] = 0u;
        }
        if (!isfinite(bits_float(tangent[i]))) {
            return false;
        }
    }
    sum0 = bits_float(tangent[0]) + bits_float(tangent[1]);
    sum1 = bits_float(tangent[2]) + bits_float(tangent[3]);
    if (!isfinite(sum0) || !isfinite(sum1)) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    local.fighter_index = fighter[4u];
    local.inverted_phase = phase == 3u;

    for (i = 0u; i < 4u; ++i) {
        write_u32(workspace + 0x118u + i*4u, response_words[i]);
    }
    write_u32(fighter + 0x670u, response_words[4u]);
    write_u32(fighter + 0x710u, response_words[5u]);
    for (i = 0u; i < 4u; ++i) {
        write_u32(fighter + 0xA58u + i*4u, response_words[10u + i]);
        write_u32(fighter + 0x650u + i*4u, response_words[14u + i]);
        write_u32(fighter + 0x660u + i*4u, tangent[i]);
    }
    write_u32(fighter + 0x614u, response_words[14u]);
    write_u32(fighter + 0x674u, response_words[15u]);
    write_u32(fighter + 0x624u,
        fighter[0xA29u] == 0u ? 0u : response_words[16u]);
    write_u32(fighter + 0x618u, response_words[17u]);

    local.plane_644 = force_flat_mode ? 0u : float_bits(sum0);
    local.plane_64c = force_flat_mode ? 0u : float_bits(sum1);
    local.radius_624 = read_u32(fighter + 0x624u);

    write_u32(fighter + 0x644u, local.plane_644);
    write_u32(fighter + 0x64Cu, local.plane_64c);
    if (local.plane_644 != 0u) {
        write_u32(fighter + 0x650u +
            ((local.plane_644 >> 31u) * 4u), 0u);
    }
    if (local.plane_64c != 0u) {
        write_u32(fighter + 0x658u +
            ((local.plane_64c >> 31u) * 4u), 0u);
    }

    if (result != NULL) {
        *result = local;
    }
    return true;
}

bool stf_collision_copro_area_stage_model2(
    const stf_collision_copro_area_model2 *model,
    stf_collision_copro_area_result *result
)
{
    stf_collision_copro_area_result local;
    stf_collision_area_query requests[2];
    stf_copro_attack_flag_result computed[2];
    uint32_t response[STF_COLLISION_AREA_RESPONSE_WORDS];
    stf_collision_area_distance_query distance_query;
    uint32_t distance_bits;
    uint8_t i;

    if (model == NULL || result == NULL || model->geometry == NULL ||
        model->rom_view == NULL || model->workspace == NULL ||
        model->fighter[0] == NULL || model->fighter[1] == NULL ||
        model->fighter_size[0] < STF_COPRO_ATTACK_FLAG_FIGHTER_MIN_SIZE ||
        model->fighter_size[1] < STF_COPRO_ATTACK_FLAG_FIGHTER_MIN_SIZE ||
        model->fighter[0][4u] != 0u || model->fighter[1][4u] != 1u ||
        model->workspace_size < STF_COLLISION_RAM_INIT_WORKSPACE_MIN_SIZE ||
        (model->angle_lut != NULL &&
         (model->distance == NULL ||
          model->angle_lut_size < STF_COLLISION_AREA_ANGLE_LUT_COUNT ||
          model->workspace_size < STF_COLLISION_AREA_SIDE_WORKSPACE_MIN_SIZE))) {
        return false;
    }
    memset(&local, 0, sizeof(local));
    /* Both queries use unchanged fighter motion state. */
    if (!stf_collision_area_query_build_model2(
            model->fighter[0], model->fighter_size[0],
            model->stage_soko_bits, model->stage_x_bits,
            model->cage_height_bits, &requests[0]) ||
        !stf_collision_area_query_build_model2(
            model->fighter[1], model->fighter_size[1],
            model->stage_soko_bits, model->stage_x_bits,
            model->cage_height_bits, &requests[1])) {
        return false;
    }
    if (!stf_collision_copro_attack_prepare_model2(
            model->workspace, model->workspace_size,
            model->fighter[0], model->fighter_size[0],
            model->fighter[1], model->fighter_size[1],
            model->rom_view, computed)) {
        return false;
    }
    local.attack[0] = computed[0];
    local.attack[1] = computed[1];

    for (i = 0u; i < 2u; ++i) {
        if (!model->geometry(
                model->geometry_user_data,
                i, &requests[i], response)) {
            *result = local;
            return false;
        }
        if (!stf_collision_area_response_apply_model2(
                model->fighter[i], model->fighter_size[i],
                model->workspace, model->workspace_size,
                response, STF_COLLISION_AREA_RESPONSE_WORDS,
                model->force_flat_mode, &local.area[i])) {
            *result = local;
            return false;
        }
        ++local.completed_areas;
        if (model->distance != NULL) {
            if (!stf_collision_area_distance_query_build_model2(
                    model->fighter[i], model->fighter_size[i],
                    &distance_query) ||
                !model->distance(
                    model->distance_user_data, i,
                    &distance_query, &distance_bits) ||
                !stf_collision_area_distance_response_apply_model2(
                    model->fighter[i], model->fighter_size[i],
                    distance_bits) ||
                !stf_collision_area_plane_choose_model2(
                    model->fighter[i], model->fighter_size[i],
                    &local.plane[i])) {
                *result = local;
                return false;
            }
            ++local.completed_distances;
            if (model->angle_lut != NULL) {
                if (!stf_collision_area_side_apply_model2(
                        model->fighter[i], model->fighter_size[i],
                        model->workspace, model->workspace_size,
                        model->angle_lut, model->angle_lut_size,
                        &local.side[i])) {
                    *result = local;
                    return false;
                }
                ++local.completed_sides;
            }
        }
    }
    *result = local;
    return true;
}
