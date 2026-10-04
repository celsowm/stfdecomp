#include "ring_scatter_runtime.h"

#include "copro_scalar.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

typedef struct stf_ring_local_pattern {
    float position_x;
    float position_z;
    float velocity_x;
    float velocity_z;
} stf_ring_local_pattern;


static const stf_ring_trajectory_info trajectory_info[] = {
    {UINT32_C(0x000AE488), UINT32_C(0x000AE5C0), UINT16_C(99)},
    {UINT32_C(0x000AE618), UINT32_C(0x000AE750), UINT16_C(119)},
    {UINT32_C(0x000AE7F8), UINT32_C(0x000AE930), UINT16_C(139)},
};

static const stf_ring_local_pattern local_patterns[8] = {
    { 0.50f,  0.00f,  0.08f,  0.00f},
    {-0.50f,  0.00f, -0.08f,  0.00f},
    { 0.00f,  0.50f,  0.00f,  0.08f},
    { 0.00f, -0.50f,  0.00f, -0.08f},
    { 0.30f,  0.30f,  0.05f,  0.05f},
    {-0.30f, -0.30f, -0.05f, -0.05f},
    { 0.30f, -0.30f,  0.05f, -0.05f},
    {-0.30f,  0.30f, -0.05f,  0.05f},
};

static const stf_ring_profile_record profile_damage_2[] = {
    {2u, UINT32_C(0x3F000000), STF_RING_TRAJECTORY_A},
};

static const stf_ring_profile_record profile_damage_4[] = {
    {8u, UINT32_C(0x3F4CCCCD), STF_RING_TRAJECTORY_B},
};

static const stf_ring_profile_record profile_damage_8[] = {
    {8u,  UINT32_C(0x3F4CCCCD), STF_RING_TRAJECTORY_C},
    {16u, UINT32_C(0x3F99999A), STF_RING_TRAJECTORY_B},
};

static const stf_ring_profile_record profile_damage_16[] = {
    {8u,  UINT32_C(0x3F800000), STF_RING_TRAJECTORY_C},
    {16u, UINT32_C(0x3FB33333), STF_RING_TRAJECTORY_B},
    {24u, UINT32_C(0x3FE66666), STF_RING_TRAJECTORY_A},
};

static const stf_ring_profile_record profile_special[] = {
    {8u, UINT32_C(0x3FCCCCCD), STF_RING_TRAJECTORY_C},
};

static float bits_to_float(uint32_t bits)
{
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static uint32_t float_to_bits(float value)
{
    uint32_t bits = 0u;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static bool select_from_records(
    const stf_ring_profile_record *records,
    size_t count,
    uint8_t ring_index,
    stf_ring_profile_record *record
)
{
    size_t index = 0u;

    if (records == NULL || record == NULL) {
        return false;
    }

    for (index = 0u; index < count; ++index) {
        if (records[index].max_ring_index >= ring_index) {
            *record = records[index];
            return true;
        }
    }

    return false;
}


bool stf_ring_trajectory_info_get(
    stf_ring_trajectory trajectory,
    stf_ring_trajectory_info *info
)
{
    const unsigned index = (unsigned)trajectory;

    if (info == NULL || index >= (sizeof(trajectory_info) / sizeof(trajectory_info[0]))) {
        return false;
    }

    *info = trajectory_info[index];
    return true;
}

bool stf_ring_trajectory_sample_bits(
    stf_ring_trajectory trajectory,
    const uint32_t *curve_words,
    size_t curve_word_count,
    uint16_t frame,
    uint32_t *sample_bits
)
{
    stf_ring_trajectory_info info;
    float sample = 0.0f;

    if (curve_words == NULL || sample_bits == NULL ||
        !stf_ring_trajectory_info_get(trajectory, &info) ||
        curve_word_count < (size_t)info.sample_count + 1u ||
        curve_words[info.sample_count] != UINT32_C(0xBF800000) ||
        frame >= info.sample_count) {
        return false;
    }

    sample = bits_to_float(curve_words[frame]);
    if (!isfinite(sample) || sample < 0.0f) {
        return false;
    }

    *sample_bits = curve_words[frame];
    return true;
}

bool stf_ring_profile_select(
    stf_ring_scatter_profile profile,
    uint8_t ring_index,
    stf_ring_profile_record *record
)
{
    switch (profile) {
    case STF_RING_SCATTER_DAMAGE_2:
        return select_from_records(
            profile_damage_2,
            sizeof(profile_damage_2) / sizeof(profile_damage_2[0]),
            ring_index,
            record
        );
    case STF_RING_SCATTER_DAMAGE_4:
        return select_from_records(
            profile_damage_4,
            sizeof(profile_damage_4) / sizeof(profile_damage_4[0]),
            ring_index,
            record
        );
    case STF_RING_SCATTER_DAMAGE_8:
        return select_from_records(
            profile_damage_8,
            sizeof(profile_damage_8) / sizeof(profile_damage_8[0]),
            ring_index,
            record
        );
    case STF_RING_SCATTER_DAMAGE_16:
        return select_from_records(
            profile_damage_16,
            sizeof(profile_damage_16) / sizeof(profile_damage_16[0]),
            ring_index,
            record
        );
    case STF_RING_SCATTER_SPECIAL_MOTION:
        return select_from_records(
            profile_special,
            sizeof(profile_special) / sizeof(profile_special[0]),
            ring_index,
            record
        );
    case STF_RING_SCATTER_NONE:
    default:
        return false;
    }
}

bool stf_ring_scatter_spawn(
    const stf_ring_scatter_plan *plan,
    const stf_ring_scatter_spawn_inputs *inputs,
    stf_ring_pool *pool,
    stf_ring_slot slots[STF_RING_POOL_SLOT_COUNT],
    stf_ring_scatter_spawn_result *result
)
{
    stf_ring_scatter_spawn_result local;
    float delta_x = 0.0f;
    float delta_z = 0.0f;
    float base_x = 0.0f;
    float base_y = 0.0f;
    float base_z = 0.0f;
    uint16_t yaw = 0u;
    uint8_t ring_index = 0u;

    if (plan == NULL || inputs == NULL || pool == NULL || slots == NULL ||
        plan->suppressed || plan->ring_count > UINT8_C(16)) {
        return false;
    }

    memset(&local, 0, sizeof(local));

    delta_x =
        bits_to_float(inputs->defender_angle_x_bits) -
        bits_to_float(inputs->attacker_angle_x_bits);
    delta_z =
        bits_to_float(inputs->defender_angle_z_bits) -
        bits_to_float(inputs->attacker_angle_z_bits);
    base_x = bits_to_float(inputs->spawn_x_bits);
    base_y = bits_to_float(inputs->spawn_y_bits);
    base_z = bits_to_float(inputs->spawn_z_bits);

    if (!isfinite(delta_x) || !isfinite(delta_z) ||
        !isfinite(base_x) || !isfinite(base_y) || !isfinite(base_z) ||
        !stf_copro_scalar_atan2_angle_bits(
            float_to_bits(delta_x),
            float_to_bits(delta_z),
            &yaw
        )) {
        return false;
    }

    for (ring_index = 0u; ring_index < plan->ring_count; ++ring_index) {
        const stf_ring_local_pattern *pattern =
            &local_patterns[ring_index & UINT8_C(7)];
        stf_ring_profile_record profile_record;
        uint8_t slot_index = STF_RING_POOL_EMPTY;
        bool recycled = false;
        uint32_t position_x_bits = 0u;
        uint32_t position_z_bits = 0u;
        uint32_t velocity_x_bits = 0u;
        uint32_t velocity_z_bits = 0u;
        float scale = 0.0f;
        stf_ring_slot *slot = NULL;

        if (!stf_ring_profile_select(
                plan->profile,
                ring_index,
                &profile_record
            ) ||
            !stf_ring_pool_allocate_ex(
                pool,
                &slot_index,
                &recycled
            ) ||
            !stf_copro_scalar_rotate_y_xz_bits(
                yaw,
                float_to_bits(pattern->position_x),
                float_to_bits(pattern->position_z),
                &position_x_bits,
                &position_z_bits
            ) ||
            !stf_copro_scalar_rotate_y_xz_bits(
                yaw,
                float_to_bits(pattern->velocity_x),
                float_to_bits(pattern->velocity_z),
                &velocity_x_bits,
                &velocity_z_bits
            )) {
            return false;
        }

        scale = bits_to_float(profile_record.velocity_scale_bits);
        if (!isfinite(scale)) {
            return false;
        }

        slot = &slots[slot_index];
        slot->active = true;
        slot->source_ring_index = ring_index;
        slot->age = 0u;
        slot->x_bits =
            float_to_bits(base_x + bits_to_float(position_x_bits));
        slot->y_bits = float_to_bits(base_y);
        slot->z_bits =
            float_to_bits(base_z + bits_to_float(position_z_bits));
        slot->vx_bits =
            float_to_bits(bits_to_float(velocity_x_bits) * scale);
        slot->vz_bits =
            float_to_bits(bits_to_float(velocity_z_bits) * scale);
        slot->trajectory = profile_record.trajectory;
        slot->visible_from_frame = plan->visible_from_frame;
        slot->expire_at_frame = plan->expire_at_frame;
        slot->drop_mode = plan->drop_mode;

        local.slot_indices[ring_index] = slot_index;
        ++local.spawned_count;
        if (recycled) {
            ++local.recycled_count;
        }
    }

    if (result != NULL) {
        *result = local;
    }

    return true;
}
