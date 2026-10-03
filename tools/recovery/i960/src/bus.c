#include "stf/recovery/i960/bus.h"

stf_status stf_i960_bus_read(
    const stf_i960_bus *bus,
    uint32_t address,
    void *output,
    size_t size
)
{
    if (bus == NULL || output == NULL || bus->read == NULL) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    if (size == 0u) {
        return STF_OK;
    }
    return bus->read(bus->context, address, output, size);
}

stf_status stf_i960_bus_write(
    stf_i960_bus *bus,
    uint32_t address,
    const void *data,
    size_t size
)
{
    if (bus == NULL || data == NULL || bus->write == NULL) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    if (size == 0u) {
        return STF_OK;
    }
    return bus->write(bus->context, address, data, size);
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
