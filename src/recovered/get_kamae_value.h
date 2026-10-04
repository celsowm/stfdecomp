#ifndef STF_RECOVERED_GET_KAMAE_VALUE_H
#define STF_RECOVERED_GET_KAMAE_VALUE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_GET_KAMAE_ROW_COUNT = 20u,
    STF_GET_KAMAE_COMPONENT_COUNT = 3u,
    STF_GET_KAMAE_DESCRIPTOR_OFFSET = 2u,
    STF_GET_KAMAE_DESCRIPTOR_BYTES = 20u,
    STF_GET_KAMAE_COUNT_STREAM_OFFSET = 22u,
    STF_GET_KAMAE_OUTPUT_BYTES = 0xF0u,
    STF_GET_KAMAE_CVTRI_WORDS = 0x24u
};

typedef struct stf_get_kamae_value_result {
    size_t count_stream_bytes;
    size_t count_stream_sum;
    size_t payload_offset;
    size_t source_start_offset;
    size_t source_end_offset;
    size_t destination_end_offset;
    size_t conversion_start_offset;
} stf_get_kamae_value_result;

/*
 * Portable recovery of get_kamae_value at 0x2FD84..0x2FF08.
 *
 * motion_record is the object resolved from offset_list_motions[selector].
 * stance_ram is the object referenced by fighter +0xBD8; destination_offset is
 * the offset selected by set_kamae_ram.
 *
 * arithmetic_control supplies the i960 AC value used by the final cvtri pass;
 * bits 31:30 select nearest-even, floor, ceil, or truncate respectively.
 */
bool stf_get_kamae_value_apply(
    const uint8_t *motion_record,
    size_t motion_record_size,
    uint32_t arithmetic_control,
    uint8_t *stance_ram,
    size_t stance_ram_size,
    size_t destination_offset,
    stf_get_kamae_value_result *result
);

#endif
