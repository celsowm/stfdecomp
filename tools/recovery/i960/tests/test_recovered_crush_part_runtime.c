#include <stdint.h>
#include <string.h>

#include "crush_part_runtime.h"

static void write_le16(uint8_t *p, int16_t v)
{
    const uint16_t u = (uint16_t)v;
    p[0] = (uint8_t)u;
    p[1] = (uint8_t)(u >> 8u);
}

static void write_le32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8u);
    p[2] = (uint8_t)(v >> 16u);
    p[3] = (uint8_t)(v >> 24u);
}

static int test_free_spin_branch(void)
{
    uint8_t slot[STF_CRUSH_PART_SLOT_SIZE];
    stf_crush_part_angle_result result;

    memset(slot, 0, sizeof(slot));
    write_le16(slot + 0x1Cu, INT16_C(100));
    write_le16(slot + 0x1Eu, INT16_C(200));
    write_le16(slot + 0x20u, INT16_C(300));
    write_le16(slot + 0x2Eu, INT16_C(25));
    write_le32(slot + 0x24u,
               (UINT32_C(1) << 12u) | (UINT32_C(1) << 14u));
    write_le32(slot + 0x3Cu, UINT32_C(2));

    if (!stf_crush_part_update_angles_model2(slot, sizeof(slot), &result) ||
        result.angle_x != INT16_C(125) ||
        result.angle_y != INT16_C(225) ||
        result.angle_z != INT16_C(325) ||
        result.step != INT16_C(0x1000)) {
        return 1;
    }
    return 0;
}

static int test_target_approach_branch(void)
{
    uint8_t slot[STF_CRUSH_PART_SLOT_SIZE];
    stf_crush_part_angle_result result;

    memset(slot, 0, sizeof(slot));
    write_le16(slot + 0x1Cu, INT16_C(10000));
    write_le16(slot + 0x1Eu, INT16_C(10000));
    write_le16(slot + 0x20u, INT16_C(-10000));
    write_le16(slot + 0x28u, INT16_C(0));
    write_le16(slot + 0x2Au, INT16_C(-10000));
    write_le16(slot + 0x2Cu, INT16_C(10000));
    write_le32(slot + 0x24u, UINT32_C(1) << 7u);
    write_le32(slot + 0x3Cu, UINT32_C(3));

    if (!stf_crush_part_update_angles_model2(slot, sizeof(slot), &result) ||
        result.angle_x != INT16_C(10000) ||
        result.angle_y != INT16_C(7952) ||
        result.angle_z != INT16_C(-7952) ||
        result.step != INT16_C(0x0800)) {
        return 1;
    }
    return 0;
}

static int test_draw_extraction(void)
{
    uint8_t slot[STF_CRUSH_PART_SLOT_SIZE];
    stf_crush_part_draw draw;

    memset(slot, 0, sizeof(slot));
    write_le16(slot + 0x22u, INT16_C(777));
    write_le32(slot + 0x24u, (UINT32_C(1) << 19u) | UINT32_C(1));
    write_le32(slot + 0x00u, UINT32_C(0x11111111));
    write_le32(slot + 0x04u, UINT32_C(0x22222222));
    write_le32(slot + 0x08u, UINT32_C(0x33333333));
    write_le16(slot + 0x1Cu, INT16_C(10));
    write_le16(slot + 0x1Eu, INT16_C(20));
    write_le16(slot + 0x20u, INT16_C(30));

    if (!stf_crush_part_build_draw_model2(slot, sizeof(slot), &draw) ||
        !draw.active ||
        !draw.use_saved_graphics_state ||
        draw.owner_index != UINT8_C(1) ||
        draw.object_id != UINT16_C(777) ||
        draw.position[0] != UINT32_C(0x11111111) ||
        draw.position[1] != UINT32_C(0x22222222) ||
        draw.position[2] != UINT32_C(0x33333333) ||
        draw.angle_x != INT16_C(10) ||
        draw.angle_y != INT16_C(20) ||
        draw.angle_z != INT16_C(30)) {
        return 1;
    }
    return 0;
}

int main(void)
{
    if (test_free_spin_branch() != 0) return 1;
    if (test_target_approach_branch() != 0) return 1;
    if (test_draw_extraction() != 0) return 1;
    return 0;
}
