#include "collision_particle_runtime.h"

#include <string.h>

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8u) |
           ((uint32_t)data[2] << 16u) |
           ((uint32_t)data[3] << 24u);
}

static void write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

static void write_triplet(uint8_t *data, const uint32_t value[3])
{
    write_le32(data + 0u, value[0]);
    write_le32(data + 4u, value[1]);
    write_le32(data + 8u, value[2]);
}

static unsigned particle_slot_limit(uint8_t also_sub_mode)
{
    return also_sub_mode == UINT8_C(0x1A) ||
           also_sub_mode == UINT8_C(0x1B)
        ? STF_COLLISION_PARTICLE_EXTENDED_SLOTS
        : STF_COLLISION_PARTICLE_NORMAL_SLOTS;
}

static void clear_consumed_stage(stf_collision_particle_stage *stage)
{
    memset(stage->position_a, 0, sizeof(stage->position_a));
    memset(stage->position_b, 0, sizeof(stage->position_b));
    stage->draw_particle = UINT16_C(0);
}

bool stf_collision_particle_put_model2(
    uint8_t *slot_bytes,
    size_t slot_bytes_size,
    uint8_t also_sub_mode,
    const stf_collision_particle_descriptor
        descriptors[STF_COLLISION_PARTICLE_KIND_COUNT],
    stf_collision_particle_stage *stage,
    stf_collision_particle_result *result
)
{
    stf_collision_particle_result local;
    const unsigned slot_limit = particle_slot_limit(also_sub_mode);
    unsigned index = 0u;

    if (slot_bytes == NULL || descriptors == NULL ||
        stage == NULL || result == NULL ||
        slot_bytes_size <
            (size_t)slot_limit * STF_COLLISION_PARTICLE_SLOT_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    local.slot_limit = (uint8_t)slot_limit;

    if (stage->draw_particle == UINT16_C(0)) {
        *result = local;
        return true;
    }

    if (stage->draw_particle >= STF_COLLISION_PARTICLE_KIND_COUNT) {
        return false;
    }

    local.requested = true;
    local.effect_address =
        descriptors[stage->draw_particle].effect_address;
    local.initial_value =
        descriptors[stage->draw_particle].initial_value;

    for (index = 0u; index < slot_limit; ++index) {
        uint8_t *slot =
            slot_bytes + (size_t)index * STF_COLLISION_PARTICLE_SLOT_SIZE;

        if (read_le32(slot + 0x1Cu) != UINT32_C(0)) {
            continue;
        }

        write_le32(slot + 0x1Cu, local.effect_address);
        write_triplet(slot + 0x00u, stage->position_a);
        write_le32(slot + 0x20u, local.initial_value);
        write_triplet(slot + 0x0Cu, stage->position_b);
        slot[0x19u] = stage->flags;

        local.allocated = true;
        local.slot_index = (uint8_t)index;
        break;
    }

    clear_consumed_stage(stage);
    *result = local;
    return true;
}


static const uint32_t k_flag3_scale_bits[8] = {
    UINT32_C(0x3F800000),
    UINT32_C(0x3FC00000),
    UINT32_C(0x40000000),
    UINT32_C(0x40200000),
    UINT32_C(0x40400000),
    UINT32_C(0x40600000),
    UINT32_C(0x3ECCCCCD),
    UINT32_C(0x40900000),
};

static const stf_collision_particle_descriptor *find_descriptor(
    uint32_t effect_address,
    const stf_collision_particle_descriptor
        descriptors[STF_COLLISION_PARTICLE_KIND_COUNT]
)
{
    unsigned index = 0u;

    for (index = 0u; index < STF_COLLISION_PARTICLE_KIND_COUNT; ++index) {
        if (descriptors[index].effect_address == effect_address) {
            return &descriptors[index];
        }
    }
    return NULL;
}

static void clear_particle_slot(uint8_t *slot)
{
    memset(slot + 0x00u, 0, 12u);
    memset(slot + 0x0Cu, 0, 12u);
    slot[0x18u] = UINT8_C(0);
    slot[0x19u] = UINT8_C(0);
    slot[0x1Au] = UINT8_C(0);
    slot[0x1Bu] = UINT8_C(0);
    write_le32(slot + 0x1Cu, UINT32_C(0));
    write_le32(slot + 0x20u, UINT32_C(0));
}

bool stf_collision_particle_update_model2(
    uint8_t *slot_bytes,
    size_t slot_bytes_size,
    const stf_collision_particle_descriptor
        descriptors[STF_COLLISION_PARTICLE_KIND_COUNT],
    stf_collision_particle_update_result *result
)
{
    stf_collision_particle_update_result local;
    unsigned index = 0u;

    if (slot_bytes == NULL || descriptors == NULL || result == NULL ||
        slot_bytes_size <
            (size_t)STF_COLLISION_PARTICLE_NORMAL_SLOTS *
            STF_COLLISION_PARTICLE_SLOT_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));

    for (index = 0u; index < STF_COLLISION_PARTICLE_NORMAL_SLOTS; ++index) {
        uint8_t *slot =
            slot_bytes + (size_t)index * STF_COLLISION_PARTICLE_SLOT_SIZE;
        const uint32_t effect_address = read_le32(slot + 0x1Cu);
        const stf_collision_particle_descriptor *descriptor = NULL;
        uint8_t age = 0u;
        size_t frame_index = 0u;

        if (effect_address == UINT32_C(0)) {
            continue;
        }

        ++local.active_before;
        descriptor = find_descriptor(effect_address, descriptors);
        if (descriptor == NULL) {
            return false;
        }

        age = slot[0x18u];
        if ((uint16_t)age >= descriptor->duration) {
            clear_particle_slot(slot);
            ++local.released;
            continue;
        }

        if (descriptor->frame_divisor == UINT16_C(0) ||
            descriptor->frames == NULL) {
            return false;
        }

        frame_index = (size_t)age / descriptor->frame_divisor;
        if (frame_index >= descriptor->frame_count) {
            return false;
        }

        slot[0x1Au] = (uint8_t)descriptor->frames[frame_index];
        slot[0x1Bu] = (uint8_t)(descriptor->frames[frame_index] >> 8u);

        if ((slot[0x19u] & (uint8_t)(UINT8_C(1) << 3u)) != 0u) {
            if (age >= 8u) {
                return false;
            }
            write_le32(slot + 0x20u, k_flag3_scale_bits[age]);
        }

        slot[0x18u] = (uint8_t)(age + UINT8_C(1));
        ++local.advanced;
    }

    *result = local;
    return true;
}
