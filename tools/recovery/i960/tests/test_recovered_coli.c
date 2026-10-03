#include <stdint.h>
#include <string.h>

#include "coli_init.h"

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

static void write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

int main(void)
{
    uint8_t workspace[STF_COLI_TASK_MODEL2_SIZE];
    stf_coli_init_values values;

    memset(workspace, 0xAA, sizeof(workspace));
    write_le32(workspace + 0x1B0u, UINT32_C(0xFFFFFFFF));

    if (!stf_coli_init_apply_model2(
            workspace,
            sizeof(workspace),
            UINT32_C(0x12345678)
        )) {
        return 1;
    }

    if (read_le32(workspace + 0x00Cu) != UINT32_C(0x12345678) ||
        read_le32(workspace + 0x090u) != 0u ||
        read_le16(workspace + 0x158u) != UINT16_C(0x0700) ||
        read_le32(workspace + 0x1B0u) != UINT32_C(0xFFFFFFFE) ||
        read_le32(workspace + 0x1D0u) != UINT32_C(0x3C23D70A) ||
        read_le32(workspace + 0x1D4u) != UINT32_C(0x3FC00000) ||
        read_le32(workspace + 0x1D8u) != UINT32_C(0x3DCCCCCD) ||
        read_le16(workspace + 0x20Cu) != UINT16_C(0x0700) ||
        read_le32(workspace + 0x278u) != UINT32_C(0x40400000)) {
        return 2;
    }

    if (workspace[0x100u] != 0xAAu ||
        workspace[0x1AFu] != 0xAAu ||
        workspace[0x27Cu] != 0xAAu ||
        workspace[STF_COLI_TASK_MODEL2_SIZE - 1u] != 0xAAu) {
        return 3;
    }

    if (stf_coli_init_apply_model2(
            workspace,
            STF_COLI_TASK_MODEL2_SIZE - 1u,
            0u
        )) {
        return 4;
    }

    memset(&values, 0, sizeof(values));
    stf_coli_init_values_build(
        &values,
        UINT32_C(0x89ABCDEF),
        UINT32_C(0xA5A5A5A5)
    );
    if (values.update_address != UINT32_C(0x89ABCDEF) ||
        values.flags_1b0 != UINT32_C(0xA5A5A5A4) ||
        values.field_1d4_bits != UINT32_C(0x3FC00000)) {
        return 5;
    }

    return 0;
}
