#include <stdint.h>
#include <string.h>

#include "collision_particle_runtime.h"

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

static int test_zero_request_keeps_stage(void)
{
    uint8_t slots[
        STF_COLLISION_PARTICLE_EXTENDED_SLOTS *
        STF_COLLISION_PARTICLE_SLOT_SIZE
    ];
    stf_collision_particle_descriptor desc[STF_COLLISION_PARTICLE_KIND_COUNT];
    stf_collision_particle_stage stage;
    stf_collision_particle_result result;

    memset(slots, 0, sizeof(slots));
    memset(desc, 0, sizeof(desc));
    memset(&stage, 0, sizeof(stage));
    stage.position_a[0] = UINT32_C(0x11111111);
    stage.position_b[2] = UINT32_C(0x22222222);
    stage.flags = UINT8_C(0x80);

    if (!stf_collision_particle_put_model2(
            slots, sizeof(slots), UINT8_C(0), desc, &stage, &result
        ) ||
        result.requested ||
        result.allocated ||
        result.slot_limit != STF_COLLISION_PARTICLE_NORMAL_SLOTS ||
        stage.position_a[0] != UINT32_C(0x11111111) ||
        stage.position_b[2] != UINT32_C(0x22222222) ||
        stage.flags != UINT8_C(0x80)) {
        return 1;
    }

    return 0;
}

static int test_allocates_first_free_slot(void)
{
    uint8_t slots[
        STF_COLLISION_PARTICLE_EXTENDED_SLOTS *
        STF_COLLISION_PARTICLE_SLOT_SIZE
    ];
    stf_collision_particle_descriptor desc[STF_COLLISION_PARTICLE_KIND_COUNT];
    stf_collision_particle_stage stage;
    stf_collision_particle_result result;
    uint8_t *slot;

    memset(slots, 0, sizeof(slots));
    memset(desc, 0, sizeof(desc));
    memset(&stage, 0, sizeof(stage));

    write_le32(slots + 0x1Cu, UINT32_C(0xDEADBEEF));
    desc[1].effect_address = UINT32_C(0x12345678);
    desc[1].initial_value = UINT32_C(0xCAFEBABE);

    stage.position_a[0] = UINT32_C(1);
    stage.position_a[1] = UINT32_C(2);
    stage.position_a[2] = UINT32_C(3);
    stage.position_b[0] = UINT32_C(4);
    stage.position_b[1] = UINT32_C(5);
    stage.position_b[2] = UINT32_C(6);
    stage.draw_particle = UINT16_C(1);
    stage.flags = UINT8_C(0x09);

    if (!stf_collision_particle_put_model2(
            slots, sizeof(slots), UINT8_C(0), desc, &stage, &result
        ) ||
        !result.requested ||
        !result.allocated ||
        result.slot_index != UINT8_C(1) ||
        result.slot_limit != STF_COLLISION_PARTICLE_NORMAL_SLOTS) {
        return 1;
    }

    slot = slots + STF_COLLISION_PARTICLE_SLOT_SIZE;
    if (read_le32(slot + 0x00u) != UINT32_C(1) ||
        read_le32(slot + 0x04u) != UINT32_C(2) ||
        read_le32(slot + 0x08u) != UINT32_C(3) ||
        read_le32(slot + 0x0Cu) != UINT32_C(4) ||
        read_le32(slot + 0x10u) != UINT32_C(5) ||
        read_le32(slot + 0x14u) != UINT32_C(6) ||
        slot[0x19u] != UINT8_C(0x09) ||
        read_le32(slot + 0x1Cu) != UINT32_C(0x12345678) ||
        read_le32(slot + 0x20u) != UINT32_C(0xCAFEBABE) ||
        stage.draw_particle != UINT16_C(0) ||
        stage.position_a[0] != UINT32_C(0) ||
        stage.position_b[2] != UINT32_C(0) ||
        stage.flags != UINT8_C(0x09)) {
        return 1;
    }

    return 0;
}

static int test_extended_pool_uses_slot_16(void)
{
    uint8_t slots[
        STF_COLLISION_PARTICLE_EXTENDED_SLOTS *
        STF_COLLISION_PARTICLE_SLOT_SIZE
    ];
    stf_collision_particle_descriptor desc[STF_COLLISION_PARTICLE_KIND_COUNT];
    stf_collision_particle_stage stage;
    stf_collision_particle_result result;
    unsigned index;

    memset(slots, 0, sizeof(slots));
    memset(desc, 0, sizeof(desc));
    memset(&stage, 0, sizeof(stage));

    for (index = 0u; index < STF_COLLISION_PARTICLE_NORMAL_SLOTS; ++index) {
        write_le32(
            slots + (size_t)index * STF_COLLISION_PARTICLE_SLOT_SIZE + 0x1Cu,
            UINT32_C(1)
        );
    }

    desc[1].effect_address = UINT32_C(0x1000);
    stage.draw_particle = UINT16_C(1);

    if (!stf_collision_particle_put_model2(
            slots, sizeof(slots), UINT8_C(0x1A), desc, &stage, &result
        ) ||
        !result.allocated ||
        result.slot_limit != STF_COLLISION_PARTICLE_EXTENDED_SLOTS ||
        result.slot_index != UINT8_C(16)) {
        return 1;
    }

    return 0;
}

static int test_full_pool_consumes_nonzero_stage(void)
{
    uint8_t slots[
        STF_COLLISION_PARTICLE_EXTENDED_SLOTS *
        STF_COLLISION_PARTICLE_SLOT_SIZE
    ];
    stf_collision_particle_descriptor desc[STF_COLLISION_PARTICLE_KIND_COUNT];
    stf_collision_particle_stage stage;
    stf_collision_particle_result result;
    unsigned index;

    memset(slots, 0, sizeof(slots));
    memset(desc, 0, sizeof(desc));
    memset(&stage, 0, sizeof(stage));

    for (index = 0u; index < STF_COLLISION_PARTICLE_NORMAL_SLOTS; ++index) {
        write_le32(
            slots + (size_t)index * STF_COLLISION_PARTICLE_SLOT_SIZE + 0x1Cu,
            UINT32_C(1)
        );
    }

    stage.position_a[0] = UINT32_C(7);
    stage.position_b[0] = UINT32_C(8);
    stage.draw_particle = UINT16_C(1);

    if (!stf_collision_particle_put_model2(
            slots, sizeof(slots), UINT8_C(0), desc, &stage, &result
        ) ||
        !result.requested ||
        result.allocated ||
        stage.draw_particle != UINT16_C(0) ||
        stage.position_a[0] != UINT32_C(0) ||
        stage.position_b[0] != UINT32_C(0)) {
        return 1;
    }

    return 0;
}

static int test_updates_frame_and_age(void);
static int test_releases_expired_slot(void);
static int test_flag3_scale_sequence(void);
static int test_updater_scans_only_first_16_slots(void);

int main(void)
{
    if (test_zero_request_keeps_stage() != 0) return 1;
    if (test_allocates_first_free_slot() != 0) return 1;
    if (test_extended_pool_uses_slot_16() != 0) return 1;
    if (test_full_pool_consumes_nonzero_stage() != 0) return 1;
    if (test_updates_frame_and_age() != 0) return 1;
    if (test_releases_expired_slot() != 0) return 1;
    if (test_flag3_scale_sequence() != 0) return 1;
    if (test_updater_scans_only_first_16_slots() != 0) return 1;
    return 0;
}


static int test_updates_frame_and_age(void)
{
    uint8_t slots[
        STF_COLLISION_PARTICLE_EXTENDED_SLOTS *
        STF_COLLISION_PARTICLE_SLOT_SIZE
    ];
    static const uint16_t frames[] = { UINT16_C(100), UINT16_C(101) };
    stf_collision_particle_descriptor desc[STF_COLLISION_PARTICLE_KIND_COUNT];
    stf_collision_particle_update_result result;

    memset(slots, 0, sizeof(slots));
    memset(desc, 0, sizeof(desc));

    desc[1].effect_address = UINT32_C(0x2000);
    desc[1].duration = UINT16_C(4);
    desc[1].frame_divisor = UINT16_C(2);
    desc[1].frames = frames;
    desc[1].frame_count = 2u;

    write_le32(slots + 0x1Cu, UINT32_C(0x2000));
    slots[0x18u] = UINT8_C(3);

    if (!stf_collision_particle_update_model2(
            slots, sizeof(slots), desc, &result
        ) ||
        result.active_before != UINT8_C(1) ||
        result.advanced != UINT8_C(1) ||
        result.released != UINT8_C(0) ||
        slots[0x18u] != UINT8_C(4) ||
        ((uint16_t)slots[0x1Au] | ((uint16_t)slots[0x1Bu] << 8u)) != UINT16_C(101)) {
        return 1;
    }

    return 0;
}

static int test_releases_expired_slot(void)
{
    uint8_t slots[
        STF_COLLISION_PARTICLE_EXTENDED_SLOTS *
        STF_COLLISION_PARTICLE_SLOT_SIZE
    ];
    static const uint16_t frames[] = { UINT16_C(7) };
    stf_collision_particle_descriptor desc[STF_COLLISION_PARTICLE_KIND_COUNT];
    stf_collision_particle_update_result result;

    memset(slots, 0, sizeof(slots));
    memset(slots, 0xAA, STF_COLLISION_PARTICLE_SLOT_SIZE);
    memset(desc, 0, sizeof(desc));

    desc[1].effect_address = UINT32_C(0x3000);
    desc[1].duration = UINT16_C(1);
    desc[1].frame_divisor = UINT16_C(1);
    desc[1].frames = frames;
    desc[1].frame_count = 1u;

    write_le32(slots + 0x1Cu, UINT32_C(0x3000));
    slots[0x18u] = UINT8_C(1);

    if (!stf_collision_particle_update_model2(
            slots, sizeof(slots), desc, &result
        ) ||
        result.active_before != UINT8_C(1) ||
        result.advanced != UINT8_C(0) ||
        result.released != UINT8_C(1) ||
        read_le32(slots + 0x00u) != UINT32_C(0) ||
        read_le32(slots + 0x0Cu) != UINT32_C(0) ||
        slots[0x18u] != UINT8_C(0) ||
        slots[0x19u] != UINT8_C(0) ||
        read_le32(slots + 0x1Cu) != UINT32_C(0) ||
        read_le32(slots + 0x20u) != UINT32_C(0)) {
        return 1;
    }

    return 0;
}

static int test_flag3_scale_sequence(void)
{
    uint8_t slots[
        STF_COLLISION_PARTICLE_EXTENDED_SLOTS *
        STF_COLLISION_PARTICLE_SLOT_SIZE
    ];
    static const uint16_t frames[] = { UINT16_C(9), UINT16_C(9), UINT16_C(9), UINT16_C(9) };
    stf_collision_particle_descriptor desc[STF_COLLISION_PARTICLE_KIND_COUNT];
    stf_collision_particle_update_result result;

    memset(slots, 0, sizeof(slots));
    memset(desc, 0, sizeof(desc));

    desc[1].effect_address = UINT32_C(0x4000);
    desc[1].duration = UINT16_C(8);
    desc[1].frame_divisor = UINT16_C(2);
    desc[1].frames = frames;
    desc[1].frame_count = 4u;

    write_le32(slots + 0x1Cu, UINT32_C(0x4000));
    slots[0x18u] = UINT8_C(6);
    slots[0x19u] = UINT8_C(1u << 3u);

    if (!stf_collision_particle_update_model2(
            slots, sizeof(slots), desc, &result
        ) ||
        read_le32(slots + 0x20u) != UINT32_C(0x3ECCCCCD) ||
        slots[0x18u] != UINT8_C(7)) {
        return 1;
    }

    return 0;
}

static int test_updater_scans_only_first_16_slots(void)
{
    uint8_t slots[
        STF_COLLISION_PARTICLE_EXTENDED_SLOTS *
        STF_COLLISION_PARTICLE_SLOT_SIZE
    ];
    static const uint16_t frames[] = { UINT16_C(1) };
    stf_collision_particle_descriptor desc[STF_COLLISION_PARTICLE_KIND_COUNT];
    stf_collision_particle_update_result result;
    uint8_t *slot16;

    memset(slots, 0, sizeof(slots));
    memset(desc, 0, sizeof(desc));

    desc[1].effect_address = UINT32_C(0x5000);
    desc[1].duration = UINT16_C(1);
    desc[1].frame_divisor = UINT16_C(1);
    desc[1].frames = frames;
    desc[1].frame_count = 1u;

    slot16 = slots + 16u * STF_COLLISION_PARTICLE_SLOT_SIZE;
    write_le32(slot16 + 0x1Cu, UINT32_C(0x5000));

    if (!stf_collision_particle_update_model2(
            slots, sizeof(slots), desc, &result
        ) ||
        result.active_before != UINT8_C(0) ||
        read_le32(slot16 + 0x1Cu) != UINT32_C(0x5000)) {
        return 1;
    }

    return 0;
}
