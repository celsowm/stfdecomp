#include <stdint.h>
#include <string.h>

#include "collision_support.h"

static uint16_t read_le16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8u));
}

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8u) |
           ((uint32_t)data[2] << 16u) |
           ((uint32_t)data[3] << 24u);
}

static void write_le16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
}

static void write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

int main(void)
{
    uint8_t workspace[STF_COLLISION_SUPPORT_MODEL2_MIN_SIZE];
    stf_collision_support_prelude state;
    size_t i = 0u;

    memset(workspace, 0xAA, sizeof(workspace));
    write_le32(workspace + 0x1B0u, UINT32_C(0xFFFFFFFF));
    write_le16(workspace + 0x220u, UINT16_C(0x1234));
    write_le16(workspace + 0x23Cu, UINT16_C(2));
    write_le16(workspace + 0x23Eu, UINT16_C(0));

    if (!stf_collision_support_prelude_apply_model2(
            workspace,
            sizeof(workspace),
            0u
        )) {
        return 1;
    }

    if (read_le32(workspace + 0x1B0u) != UINT32_C(0x3FF) ||
        read_le16(workspace + 0x220u) != UINT16_C(0x1235) ||
        read_le16(workspace + 0x23Cu) != UINT16_C(1) ||
        read_le16(workspace + 0x23Eu) != UINT16_C(0)) {
        return 2;
    }

    {
        const size_t zero_offsets[] = {
            0x1B4u, 0x1B8u, 0x1BCu, 0x1C0u, 0x1C4u,
            0x1C8u, 0x1CCu, 0x1DCu, 0x1E0u, 0x1E4u
        };
        for (i = 0u; i < sizeof(zero_offsets) / sizeof(zero_offsets[0]); ++i) {
            if (read_le32(workspace + zero_offsets[i]) != 0u) {
                return 3;
            }
        }
    }

    if (workspace[0x100u] != 0xAAu ||
        workspace[0x1D0u] != 0xAAu ||
        workspace[0x23Au] != 0xAAu) {
        return 4;
    }

    memset(workspace, 0x5Au, sizeof(workspace));
    if (!stf_collision_support_prelude_apply_model2(
            workspace,
            sizeof(workspace),
            UINT32_C(1) << 5u
        )) {
        return 5;
    }
    if (workspace[0x1B4u] != 0x5Au || workspace[0x220u] != 0x5Au) {
        return 6;
    }

    memset(&state, 0, sizeof(state));
    state.flags_1b0 = UINT32_C(0x80000000);
    state.counter_220 = UINT16_C(0xFFFF);
    state.counter_23c = UINT16_C(1);
    state.counter_23e = UINT16_C(1);
    stf_collision_support_prelude_step(&state, 0u);

    if (state.flags_1b0 != UINT32_C(0x3FE) ||
        state.counter_220 != UINT16_C(0) ||
        state.counter_23c != UINT16_C(0) ||
        state.counter_23e != UINT16_C(0)) {
        return 7;
    }

    if (stf_collision_support_prelude_apply_model2(
            workspace,
            STF_COLLISION_SUPPORT_MODEL2_MIN_SIZE - 1u,
            0u
        )) {
        return 8;
    }

    return 0;
}
