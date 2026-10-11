#include "collision_area_distance.h"
#include <math.h>
#include <string.h>

static uint32_t read_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) |
        ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}
static void write_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8u);
    p[2] = (uint8_t)(v >> 16u); p[3] = (uint8_t)(v >> 24u);
}
static float bits_float(uint32_t word)
{
    float value;
    memcpy(&value, &word, sizeof(value));
    return value;
}
static uint32_t float_bits(float value)
{
    uint32_t word;
    memcpy(&word, &value, sizeof(word));
    return word;
}

bool stf_collision_area_distance_query_build_model2(
    const uint8_t *fighter,
    size_t fighter_size,
    stf_collision_area_distance_query *query
)
{
    unsigned i;
    if (fighter == NULL || query == NULL ||
        fighter_size < STF_COLLISION_AREA_DISTANCE_FIGHTER_MIN_SIZE ||
        fighter[4u] > 1u) {
        return false;
    }
    query->words[0] = UINT32_C(0x15802B2B);
    for (i=0u; i<3u; ++i) {
        query->words[1u+i] = read_u32(fighter+0x1F4u+i*4u);
        query->words[4u+i] = 0u;
    }
    return true;
}

bool stf_collision_area_distance_response_apply_model2(
    uint8_t *fighter,
    size_t fighter_size,
    uint32_t response_bits
)
{
    const float distance = bits_float(response_bits);
    if (fighter == NULL ||
        fighter_size < STF_COLLISION_AREA_DISTANCE_FIGHTER_MIN_SIZE ||
        !isfinite(distance) || distance < 0.0f) {
        return false;
    }
    write_u32(fighter+0x680u,response_bits);
    return true;
}

bool stf_collision_area_distance_semantic_bits(
    const stf_collision_area_distance_query *query,
    uint32_t *distance_bits
)
{
    float a[3], b[3];
    float dx, dy, dz, distance;
    unsigned i;
    if (query == NULL || distance_bits == NULL ||
        query->words[0] != UINT32_C(0x15802B2B)) {
        return false;
    }
    for (i=0u;i<3u;++i) {
        a[i]=bits_float(query->words[1u+i]);
        b[i]=bits_float(query->words[4u+i]);
        if (!isfinite(a[i]) || !isfinite(b[i])) {
            return false;
        }
    }
    dx=a[0]-b[0];
    dy=a[1]-b[1];
    dz=a[2]-b[2];
    distance=sqrtf(dx*dx+dy*dy+dz*dz);
    if (!isfinite(distance)) {
        return false;
    }
    *distance_bits=float_bits(distance);
    return true;
}

bool stf_collision_area_plane_choose_model2(
    const uint8_t *fighter,
    size_t fighter_size,
    stf_collision_area_plane *plane
)
{
    uint32_t x,z;
    if (fighter == NULL || plane == NULL || fighter_size < 0x650u) {
        return false;
    }
    x = read_u32(fighter + 0x644u) & UINT32_C(0x7FFFFFFF);
    z = read_u32(fighter + 0x64Cu) & UINT32_C(0x7FFFFFFF);
    /*
     * 0x2DF18 compares positive float magnitudes with cmpr/bl:
     * if X is nonzero and (Z is zero or X >= Z), take X;
     * otherwise take Z if it is nonzero and Z >= X.
     * When both are zero, no side is chosen. Equal magnitudes favor X.
     */
    if (x != 0u && (z == 0u || x >= z)) {
        *plane = STF_COLLISION_AREA_PLANE_X;
    } else if (z != 0u && z >= x) {
        *plane = STF_COLLISION_AREA_PLANE_Z;
    } else {
        *plane = STF_COLLISION_AREA_PLANE_NONE;
    }
    return true;
}
