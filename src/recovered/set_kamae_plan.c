#include "set_kamae_plan.h"

#include <string.h>

static uint16_t read_le16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8u));
}

static void set_request(
    stf_set_kamae_request *request,
    const uint8_t *selector_block,
    uint16_t selector_offset,
    uint32_t destination_offset
)
{
    request->selector = read_le16(selector_block + selector_offset);
    request->selector_offset = selector_offset;
    request->destination_offset = destination_offset;
}

bool stf_set_kamae_plan_compute(
    uint32_t fighter_flags_0,
    const uint8_t *selector_block,
    size_t selector_block_size,
    stf_set_kamae_plan *result
)
{
    stf_set_kamae_plan local;

    if (selector_block == NULL || result == NULL) {
        return false;
    }

    memset(&local, 0, sizeof(local));

    if ((fighter_flags_0 & (UINT32_C(1) << 29u)) != 0u) {
        if (selector_block_size < STF_SET_KAMAE_SHORT_SELECTOR_MIN_SIZE) {
            return false;
        }

        local.request_count = 1u;
        set_request(
            &local.requests[0],
            selector_block,
            UINT16_C(0x02),
            UINT32_C(0x1E0)
        );
        *result = local;
        return true;
    }

    if (selector_block_size < STF_SET_KAMAE_FULL_SELECTOR_MIN_SIZE) {
        return false;
    }

    local.request_count = 4u;
    set_request(
        &local.requests[0],
        selector_block,
        UINT16_C(0x00),
        UINT32_C(0x1E0)
    );
    set_request(
        &local.requests[1],
        selector_block,
        UINT16_C(0x08),
        UINT32_C(0x2D0)
    );
    set_request(
        &local.requests[2],
        selector_block,
        UINT16_C(0x0A),
        UINT32_C(0x3C0)
    );
    set_request(
        &local.requests[3],
        selector_block,
        UINT16_C(0x50),
        UINT32_C(0x5A0)
    );

    *result = local;
    return true;
}
