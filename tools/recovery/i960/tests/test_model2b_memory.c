#include <stdint.h>

#include "stf/recovery/i960/bus.h"
#include "stf/recovery/model2b_bus.h"
#include "stf/recovery/model2b_map.h"

int main(void)
{
    stf_model2b_bus model2b;
    stf_i960_bus *bus = NULL;
    uint8_t main_data[32] = {0u};
    uint8_t main_data_ep[32] = {0u};
    uint32_t value = 0u;
    const stf_model2b_fault *fault = NULL;

    if (stf_model2b_bus_init(&model2b) != STF_OK) {
        return 1;
    }
    bus = stf_model2b_bus_i960(&model2b);

    main_data[4] = 0x78u;
    main_data[5] = 0x56u;
    main_data[6] = 0x34u;
    main_data[7] = 0x12u;
    main_data_ep[8] = 0xefu;
    main_data_ep[9] = 0xbeu;
    main_data_ep[10] = 0xadu;
    main_data_ep[11] = 0xdeu;

    if (stf_model2b_bus_attach_main_data(
            &model2b,
            main_data,
            sizeof(main_data)
        ) != STF_OK ||
        stf_model2b_bus_attach_main_data_ep(
            &model2b,
            main_data_ep,
            sizeof(main_data_ep)
        ) != STF_OK) {
        stf_model2b_bus_destroy(&model2b);
        return 2;
    }

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

    if (stf_i960_bus_read_u32(
            bus,
            STF_MODEL2B_MAIN_DATA_START + 4u,
            &value
        ) != STF_OK ||
        value != UINT32_C(0x12345678)) {
        stf_model2b_bus_destroy(&model2b);
        return 6;
    }

    if (stf_i960_bus_read_u32(
            bus,
            STF_MODEL2B_MAIN_DATA_EP_START + 8u,
            &value
        ) != STF_OK ||
        value != UINT32_C(0xDEADBEEF)) {
        stf_model2b_bus_destroy(&model2b);
        return 7;
    }

    if (stf_i960_bus_read_u32(
            bus,
            STF_MODEL2B_MAIN_DATA_EP_MIRROR_START + 8u,
            &value
        ) != STF_OK ||
        value != UINT32_C(0xDEADBEEF)) {
        stf_model2b_bus_destroy(&model2b);
        return 8;
    }

    if (stf_i960_bus_write_u32(
            bus,
            STF_MODEL2B_STAGE_PALETTE_DATA + 0x20u,
            UINT32_C(0xCAFEBABE)
        ) != STF_OK ||
        stf_i960_bus_read_u32(
            bus,
            STF_MODEL2B_STAGE_PALETTE_DATA + 0x20u,
            &value
        ) != STF_OK ||
        value != UINT32_C(0xCAFEBABE)) {
        stf_model2b_bus_destroy(&model2b);
        return 9;
    }

    if (stf_i960_bus_read_u32(
            bus,
            STF_MODEL2B_BACKUP_RAM_START,
            &value
        ) != STF_OK ||
        value != UINT32_C(0xFFFFFFFF)) {
        stf_model2b_bus_destroy(&model2b);
        return 10;
    }

    if (stf_i960_bus_write_u32(
            bus,
            STF_MODEL2B_TEXTURE0_START + 0x100u,
            UINT32_C(0xA1B2C3D4)
        ) != STF_OK ||
        stf_i960_bus_read_u32(
            bus,
            STF_MODEL2B_TEXTURE0_START + 0x100u,
            &value
        ) != STF_OK ||
        value != UINT32_C(0xA1B2C3D4)) {
        stf_model2b_bus_destroy(&model2b);
        return 11;
    }

    stf_model2b_bus_clear_fault(&model2b);
    if (stf_i960_bus_read_u32(
            bus,
            STF_MODEL2B_COPRO_CONTROL1_START,
            &value
        ) != STF_ERROR_UNSUPPORTED) {
        stf_model2b_bus_destroy(&model2b);
        return 12;
    }

    fault = stf_model2b_bus_last_fault(&model2b);
    if (fault == NULL || !fault->valid || fault->write ||
        fault->address != STF_MODEL2B_COPRO_CONTROL1_START ||
        fault->size != 4u) {
        stf_model2b_bus_destroy(&model2b);
        return 13;
    }

    stf_model2b_bus_destroy(&model2b);
    return 0;
}
