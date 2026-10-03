#ifndef STF_RECOVERY_I960_BUS_H
#define STF_RECOVERY_I960_BUS_H

#include <stddef.h>
#include <stdint.h>

#include "stf/recovery/status.h"

enum {
    STF_I960_BUS_TRACE_MAX_BYTES = 16u
};

typedef enum stf_i960_bus_access_kind {
    STF_I960_BUS_ACCESS_READ = 0,
    STF_I960_BUS_ACCESS_WRITE
} stf_i960_bus_access_kind;

typedef struct stf_i960_bus_trace_event {
    uint64_t step;
    stf_i960_bus_access_kind kind;
    uint32_t address;
    size_t size;
    stf_status status;
    uint8_t bytes[STF_I960_BUS_TRACE_MAX_BYTES];
    size_t byte_count;
} stf_i960_bus_trace_event;

typedef void (*stf_i960_bus_trace_callback)(
    const stf_i960_bus_trace_event *event,
    void *user_data
);

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
    uint64_t trace_step;
    stf_i960_bus_trace_callback trace_callback;
    void *trace_user_data;
} stf_i960_bus;

void stf_i960_bus_set_trace(
    stf_i960_bus *bus,
    stf_i960_bus_trace_callback callback,
    void *user_data
);

void stf_i960_bus_set_trace_step(stf_i960_bus *bus, uint64_t step);

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
