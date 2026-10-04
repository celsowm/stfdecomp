#include <stdint.h>
#include <string.h>

#include "set_kamae_runtime.h"

typedef struct fixture {
    uint16_t selectors[4];
    uint8_t records[4][512];
} fixture;

static void write_le16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
}

static void write_u32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

static uint32_t float_bits(float value)
{
    uint32_t bits = 0u;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static int16_t read_i16(const uint8_t *data)
{
    return (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8u));
}

static bool resolve_motion(
    uint16_t selector,
    const uint8_t **motion_record,
    size_t *motion_record_size,
    void *user_data
)
{
    fixture *fx = (fixture *)user_data;
    size_t index = 0u;

    if (fx == NULL || motion_record == NULL || motion_record_size == NULL) {
        return false;
    }

    for (index = 0u; index < 4u; ++index) {
        if (fx->selectors[index] == selector) {
            *motion_record = fx->records[index];
            *motion_record_size = sizeof(fx->records[index]);
            return true;
        }
    }
    return false;
}

static void build_simple_record(uint8_t *record, float base)
{
    size_t index = 0u;

    memset(record, 0, 512u);
    for (index = 0u; index < STF_GET_KAMAE_ROW_COUNT; ++index) {
        record[STF_GET_KAMAE_DESCRIPTOR_OFFSET + index] = UINT8_C(0x15);
    }
    for (index = 0u; index < 60u; ++index) {
        write_u32(
            record + 24u + index * 4u,
            float_bits(base + (float)index)
        );
    }
}

int main(void)
{
    uint8_t selectors[STF_SET_KAMAE_FULL_SELECTOR_MIN_SIZE];
    uint8_t stance[2048];
    fixture fx;
    stf_set_kamae_runtime_result result;
    size_t index = 0u;

    memset(selectors, 0, sizeof(selectors));
    memset(stance, 0, sizeof(stance));
    memset(&fx, 0, sizeof(fx));

    fx.selectors[0] = UINT16_C(0x1010);
    fx.selectors[1] = UINT16_C(0x2020);
    fx.selectors[2] = UINT16_C(0x3030);
    fx.selectors[3] = UINT16_C(0x4040);

    write_le16(selectors + 0x00u, fx.selectors[0]);
    write_le16(selectors + 0x08u, fx.selectors[1]);
    write_le16(selectors + 0x0Au, fx.selectors[2]);
    write_le16(selectors + 0x50u, fx.selectors[3]);

    for (index = 0u; index < 4u; ++index) {
        build_simple_record(fx.records[index], (float)(10u * (index + 1u)));
    }

    if (!stf_set_kamae_runtime_apply(
            0u,
            selectors,
            sizeof(selectors),
            0u,
            stance,
            sizeof(stance),
            resolve_motion,
            &fx,
            &result
        ) ||
        result.request_count != 4u ||
        result.selectors[0] != fx.selectors[0] ||
        result.selectors[1] != fx.selectors[1] ||
        result.selectors[2] != fx.selectors[2] ||
        result.selectors[3] != fx.selectors[3] ||
        result.destination_offsets[0] != UINT32_C(0x1E0) ||
        result.destination_offsets[1] != UINT32_C(0x2D0) ||
        result.destination_offsets[2] != UINT32_C(0x3C0) ||
        result.destination_offsets[3] != UINT32_C(0x5A0)) {
        return 1;
    }

    if (read_i16(stance + 0x1E0u) != 10 ||
        read_i16(stance + 0x2D0u) != 20 ||
        read_i16(stance + 0x3C0u) != 30 ||
        read_i16(stance + 0x5A0u) != 40) {
        return 2;
    }

    memset(stance, 0, sizeof(stance));
    write_le16(selectors + 0x02u, fx.selectors[2]);

    if (!stf_set_kamae_runtime_apply(
            UINT32_C(1) << 29u,
            selectors,
            sizeof(selectors),
            UINT32_C(3) << 30u,
            stance,
            sizeof(stance),
            resolve_motion,
            &fx,
            &result
        ) ||
        result.request_count != 1u ||
        result.selectors[0] != fx.selectors[2] ||
        result.destination_offsets[0] != UINT32_C(0x1E0) ||
        read_i16(stance + 0x1E0u) != 30) {
        return 3;
    }

    write_le16(selectors + 0x02u, UINT16_C(0x9999));
    if (stf_set_kamae_runtime_apply(
            UINT32_C(1) << 29u,
            selectors,
            sizeof(selectors),
            0u,
            stance,
            sizeof(stance),
            resolve_motion,
            &fx,
            &result
        )) {
        return 4;
    }

    return 0;
}
