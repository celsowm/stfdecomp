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

void stf_collision_support_prelude_step(
    stf_collision_support_prelude *state,
    uint32_t debug_flag
)
{
    if (state == NULL || (debug_flag & (UINT32_C(1) << 5u)) != 0u) {
        return;
    }

    state->flags_1b0 = (state->flags_1b0 & UINT32_C(1)) | UINT32_C(0x3FE);
    state->counter_220 = (uint16_t)(state->counter_220 + UINT16_C(1));
    if (state->counter_23c != 0u) {
        --state->counter_23c;
    }
    if (state->counter_23e != 0u) {
        --state->counter_23e;
    }
}

bool stf_collision_support_prelude_apply_model2(
    uint8_t *workspace,
    size_t workspace_size,
    uint32_t debug_flag
)
{
    stf_collision_support_prelude state;

    if (workspace == NULL ||
        workspace_size < STF_COLLISION_SUPPORT_MODEL2_MIN_SIZE) {
        return false;
    }

    if ((debug_flag & (UINT32_C(1) << 5u)) != 0u) {
        return true;
    }

    state.flags_1b0 = read_le32(workspace + 0x1B0u);
    state.counter_220 = read_le16(workspace + 0x220u);
    state.counter_23c = read_le16(workspace + 0x23Cu);
    state.counter_23e = read_le16(workspace + 0x23Eu);

    stf_collision_support_prelude_step(&state, debug_flag);

    write_le32(workspace + 0x1B4u, 0u);
    write_le32(workspace + 0x1B8u, 0u);
    write_le32(workspace + 0x1BCu, 0u);
    write_le32(workspace + 0x1C0u, 0u);
    write_le32(workspace + 0x1C4u, 0u);
    write_le32(workspace + 0x1C8u, 0u);
    write_le32(workspace + 0x1CCu, 0u);
    write_le32(workspace + 0x1DCu, 0u);
    write_le32(workspace + 0x1E0u, 0u);
    write_le32(workspace + 0x1E4u, 0u);
    write_le32(workspace + 0x1B0u, state.flags_1b0);
    write_le16(workspace + 0x220u, state.counter_220);
    write_le16(workspace + 0x23Cu, state.counter_23c);
    write_le16(workspace + 0x23Eu, state.counter_23e);

    return true;
}
