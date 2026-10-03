#include <stdint.h>

#include "stf/recovery/i960/bus.h"
#include "stf/recovery/model2b_bus.h"
#include "stf/recovery/model2b_map.h"

int main(void)
{
    stf_model2b_bus model2b;
    stf_i960_bus *bus = NULL;
    uint32_t value = 0u;
    const stf_model2b_fault *fault = NULL;

    if (stf_model2b_bus_init(&model2b) != STF_OK) {
        return 1;
    }
    bus = stf_model2b_bus_i960(&model2b);

    if (stf_i960_bus_write_u32(
            bus,
            STF_MODEL2B_BUFF_RAM_02 + 0x10u,
            UINT32_C(0xAABBCCDD)
        ) != STF_OK) {
        stf_model2b_bus_destroy(&model2b);
        return 2;
    }
    if (stf_i960_bus_read_u32(
            bus,
            STF_MODEL2B_BUFF_RAM_02 + 0x10u,
            &value
        ) != STF_OK ||
        value != UINT32_C(0xAABBCCDD)) {
        stf_model2b_bus_destroy(&model2b);
        return 3;
    }

    stf_model2b_bus_clear_fault(&model2b);
    if (stf_i960_bus_write_u32(
            bus,
            STF_MODEL2B_GEO_START + 0x20u,
            UINT32_C(0x11223344)
        ) != STF_ERROR_UNSUPPORTED) {
        stf_model2b_bus_destroy(&model2b);
        return 4;
    }

    fault = stf_model2b_bus_last_fault(&model2b);
    if (fault == NULL || !fault->valid || !fault->write ||
        fault->address != STF_MODEL2B_GEO_START + 0x20u ||
        fault->size != 4u) {
        stf_model2b_bus_destroy(&model2b);
        return 5;
    }

    stf_model2b_bus_clear_fault(&model2b);
    if (stf_i960_bus_read_u32(
            bus,
            STF_MODEL2B_COPRO_CONTROL1_START,
            &value
        ) != STF_ERROR_UNSUPPORTED) {
        stf_model2b_bus_destroy(&model2b);
        return 6;
    }

    fault = stf_model2b_bus_last_fault(&model2b);
    if (fault == NULL || !fault->valid || fault->write ||
        fault->address != STF_MODEL2B_COPRO_CONTROL1_START ||
        fault->size != 4u) {
        stf_model2b_bus_destroy(&model2b);
        return 7;
    }

    stf_model2b_bus_destroy(&model2b);
    return 0;
}
