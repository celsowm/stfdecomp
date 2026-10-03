#ifndef STF_RECOVERY_I960_BUS_H
#define STF_RECOVERY_I960_BUS_H

#include <stddef.h>
#include <stdint.h>

#include "stf/recovery/status.h"

typedef stf_status (*stf_i960_bus_read_fn)(
    void *context,
    uint32_t address,
    void *output,
    size_t size
);

typedef stf_status (*stf_i960_bus_write_fn)(
    void *context,
    uint32_t address,
    const void *data,
    size_t size
);

typedef struct stf_i960_bus {
    const uint8_t *program_image;
    size_t program_size;
    void *context;
    stf_i960_bus_read_fn read;
    stf_i960_bus_write_fn write;
} stf_i960_bus;

stf_status stf_i960_bus_read(
    const stf_i960_bus *bus,
    uint32_t address,
    void *output,
    size_t size
);

stf_status stf_i960_bus_write(
    stf_i960_bus *bus,
    uint32_t address,
    const void *data,
    size_t size
);

stf_status stf_i960_bus_read_u32(
    const stf_i960_bus *bus,
    uint32_t address,
    uint32_t *value
);

stf_status stf_i960_bus_write_u32(
    stf_i960_bus *bus,
    uint32_t address,
    uint32_t value
);

#endif
