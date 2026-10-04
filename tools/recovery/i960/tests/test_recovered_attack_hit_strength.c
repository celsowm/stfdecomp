#include <stdint.h>
#include <string.h>

#include "attack_hit_strength.h"

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
    uint8_t fighter[STF_ATTACK_HIT_STRENGTH_FIGHTER_MODEL2_MIN_SIZE];
    uint8_t workspace[STF_ATTACK_HIT_STRENGTH_WORKSPACE_MODEL2_MIN_SIZE];
    stf_attack_hit_strength_input result;

    memset(fighter, 0, sizeof(fighter));
    memset(workspace, 0, sizeof(workspace));

    write_le16(fighter + 0x82Au, UINT16_C(0x0010));
    write_le16(fighter + 0x026u, UINT16_C(0xFFF0)); /* -16 */
    fighter[0x822u] = UINT8_C(3);
    write_le32(workspace + 0x26Cu, UINT32_C(0x10));

    if (!stf_attack_hit_strength_prepare_model2(
            fighter, sizeof(fighter),
            workspace, sizeof(workspace),
            &result
        )) {
        return 1;
    }

    if (result.combined_82a_026 != 0 ||
        result.raw_822 != UINT8_C(3) ||
        result.raw_822_float_bits != UINT32_C(0x40400000) ||
        result.workspace_26c != UINT32_C(0x11) ||
        read_le32(workspace + 0x26Cu) != UINT32_C(0x11)) {
        return 2;
    }

    fighter[0x822u] = UINT8_C(1);
    write_le16(fighter + 0x82Au, UINT16_C(0x7FFF));
    write_le16(fighter + 0x026u, UINT16_C(1));
    write_le32(workspace + 0x26Cu, 0u);

    if (!stf_attack_hit_strength_prepare_model2(
            fighter, sizeof(fighter),
            workspace, sizeof(workspace),
            &result
        ) ||
        result.combined_82a_026 != 32768 ||
        result.raw_822_float_bits != UINT32_C(0x3F800000)) {
        return 3;
    }

    fighter[0x822u] = UINT8_C(255);
    if (!stf_attack_hit_strength_prepare_model2(
            fighter, sizeof(fighter),
            workspace, sizeof(workspace),
            &result
        ) ||
        result.raw_822_float_bits != UINT32_C(0x437F0000)) {
        return 4;
    }

    fighter[0x822u] = 0u;
    if (!stf_attack_hit_strength_prepare_model2(
            fighter, sizeof(fighter),
            workspace, sizeof(workspace),
            &result
        ) ||
        result.raw_822_float_bits != 0u) {
        return 5;
    }

    {
        uint32_t scaled = 0u;

        if (!stf_attack_hit_strength_scale_bits(
                UINT32_C(0x40800000), /* 4.0 */
                UINT32_C(0x3F800000), /* x1.0 */
                &scaled
            ) ||
            scaled != UINT32_C(0x3CCCCCCC)) { /* float32 staged 0.025 */
            return 6;
        }

        if (!stf_attack_hit_strength_scale_bits(
                UINT32_C(0x41800000), /* 16.0 */
                UINT32_C(0x40000000), /* x2.0 */
                &scaled
            ) ||
            scaled != UINT32_C(0x3DCCCCCC)) { /* float32 staged 0.1 */
            return 7;
        }
    }

    return 0;
}
