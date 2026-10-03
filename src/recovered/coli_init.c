#include "coli_init.h"

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

void stf_coli_init_values_build(
    stf_coli_init_values *values,
    uint32_t collision_address,
    uint32_t previous_flags_1b0
)
{
    if (values == NULL) {
        return;
    }

    values->update_address = collision_address;
    values->field_090 = 0u;
    values->field_158 = UINT16_C(0x0700);
    values->flags_1b0 = previous_flags_1b0 & ~UINT32_C(1);
    values->field_1d0_bits = UINT32_C(0x3C23D70A);
    values->field_1d4_bits = UINT32_C(0x3FC00000);
    values->field_1d8_bits = UINT32_C(0x3DCCCCCD);
    values->field_20c = UINT16_C(0x0700);
    values->field_278_bits = UINT32_C(0x40400000);
}

bool stf_coli_init_apply_model2(
    uint8_t *workspace,
    size_t workspace_size,
    uint32_t collision_address
)
{
    stf_coli_init_values values;
    uint32_t previous_flags = 0u;

    if (workspace == NULL || workspace_size < STF_COLI_TASK_MODEL2_SIZE) {
        return false;
    }

    previous_flags = read_le32(workspace + 0x1B0u);
    stf_coli_init_values_build(
        &values,
        collision_address,
        previous_flags
    );

    write_le32(workspace + 0x00Cu, values.update_address);
    write_le32(workspace + 0x090u, values.field_090);
    write_le16(workspace + 0x158u, values.field_158);
    write_le32(workspace + 0x1B0u, values.flags_1b0);
    write_le32(workspace + 0x1D0u, values.field_1d0_bits);
    write_le32(workspace + 0x1D4u, values.field_1d4_bits);
    write_le32(workspace + 0x1D8u, values.field_1d8_bits);
    write_le16(workspace + 0x20Cu, values.field_20c);
    write_le32(workspace + 0x278u, values.field_278_bits);

    return true;
}
