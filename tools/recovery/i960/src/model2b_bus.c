#include "stf/recovery/model2b_bus.h"

#include <stdlib.h>
#include <string.h>

static int range_contains(uint32_t base, size_t region_size, uint32_t address, size_t size)
{
    uint64_t region_end = (uint64_t)base + (uint64_t)region_size;
    uint64_t access_end = (uint64_t)address + (uint64_t)size;
    return (uint64_t)address >= (uint64_t)base && access_end <= region_end;
}

static stf_status model2b_read(
    void *context,
    uint32_t address,
    void *output,
    size_t size
)
{
    stf_model2b_bus *model2b = (stf_model2b_bus *)context;

    if (model2b == NULL || output == NULL) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    if (size == 0u) {
        return STF_OK;
    }
    if (model2b->rom != NULL &&
        range_contains(STF_MODEL2B_ROM_BASE, model2b->rom_size, address, size)) {
        memcpy(output, model2b->rom + (address - STF_MODEL2B_ROM_BASE), size);
        return STF_OK;
    }
    if (model2b->work_ram != NULL &&
        range_contains(
            STF_MODEL2B_WORK_RAM_BASE,
            model2b->work_ram_size,
            address,
            size
        )) {
        memcpy(
            output,
            model2b->work_ram + (address - STF_MODEL2B_WORK_RAM_BASE),
            size
        );
        return STF_OK;
    }
    if (model2b->device_read != NULL) {
        return model2b->device_read(
            model2b->device_context,
            address,
            output,
            size
        );
    }
    return STF_ERROR_UNSUPPORTED;
}

static stf_status model2b_write(
    void *context,
    uint32_t address,
    const void *data,
    size_t size
)
{
    stf_model2b_bus *model2b = (stf_model2b_bus *)context;

    if (model2b == NULL || data == NULL) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    if (size == 0u) {
        return STF_OK;
    }
    if (model2b->work_ram != NULL &&
        range_contains(
            STF_MODEL2B_WORK_RAM_BASE,
            model2b->work_ram_size,
            address,
            size
        )) {
        memcpy(
            model2b->work_ram + (address - STF_MODEL2B_WORK_RAM_BASE),
            data,
            size
        );
        return STF_OK;
    }
    if (range_contains(STF_MODEL2B_ROM_BASE, model2b->rom_size, address, size)) {
        return STF_ERROR_UNSUPPORTED;
    }
    if (model2b->device_write != NULL) {
        return model2b->device_write(
            model2b->device_context,
            address,
            data,
            size
        );
    }
    return STF_ERROR_UNSUPPORTED;
}

stf_status stf_model2b_bus_init(stf_model2b_bus *model2b)
{
    if (model2b == NULL) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    memset(model2b, 0, sizeof(*model2b));
    model2b->work_ram = (uint8_t *)calloc(1u, STF_MODEL2B_WORK_RAM_SIZE);
    if (model2b->work_ram == NULL) {
        return STF_ERROR_OUT_OF_MEMORY;
    }
    model2b->work_ram_size = STF_MODEL2B_WORK_RAM_SIZE;
    model2b->i960.context = model2b;
    model2b->i960.read = model2b_read;
    model2b->i960.write = model2b_write;
    return STF_OK;
}

void stf_model2b_bus_destroy(stf_model2b_bus *model2b)
{
    if (model2b == NULL) {
        return;
    }
    free(model2b->work_ram);
    memset(model2b, 0, sizeof(*model2b));
}

stf_status stf_model2b_bus_attach_program(
    stf_model2b_bus *model2b,
    const uint8_t *rom,
    size_t rom_size
)
{
    if (model2b == NULL || rom == NULL || rom_size == 0u) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    model2b->rom = rom;
    model2b->rom_size = rom_size;
    model2b->i960.program_image = rom;
    model2b->i960.program_size = rom_size;
    return STF_OK;
}

void stf_model2b_bus_set_device_callbacks(
    stf_model2b_bus *model2b,
    void *context,
    stf_model2b_device_read_fn read_callback,
    stf_model2b_device_write_fn write_callback
)
{
    if (model2b == NULL) {
        return;
    }
    model2b->device_context = context;
    model2b->device_read = read_callback;
    model2b->device_write = write_callback;
}

stf_i960_bus *stf_model2b_bus_i960(stf_model2b_bus *model2b)
{
    return model2b != NULL ? &model2b->i960 : NULL;
}

const stf_i960_bus *stf_model2b_bus_i960_const(const stf_model2b_bus *model2b)
{
    return model2b != NULL ? &model2b->i960 : NULL;
}
