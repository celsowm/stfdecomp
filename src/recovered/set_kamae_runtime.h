#ifndef STF_RECOVERED_SET_KAMAE_RUNTIME_H
#define STF_RECOVERED_SET_KAMAE_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "get_kamae_value.h"
#include "set_kamae_plan.h"

typedef bool (*stf_kamae_motion_resolver)(
    uint16_t selector,
    const uint8_t **motion_record,
    size_t *motion_record_size,
    void *user_data
);

typedef struct stf_set_kamae_runtime_result {
    size_t request_count;
    uint16_t selectors[STF_SET_KAMAE_MAX_REQUESTS];
    uint32_t destination_offsets[STF_SET_KAMAE_MAX_REQUESTS];
    stf_get_kamae_value_result values[STF_SET_KAMAE_MAX_REQUESTS];
} stf_set_kamae_runtime_result;

/*
 * Compose set_kamae_ram with get_kamae_value.
 *
 * The original offset_list_motions lookup stays outside this portable layer:
 * the caller resolves each selector to its motion record through resolver.
 */
bool stf_set_kamae_runtime_apply(
    uint32_t fighter_flags_0,
    const uint8_t *selector_block,
    size_t selector_block_size,
    uint32_t arithmetic_control,
    uint8_t *stance_ram,
    size_t stance_ram_size,
    stf_kamae_motion_resolver resolver,
    void *resolver_user_data,
    stf_set_kamae_runtime_result *result
);

#endif
