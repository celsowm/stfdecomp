#include <stdint.h>
#include <string.h>

#include "collision_no_unit.h"

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

static uint16_t read_le16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8u));
}

int main(void)
{
    uint8_t fighter[STF_COLLISION_NO_UNIT_SELF_MIN_SIZE];
    uint8_t opponent[STF_COLLISION_NO_UNIT_OTHER_MIN_SIZE];
    uint16_t mask = 0u;
    size_t i = 0u;

    memset(fighter, 0, sizeof(fighter));
    memset(opponent, 0, sizeof(opponent));

    /* Early all-mask branch: both bit 8, self A29 >= 30, other A29 <= 5. */
    write_le32(fighter + 0x1A4u, UINT32_C(1) << 8u);
    write_le32(opponent + 0x1A4u, UINT32_C(1) << 8u);
    fighter[0xA29u] = 30u;
    opponent[0xA29u] = 5u;
    if (!stf_collision_no_unit_mask_model2(
            fighter, sizeof(fighter), opponent, sizeof(opponent),
            UINT32_C(0), UINT32_C(0), UINT32_C(0), &mask
        ) || mask != UINT16_MAX) {
        return 1;
    }

    /* Other fighter bit 8 clear -> zero mask. */
    write_le32(opponent + 0x1A4u, 0u);
    if (!stf_collision_no_unit_mask_model2(
            fighter, sizeof(fighter), opponent, sizeof(opponent),
            0u, 0u, 0u, &mask
        ) || mask != 0u) {
        return 2;
    }

    /* Self bit 15 with incompatible opponent kind -> all mask. */
    memset(fighter, 0, sizeof(fighter));
    memset(opponent, 0, sizeof(opponent));
    write_le32(fighter + 0x1A4u, UINT32_C(1) << 15u);
    write_le32(opponent + 0x1A4u, UINT32_C(1) << 8u);
    opponent[0x821u] = 3u;
    if (!stf_collision_no_unit_mask_model2(
            fighter, sizeof(fighter), opponent, sizeof(opponent),
            0u, 0u, 0u, &mask
        ) || mask != UINT16_MAX) {
        return 3;
    }

    /* Bit-14 height scan: set every other sample above threshold. */
    memset(fighter, 0, sizeof(fighter));
    memset(opponent, 0, sizeof(opponent));
    write_le32(fighter + 0x1A4u, UINT32_C(1) << 14u);
    write_le32(opponent + 0x1A4u, UINT32_C(1) << 8u);
    write_le16(fighter + 0x61Cu, 1u);
    opponent[0x821u] = 1u;
    write_le32(fighter + 0x1F4u, UINT32_C(0x00000000)); /* x = 0 */
    write_le32(fighter + 0x1FCu, UINT32_C(0x00000000)); /* z = 0 */
    for (i = 0u; i < 16u; ++i) {
        write_le32(
            fighter + 0x1F8u + i * 0xCu,
            (i & 1u) ? UINT32_C(0x40000000) : UINT32_C(0x3F000000)
        ); /* 2.0 / 0.5 */
    }
    if (!stf_collision_no_unit_mask_model2(
            fighter, sizeof(fighter), opponent, sizeof(opponent),
            UINT32_C(0x41200000), /* 10.0, ignored because inside stage extent */
            UINT32_C(0x41A00000), /* 20.0 */
            UINT32_C(0x3F800000), /* 1.0 threshold */
            &mask
        ) || mask != UINT16_C(0xAAAA)) {
        return 4;
    }

    /* Fallback height comparison branch. */
    memset(fighter, 0, sizeof(fighter));
    memset(opponent, 0, sizeof(opponent));
    write_le32(fighter + 0x1A4u, UINT32_C(1) << 4u);
    write_le32(opponent + 0x1A4u, UINT32_C(1) << 8u);
    opponent[0x821u] = 2u;
    write_le32(fighter + 0x1F8u, UINT32_C(0x40A00000)); /* 5.0 */
    write_le32(fighter + 0x1E10u, UINT32_C(0x40800000)); /* 4.0 */
    if (!stf_collision_no_unit_apply_model2(
            fighter, sizeof(fighter), opponent, sizeof(opponent),
            0u, 0u, 0u
        ) || read_le16(fighter + 0x6F8u) != UINT16_MAX) {
        return 5;
    }

    if (stf_collision_no_unit_mask_model2(
            fighter,
            STF_COLLISION_NO_UNIT_SELF_MIN_SIZE - 1u,
            opponent,
            sizeof(opponent),
            0u, 0u, 0u, &mask
        )) {
        return 6;
    }

    return 0;
}
