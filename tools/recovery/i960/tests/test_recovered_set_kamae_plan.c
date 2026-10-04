#include <stdint.h>
#include <string.h>

#include "set_kamae_plan.h"

static void write_le16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
}

int main(void)
{
    uint8_t selectors[STF_SET_KAMAE_FULL_SELECTOR_MIN_SIZE];
    stf_set_kamae_plan plan;

    memset(selectors, 0, sizeof(selectors));
    write_le16(selectors + 0x00u, UINT16_C(0x1111));
    write_le16(selectors + 0x02u, UINT16_C(0x5555));
    write_le16(selectors + 0x08u, UINT16_C(0x2222));
    write_le16(selectors + 0x0Au, UINT16_C(0x3333));
    write_le16(selectors + 0x50u, UINT16_C(0x4444));

    if (!stf_set_kamae_plan_compute(
            0u, selectors, sizeof(selectors), &plan
        ) ||
        plan.request_count != 4u ||
        plan.requests[0].selector != UINT16_C(0x1111) ||
        plan.requests[0].selector_offset != UINT16_C(0x00) ||
        plan.requests[0].destination_offset != UINT32_C(0x1E0) ||
        plan.requests[1].selector != UINT16_C(0x2222) ||
        plan.requests[1].selector_offset != UINT16_C(0x08) ||
        plan.requests[1].destination_offset != UINT32_C(0x2D0) ||
        plan.requests[2].selector != UINT16_C(0x3333) ||
        plan.requests[2].selector_offset != UINT16_C(0x0A) ||
        plan.requests[2].destination_offset != UINT32_C(0x3C0) ||
        plan.requests[3].selector != UINT16_C(0x4444) ||
        plan.requests[3].selector_offset != UINT16_C(0x50) ||
        plan.requests[3].destination_offset != UINT32_C(0x5A0)) {
        return 1;
    }

    if (!stf_set_kamae_plan_compute(
            UINT32_C(1) << 29u,
            selectors,
            STF_SET_KAMAE_SHORT_SELECTOR_MIN_SIZE,
            &plan
        ) ||
        plan.request_count != 1u ||
        plan.requests[0].selector != UINT16_C(0x5555) ||
        plan.requests[0].selector_offset != UINT16_C(0x02) ||
        plan.requests[0].destination_offset != UINT32_C(0x1E0)) {
        return 2;
    }

    if (stf_set_kamae_plan_compute(
            0u,
            selectors,
            STF_SET_KAMAE_FULL_SELECTOR_MIN_SIZE - 1u,
            &plan
        )) {
        return 3;
    }

    if (stf_set_kamae_plan_compute(
            UINT32_C(1) << 29u,
            selectors,
            STF_SET_KAMAE_SHORT_SELECTOR_MIN_SIZE - 1u,
            &plan
        )) {
        return 4;
    }

    return 0;
}
