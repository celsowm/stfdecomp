#ifndef STF_RECOVERED_SET_KAMAE_PLAN_H
#define STF_RECOVERED_SET_KAMAE_PLAN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_SET_KAMAE_MAX_REQUESTS = 4u,
    STF_SET_KAMAE_SHORT_SELECTOR_MIN_SIZE = 0x04u,
    STF_SET_KAMAE_FULL_SELECTOR_MIN_SIZE = 0x52u
};

typedef struct stf_set_kamae_request {
    uint16_t selector;
    uint16_t selector_offset;
    uint32_t destination_offset;
} stf_set_kamae_request;

typedef struct stf_set_kamae_plan {
    size_t request_count;
    stf_set_kamae_request requests[STF_SET_KAMAE_MAX_REQUESTS];
} stf_set_kamae_plan;

/*
 * Portable recovery of set_kamae_ram at 0x2F258..0x2F2AC.
 *
 * selector_block is the object referenced by fighter +0x1A0. Destination
 * offsets are relative to the stance-RAM object referenced by fighter +0xBD8.
 *
 * Normal path:
 *   selector +0x00 -> destination +0x1E0
 *   selector +0x08 -> destination +0x2D0
 *   selector +0x0A -> destination +0x3C0
 *   selector +0x50 -> destination +0x5A0
 *
 * If fighter flags bit 29 is set:
 *   selector +0x02 -> destination +0x1E0
 *
 * Each request corresponds to one get_kamae_value call. This helper recovers
 * the wrapper/orchestration only; get_kamae_value remains a separate contract.
 */
bool stf_set_kamae_plan_compute(
    uint32_t fighter_flags_0,
    const uint8_t *selector_block,
    size_t selector_block_size,
    stf_set_kamae_plan *result
);

#endif
