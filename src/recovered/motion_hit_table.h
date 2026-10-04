#ifndef STF_RECOVERED_MOTION_HIT_TABLE_H
#define STF_RECOVERED_MOTION_HIT_TABLE_H

#include <stddef.h>
#include <stdint.h>

enum {
    STF_MOTION_HIT_RECORD_SIZE_COUNT = 25u
};

typedef enum stf_motion_hit_lookup_status {
    STF_MOTION_HIT_FOUND = 0,
    STF_MOTION_HIT_NOT_FOUND,
    STF_MOTION_HIT_INVALID
} stf_motion_hit_lookup_status;

/*
 * Canonical byte_1D006 stride table used by calc_mht_adr.
 */
const uint8_t *stf_motion_hit_default_record_sizes(size_t *count);

/*
 * Portable recovery of calc_mht_adr.
 *
 * animation_offsets contains offsets into motion_blob rather than Model 2
 * absolute addresses. The original selector is masked with 0x1fff, then the
 * selected animation record starts at +0x0d. Each record's first byte is its
 * tag; byte 0 or 8 terminates the search. record_size_by_tag is the recovered
 * byte_1D006 stride table.
 */
stf_motion_hit_lookup_status stf_motion_hit_table_find_offset(
    uint32_t selector,
    uint8_t target_tag,
    const uint32_t *animation_offsets,
    size_t animation_count,
    const uint8_t *motion_blob,
    size_t motion_blob_size,
    const uint8_t *record_size_by_tag,
    size_t record_size_count,
    uint32_t *record_offset
);

#endif
