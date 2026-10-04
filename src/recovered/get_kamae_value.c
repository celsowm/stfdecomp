#include "get_kamae_value.h"

#include <limits.h>
#include <math.h>
#include <string.h>

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

static float bits_to_float(uint32_t bits)
{
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static bool add_size(size_t left, size_t right, size_t *result)
{
    if (result == NULL || right > SIZE_MAX - left) {
        return false;
    }
    *result = left + right;
    return true;
}

static bool mul_size(size_t left, size_t right, size_t *result)
{
    if (result == NULL || (left != 0u && right > SIZE_MAX / left)) {
        return false;
    }
    *result = left * right;
    return true;
}

static bool copy_bytes(
    const uint8_t *source,
    size_t source_size,
    size_t source_offset,
    uint8_t *destination,
    size_t destination_size,
    size_t destination_offset,
    size_t size
)
{
    if (source_offset > source_size || size > source_size - source_offset ||
        destination_offset > destination_size ||
        size > destination_size - destination_offset) {
        return false;
    }
    memcpy(destination + destination_offset, source + source_offset, size);
    return true;
}

static uint8_t decode_component_code(uint8_t descriptor, unsigned component)
{
    const uint8_t row_code = (uint8_t)((descriptor >> 6u) & 3u);
    if (row_code != 0u) {
        return (uint8_t)(row_code - 1u);
    }
    return (uint8_t)(((descriptor >> (component * 2u)) & 3u) + 3u);
}

static bool cvtri(
    uint32_t bits,
    uint32_t arithmetic_control,
    int32_t *result
)
{
    const float value = bits_to_float(bits);
    const double input = (double)value;
    double rounded = 0.0;
    const unsigned mode = (unsigned)((arithmetic_control >> 30u) & 3u);

    if (result == NULL || !isfinite(value)) {
        return false;
    }

    switch (mode) {
    case 0u: {
        const double lower = floor(input);
        const double fraction = input - lower;
        if (fraction < 0.5) {
            rounded = lower;
        } else if (fraction > 0.5) {
            rounded = lower + 1.0;
        } else {
            rounded = fmod(fabs(lower), 2.0) == 0.0
                ? lower : lower + 1.0;
        }
        break;
    }
    case 1u:
        rounded = floor(input);
        break;
    case 2u:
        rounded = ceil(input);
        break;
    case 3u:
    default:
        rounded = trunc(input);
        break;
    }

    if (rounded < (double)INT32_MIN || rounded > (double)INT32_MAX) {
        return false;
    }
    *result = (int32_t)rounded;
    return true;
}

bool stf_get_kamae_value_apply(
    const uint8_t *motion_record,
    size_t motion_record_size,
    uint32_t arithmetic_control,
    uint8_t *stance_ram,
    size_t stance_ram_size,
    size_t destination_offset,
    stf_get_kamae_value_result *result
)
{
    uint8_t codes[STF_GET_KAMAE_ROW_COUNT][STF_GET_KAMAE_COMPONENT_COUNT];
    stf_get_kamae_value_result local;
    size_t count_cursor = STF_GET_KAMAE_COUNT_STREAM_OFFSET;
    size_t count_count = 0u;
    size_t count_sum = 0u;
    size_t cursor = 0u;
    size_t payload_offset = 0u;
    size_t source_offset = 0u;
    size_t destination_cursor = destination_offset;
    size_t row = 0u;

    if (motion_record == NULL || stance_ram == NULL ||
        motion_record_size < STF_GET_KAMAE_COUNT_STREAM_OFFSET ||
        destination_offset > stance_ram_size) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    memset(codes, 0, sizeof(codes));

    for (row = 0u; row < STF_GET_KAMAE_ROW_COUNT; ++row) {
        const uint8_t descriptor =
            motion_record[STF_GET_KAMAE_DESCRIPTOR_OFFSET + row];
        unsigned component = 0u;
        for (component = 0u; component < STF_GET_KAMAE_COMPONENT_COUNT;
             ++component) {
            const uint8_t code = decode_component_code(descriptor, component);
            codes[row][component] = code;
            if (code > 4u) {
                size_t next_sum = 0u;
                if (count_cursor >= motion_record_size ||
                    !add_size(count_sum, motion_record[count_cursor], &next_sum)) {
                    return false;
                }
                count_sum = next_sum;
                ++count_cursor;
                ++count_count;
            }
        }
    }

    cursor = count_cursor;
    if (!add_size(cursor, 3u, &payload_offset)) {
        return false;
    }
    payload_offset &= ~(size_t)3u;

    {
        size_t skip = 0u;
        if (!mul_size(count_sum, 4u, &skip) ||
            !add_size(payload_offset, skip, &source_offset) ||
            source_offset > motion_record_size) {
            return false;
        }
    }

    local.count_stream_bytes = count_count;
    local.count_stream_sum = count_sum;
    local.payload_offset = payload_offset;
    local.source_start_offset = source_offset;

    count_cursor = STF_GET_KAMAE_COUNT_STREAM_OFFSET;

    for (row = 0u; row < STF_GET_KAMAE_ROW_COUNT; ++row) {
        unsigned component = 0u;
        for (component = 0u; component < STF_GET_KAMAE_COMPONENT_COUNT;
             ++component) {
            const uint8_t code = codes[row][component];

            if (code > 5u) {
                size_t delta = 0u;
                if (!copy_bytes(
                        motion_record, motion_record_size, source_offset,
                        stance_ram, stance_ram_size, destination_cursor, 4u
                    ) ||
                    !add_size(destination_cursor, 4u, &destination_cursor) ||
                    count_cursor >= motion_record_size ||
                    !mul_size(motion_record[count_cursor], 12u, &delta) ||
                    !add_size(source_offset, delta, &source_offset) ||
                    source_offset > motion_record_size) {
                    return false;
                }
                ++count_cursor;
                continue;
            }

            if (code == 5u) {
                size_t delta = 0u;
                if (!copy_bytes(
                        motion_record, motion_record_size, source_offset,
                        stance_ram, stance_ram_size, destination_cursor, 4u
                    ) ||
                    !add_size(destination_cursor, 4u, &destination_cursor) ||
                    count_cursor >= motion_record_size ||
                    !mul_size(motion_record[count_cursor], 4u, &delta) ||
                    !add_size(source_offset, delta, &source_offset) ||
                    source_offset > motion_record_size) {
                    return false;
                }
                ++count_cursor;
                continue;
            }

            if (code == 4u) {
                if (!copy_bytes(
                        motion_record, motion_record_size, source_offset,
                        stance_ram, stance_ram_size, destination_cursor, 4u
                    ) ||
                    !add_size(source_offset, 4u, &source_offset) ||
                    !add_size(destination_cursor, 4u, &destination_cursor)) {
                    return false;
                }
                continue;
            }

            if (code == 3u) {
                if (destination_cursor > stance_ram_size ||
                    4u > stance_ram_size - destination_cursor) {
                    return false;
                }
                write_le32(stance_ram + destination_cursor, 0u);
                if (!add_size(destination_cursor, 4u, &destination_cursor)) {
                    return false;
                }
                continue;
            }

            if (code == 2u) {
                break;
            }

            if (!copy_bytes(
                    motion_record, motion_record_size, source_offset,
                    stance_ram, stance_ram_size, destination_cursor, 12u
                ) ||
                !add_size(source_offset, 12u, &source_offset) ||
                !add_size(destination_cursor, 12u, &destination_cursor)) {
                return false;
            }
            break;
        }
    }

    if (destination_cursor < STF_GET_KAMAE_OUTPUT_BYTES) {
        return false;
    }
    local.destination_end_offset = destination_cursor;
    local.source_end_offset = source_offset;
    local.conversion_start_offset =
        destination_cursor - STF_GET_KAMAE_OUTPUT_BYTES;

    for (cursor = 0u; cursor < STF_GET_KAMAE_CVTRI_WORDS; ++cursor) {
        size_t word_offset = 0u;
        int32_t converted = 0;
        if (!mul_size(cursor, 4u, &word_offset) ||
            !add_size(local.conversion_start_offset, word_offset, &word_offset) ||
            word_offset > stance_ram_size ||
            4u > stance_ram_size - word_offset ||
            !cvtri(read_le32(stance_ram + word_offset), arithmetic_control, &converted)) {
            return false;
        }
        write_le16(stance_ram + word_offset, (uint16_t)converted);
    }

    if (result != NULL) {
        *result = local;
    }
    return true;
}
