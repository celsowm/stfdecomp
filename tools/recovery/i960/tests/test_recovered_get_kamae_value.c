#include <stdint.h>
#include <string.h>

#include "get_kamae_value.h"

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

int main(void)
{
    uint8_t record[1024];
    uint8_t stance[2048];
    stf_get_kamae_value_result result;
    const size_t destination = 0x1E0u;
    size_t index = 0u;

    memset(record, 0, sizeof(record));
    memset(stance, 0xA5, sizeof(stance));
    for (index = 0u; index < STF_GET_KAMAE_ROW_COUNT; ++index) {
        record[STF_GET_KAMAE_DESCRIPTOR_OFFSET + index] = UINT8_C(0x15);
    }
    for (index = 0u; index < 60u; ++index) {
        float value = (float)index + 0.25f;
        if (index == 0u) {
            value = 1.5f;
        } else if (index == 1u) {
            value = 2.5f;
        } else if (index == 2u) {
            value = -1.5f;
        }
        write_u32(record + 24u + index * 4u, float_bits(value));
    }

    if (!stf_get_kamae_value_apply(
            record, sizeof(record), 0u,
            stance, sizeof(stance), destination, &result
        ) ||
        result.payload_offset != 24u ||
        result.source_start_offset != 24u ||
        result.destination_end_offset != destination + 0xF0u ||
        result.conversion_start_offset != destination) {
        return 1;
    }
    if (read_i16(stance + destination) != 2 ||
        read_i16(stance + destination + 4u) != 2 ||
        read_i16(stance + destination + 8u) != -2) {
        return 2;
    }
    if (memcmp(
            stance + destination + 36u * 4u,
            record + 24u + 36u * 4u,
            4u
        ) != 0) {
        return 3;
    }

    memset(record, 0, sizeof(record));
    memset(stance, 0, sizeof(stance));
    record[STF_GET_KAMAE_DESCRIPTOR_OFFSET] = UINT8_C(0x1E);
    for (index = 1u; index < STF_GET_KAMAE_ROW_COUNT; ++index) {
        record[STF_GET_KAMAE_DESCRIPTOR_OFFSET + index] = UINT8_C(0x15);
    }
    record[STF_GET_KAMAE_COUNT_STREAM_OFFSET] = 1u;
    record[STF_GET_KAMAE_COUNT_STREAM_OFFSET + 1u] = 2u;
    for (index = 0u; index < 50u; ++index) {
        write_u32(record + 24u + index * 4u, float_bits((float)(100u + index)));
    }

    if (!stf_get_kamae_value_apply(
            record, sizeof(record), 0u,
            stance, sizeof(stance), destination, &result
        ) ||
        result.count_stream_bytes != 2u ||
        result.count_stream_sum != 3u ||
        result.source_start_offset != 36u) {
        return 4;
    }

    memset(record, 0, sizeof(record));
    memset(stance, 0, sizeof(stance));
    record[STF_GET_KAMAE_DESCRIPTOR_OFFSET] = UINT8_C(0x40);
    for (index = 1u; index < STF_GET_KAMAE_ROW_COUNT; ++index) {
        record[STF_GET_KAMAE_DESCRIPTOR_OFFSET + index] = UINT8_C(0x15);
    }
    for (index = 0u; index < 60u; ++index) {
        write_u32(record + 24u + index * 4u, float_bits((float)index));
    }
    if (!stf_get_kamae_value_apply(
            record, sizeof(record), 0u,
            stance, sizeof(stance), destination, &result
        ) ||
        result.destination_end_offset != destination + 0xF0u ||
        result.conversion_start_offset != destination) {
        return 5;
    }

    memset(record, 0, sizeof(record));
    memset(stance, 0, sizeof(stance));
    record[STF_GET_KAMAE_DESCRIPTOR_OFFSET] = UINT8_C(0xC0);
    for (index = 1u; index < STF_GET_KAMAE_ROW_COUNT; ++index) {
        record[STF_GET_KAMAE_DESCRIPTOR_OFFSET + index] = UINT8_C(0x15);
    }
    for (index = 0u; index < 60u; ++index) {
        write_u32(record + 24u + index * 4u, float_bits((float)index));
    }
    write_u32(stance + destination - 12u, float_bits(7.0f));
    write_u32(stance + destination - 8u, float_bits(8.0f));
    write_u32(stance + destination - 4u, float_bits(9.0f));

    if (!stf_get_kamae_value_apply(
            record, sizeof(record), 0u,
            stance, sizeof(stance), destination, &result
        ) ||
        result.destination_end_offset != destination + 0xE4u ||
        result.conversion_start_offset != destination - 12u ||
        read_i16(stance + destination - 12u) != 7 ||
        read_i16(stance + destination - 8u) != 8 ||
        read_i16(stance + destination - 4u) != 9) {
        return 6;
    }

    memset(record, 0, sizeof(record));
    memset(stance, 0, sizeof(stance));
    for (index = 0u; index < STF_GET_KAMAE_ROW_COUNT; ++index) {
        record[STF_GET_KAMAE_DESCRIPTOR_OFFSET + index] = UINT8_C(0x15);
    }
    for (index = 0u; index < 60u; ++index) {
        write_u32(record + 24u + index * 4u, float_bits(1.75f));
    }

    if (!stf_get_kamae_value_apply(
            record, sizeof(record), UINT32_C(3) << 30u,
            stance, sizeof(stance), destination, &result
        ) ||
        read_i16(stance + destination) != 1) {
        return 7;
    }

    if (stf_get_kamae_value_apply(
            record, 21u, 0u,
            stance, sizeof(stance), destination, &result
        )) {
        return 8;
    }

    return 0;
}
