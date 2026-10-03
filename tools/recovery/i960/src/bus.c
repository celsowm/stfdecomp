#include "stf/recovery/i960/bus.h"

#include <string.h>

static void emit_trace(
    const stf_i960_bus *bus,
    stf_i960_bus_access_kind kind,
    uint32_t address,
    size_t size,
    stf_status status,
    const void *data,
    int data_valid
)
{
    stf_i960_bus_trace_event event;
    size_t copy_size = 0u;

    if (bus == NULL || bus->trace_callback == NULL) {
        return;
    }

    memset(&event, 0, sizeof(event));
    event.step = bus->trace_step;
    event.kind = kind;
    event.address = address;
    event.size = size;
    event.status = status;

    if (data_valid != 0 && data != NULL) {
        copy_size = size < STF_I960_BUS_TRACE_MAX_BYTES
            ? size
            : STF_I960_BUS_TRACE_MAX_BYTES;
        memcpy(event.bytes, data, copy_size);
        event.byte_count = copy_size;
    }

    bus->trace_callback(&event, bus->trace_user_data);
}

void stf_i960_bus_set_trace(
    stf_i960_bus *bus,
    stf_i960_bus_trace_callback callback,
    void *user_data
)
{
    if (bus == NULL) {
        return;
    }
    bus->trace_callback = callback;
    bus->trace_user_data = user_data;
}

void stf_i960_bus_set_trace_step(stf_i960_bus *bus, uint64_t step)
{
    if (bus != NULL) {
        bus->trace_step = step;
    }
}

stf_status stf_i960_bus_read(
    const stf_i960_bus *bus,
    uint32_t address,
    void *output,
    size_t size
)
{
    stf_status status = STF_OK;

    if (bus == NULL || output == NULL || bus->read == NULL) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    if (size == 0u) {
        return STF_OK;
    }

    status = bus->read(bus->context, address, output, size);
    emit_trace(
        bus,
        STF_I960_BUS_ACCESS_READ,
        address,
        size,
        status,
        output,
        status == STF_OK
    );
    return status;
}

stf_status stf_i960_bus_write(
    stf_i960_bus *bus,
    uint32_t address,
    const void *data,
    size_t size
)
{
    stf_status status = STF_OK;

    if (bus == NULL || data == NULL || bus->write == NULL) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    if (size == 0u) {
        return STF_OK;
    }

    status = bus->write(bus->context, address, data, size);
    emit_trace(
        bus,
        STF_I960_BUS_ACCESS_WRITE,
        address,
        size,
        status,
        data,
        1
    );
    return status;
}

stf_status stf_i960_bus_read_u32(
    const stf_i960_bus *bus,
    uint32_t address,
    uint32_t *value
)
{
    uint8_t data[4];
    stf_status status = STF_OK;
    if (value == NULL) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    status = stf_i960_bus_read(bus, address, data, sizeof(data));
    if (status != STF_OK) {
        return status;
    }
    *value = (uint32_t)data[0] |
             ((uint32_t)data[1] << 8u) |
             ((uint32_t)data[2] << 16u) |
             ((uint32_t)data[3] << 24u);
    return STF_OK;
}

stf_status stf_i960_bus_write_u32(
    stf_i960_bus *bus,
    uint32_t address,
    uint32_t value
)
{
    uint8_t data[4];
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
    return stf_i960_bus_write(bus, address, data, sizeof(data));
}
