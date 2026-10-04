#include "set_kamae_runtime.h"

#include <string.h>

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
)
{
    stf_set_kamae_plan plan;
    stf_set_kamae_runtime_result local;
    size_t index = 0u;

    if (selector_block == NULL || stance_ram == NULL || resolver == NULL) {
        return false;
    }
    if (!stf_set_kamae_plan_compute(
            fighter_flags_0,
            selector_block,
            selector_block_size,
            &plan
        )) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    local.request_count = plan.request_count;

    for (index = 0u; index < plan.request_count; ++index) {
        const uint8_t *motion_record = NULL;
        size_t motion_record_size = 0u;

        if (!resolver(
                plan.requests[index].selector,
                &motion_record,
                &motion_record_size,
                resolver_user_data
            ) ||
            motion_record == NULL ||
            !stf_get_kamae_value_apply(
                motion_record,
                motion_record_size,
                arithmetic_control,
                stance_ram,
                stance_ram_size,
                plan.requests[index].destination_offset,
                &local.values[index]
            )) {
            return false;
        }

        local.selectors[index] = plan.requests[index].selector;
        local.destination_offsets[index] =
            plan.requests[index].destination_offset;
    }

    if (result != NULL) {
        *result = local;
    }
    return true;
}
