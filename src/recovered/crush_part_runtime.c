#include "crush_part_runtime.h"

#include <string.h>

static uint16_t read_le16u(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8u));
}

static int16_t read_le16s(const uint8_t *p)
{
    return (int16_t)read_le16u(p);
}

static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8u) |
           ((uint32_t)p[2] << 16u) |
           ((uint32_t)p[3] << 24u);
}

static void write_le16(uint8_t *p, int16_t value)
{
    const uint16_t u = (uint16_t)value;
    p[0] = (uint8_t)u;
    p[1] = (uint8_t)(u >> 8u);
}

static int16_t add_wrap16(int16_t a, int32_t b)
{
    return (int16_t)((uint16_t)a + (uint16_t)b);
}

static int16_t approach_axis(int16_t current, int16_t target, int32_t step)
{
    const int32_t delta = (int32_t)target - (int32_t)current;

    if (target == 0 || target == current) {
        return current;
    }
    if (delta > step) {
        return add_wrap16(current, -step);
    }
    if (delta < -step) {
        return add_wrap16(current, step);
    }
    return target;
}

bool stf_crush_part_update_angles_model2(
    uint8_t *slot,
    size_t slot_size,
    stf_crush_part_angle_result *result
)
{
    uint32_t flags;
    uint32_t contacts;
    int32_t step;
    int16_t x;
    int16_t y;
    int16_t z;
    const int16_t spin = slot != NULL ? read_le16s(slot + 0x2Eu) : 0;

    if (slot == NULL || result == NULL || slot_size < STF_CRUSH_PART_SLOT_SIZE) {
        return false;
    }

    flags = read_le32(slot + 0x24u);
    contacts = read_le32(slot + 0x3Cu);
    x = read_le16s(slot + 0x1Cu);
    y = read_le16s(slot + 0x1Eu);
    z = read_le16s(slot + 0x20u);

    step = (flags & (UINT32_C(1) << 7u)) != 0u ? 0x800 : 0x1000;

    if ((flags & (UINT32_C(1) << 7u)) == 0u && contacts <= UINT32_C(3)) {
        if ((flags & (UINT32_C(1) << 12u)) != 0u) {
            x = add_wrap16(x, spin);
        }
        y = add_wrap16(y, spin);
        if ((flags & (UINT32_C(1) << 14u)) != 0u) {
            z = add_wrap16(z, spin);
        }
    } else {
        const int16_t target_x = read_le16s(slot + 0x28u);
        const int16_t target_y = read_le16s(slot + 0x2Au);
        const int16_t target_z = read_le16s(slot + 0x2Cu);
        const int32_t negative_step = -step;

        if (target_x != 0 && target_x != x) {
            const int32_t delta = (int32_t)target_x - (int32_t)x;
            if (delta > step) {
                x = add_wrap16(x, -step);
            } else if (delta < negative_step) {
                x = add_wrap16(x, step);
            } else {
                x = target_x;
            }
        }

        if (target_y != 0 && target_y != y) {
            const int32_t delta = (int32_t)target_y - (int32_t)y;
            if (delta > step || delta < negative_step) {
                const uint32_t divisor = contacts + UINT32_C(1);
                if (divisor != UINT32_C(0)) {
                    step /= (int32_t)divisor;
                }
                if (delta > 0) {
                    y = add_wrap16(y, -step);
                } else {
                    y = add_wrap16(y, step);
                }
            } else {
                y = target_y;
            }
        }

        if (target_z != 0 && target_z != z) {
            const int32_t delta = (int32_t)target_z - (int32_t)z;
            if (delta > step) {
                z = add_wrap16(z, -step);
            } else if (delta < negative_step) {
                z = add_wrap16(z, step);
            } else {
                z = target_z;
            }
        }
    }

    write_le16(slot + 0x1Cu, x);
    write_le16(slot + 0x1Eu, y);
    write_le16(slot + 0x20u, z);

    result->angle_x = x;
    result->angle_y = y;
    result->angle_z = z;
    result->step = (int16_t)step;
    return true;
}

bool stf_crush_part_build_draw_model2(
    const uint8_t *slot,
    size_t slot_size,
    stf_crush_part_draw *draw
)
{
    uint32_t flags;

    if (slot == NULL || draw == NULL || slot_size < STF_CRUSH_PART_SLOT_SIZE) {
        return false;
    }

    memset(draw, 0, sizeof(*draw));
    draw->object_id = read_le16u(slot + 0x22u);
    if (draw->object_id == UINT16_C(0)) {
        return true;
    }

    flags = read_le32(slot + 0x24u);
    draw->active = true;
    draw->owner_index = (uint8_t)(flags & UINT32_C(1));
    draw->use_saved_graphics_state =
        (flags & (UINT32_C(1) << 19u)) != 0u;
    draw->position[0] = read_le32(slot + 0x00u);
    draw->position[1] = read_le32(slot + 0x04u);
    draw->position[2] = read_le32(slot + 0x08u);
    draw->angle_x = read_le16s(slot + 0x1Cu);
    draw->angle_y = read_le16s(slot + 0x1Eu);
    draw->angle_z = read_le16s(slot + 0x20u);
    return true;
}
