#ifndef STF_RECOVERY_MODEL2B_BUS_H
#define STF_RECOVERY_MODEL2B_BUS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "stf/recovery/i960/bus.h"

enum {
    STF_MODEL2B_ROM_BASE = 0x00000000u,
    STF_MODEL2B_WORK_RAM_BASE = 0x00500000u,
    STF_MODEL2B_WORK_RAM_SIZE = 0x00100000u
};

typedef stf_status (*stf_model2b_device_read_fn)(
    void *context,
    uint32_t address,
    void *output,
    size_t size
);

typedef stf_status (*stf_model2b_device_write_fn)(
    void *context,
    uint32_t address,
    const void *data,
    size_t size
);

typedef struct stf_model2b_fault {
    bool valid;
    bool write;
    uint32_t address;
    size_t size;
    stf_status status;
} stf_model2b_fault;

typedef struct stf_model2b_bus {
    stf_i960_bus i960;
    uint8_t *work_ram;
    size_t work_ram_size;
    const uint8_t *rom;
    size_t rom_size;
    void *device_context;
    stf_model2b_device_read_fn device_read;
    stf_model2b_device_write_fn device_write;
    stf_model2b_fault last_fault;
} stf_model2b_bus;

stf_status stf_model2b_bus_init(stf_model2b_bus *model2b);
void stf_model2b_bus_destroy(stf_model2b_bus *model2b);

stf_status stf_model2b_bus_attach_program(
    stf_model2b_bus *model2b,
    const uint8_t *rom,
    size_t rom_size
);

void stf_model2b_bus_set_device_callbacks(
    stf_model2b_bus *model2b,
    void *context,
    stf_model2b_device_read_fn read_callback,
    stf_model2b_device_write_fn write_callback
);

void stf_model2b_bus_clear_fault(stf_model2b_bus *model2b);

const stf_model2b_fault *stf_model2b_bus_last_fault(
    const stf_model2b_bus *model2b
);

stf_i960_bus *stf_model2b_bus_i960(stf_model2b_bus *model2b);
const stf_i960_bus *stf_model2b_bus_i960_const(const stf_model2b_bus *model2b);

#endif
