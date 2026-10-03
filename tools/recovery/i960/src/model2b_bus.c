#include "stf/recovery/model2b_bus.h"

#include <stdlib.h>
#include <string.h>

static int range_contains(
    uint32_t base,
    size_t region_size,
    uint32_t address,
    size_t size
)
{
    const uint64_t region_end = (uint64_t)base + (uint64_t)region_size;
    const uint64_t access_end = (uint64_t)address + (uint64_t)size;
    return (uint64_t)address >= (uint64_t)base && access_end <= region_end;
}

static void record_fault(
    stf_model2b_bus *model2b,
    int write,
    uint32_t address,
    size_t size,
    stf_status status
)
{
    if (model2b == NULL) {
        return;
    }
    model2b->last_fault.valid = true;
    model2b->last_fault.write = write != 0;
    model2b->last_fault.address = address;
    model2b->last_fault.size = size;
    model2b->last_fault.status = status;
}

static stf_status read_storage(
    const uint8_t *storage,
    size_t storage_size,
    uint32_t base,
    uint32_t address,
    void *output,
    size_t size
)
{
    if (storage == NULL ||
        !range_contains(base, storage_size, address, size)) {
        return STF_ERROR_OUT_OF_BOUNDS;
    }
    memcpy(output, storage + (address - base), size);
    return STF_OK;
}

static stf_status write_storage(
    uint8_t *storage,
    size_t storage_size,
    uint32_t base,
    uint32_t address,
    const void *data,
    size_t size
)
{
    if (storage == NULL ||
        !range_contains(base, storage_size, address, size)) {
        return STF_ERROR_OUT_OF_BOUNDS;
    }
    memcpy(storage + (address - base), data, size);
    return STF_OK;
}

static int read_attached_rom(
    const uint8_t *data,
    size_t data_size,
    uint32_t base,
    uint32_t address,
    void *output,
    size_t size
)
{
    if (data == NULL || !range_contains(base, data_size, address, size)) {
        return 0;
    }
    memcpy(output, data + (address - base), size);
    return 1;
}

static int write_hits_attached_rom(
    const uint8_t *data,
    size_t data_size,
    uint32_t base,
    uint32_t address,
    size_t size
)
{
    return data != NULL && range_contains(base, data_size, address, size);
}

static stf_status model2b_read(
    void *context,
    uint32_t address,
    void *output,
    size_t size
)
{
    stf_model2b_bus *model2b = (stf_model2b_bus *)context;
    stf_status status = STF_ERROR_UNSUPPORTED;

    if (model2b == NULL || output == NULL) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    if (size == 0u) {
        return STF_OK;
    }

    if (read_attached_rom(
            model2b->rom,
            model2b->rom_size,
            STF_MODEL2B_ROM_BASE,
            address,
            output,
            size
        )) {
        return STF_OK;
    }

    if (range_contains(
            STF_MODEL2B_WORK_RAM_BASE,
            model2b->work_ram_size,
            address,
            size
        )) {
        return read_storage(
            model2b->work_ram,
            model2b->work_ram_size,
            STF_MODEL2B_WORK_RAM_BASE,
            address,
            output,
            size
        );
    }

    if (range_contains(
            STF_MODEL2B_GEO_START,
            model2b->geometry_ram_size,
            address,
            size
        )) {
        return read_storage(
            model2b->geometry_ram,
            model2b->geometry_ram_size,
            STF_MODEL2B_GEO_START,
            address,
            output,
            size
        );
    }

    if (range_contains(
            STF_MODEL2B_BUFF_RAM_START,
            model2b->buffer_ram_size,
            address,
            size
        )) {
        return read_storage(
            model2b->buffer_ram,
            model2b->buffer_ram_size,
            STF_MODEL2B_BUFF_RAM_START,
            address,
            output,
            size
        );
    }

    if (read_attached_rom(
            model2b->main_data,
            model2b->main_data_size,
            STF_MODEL2B_MAIN_DATA_START,
            address,
            output,
            size
        )) {
        return STF_OK;
    }

    if (read_attached_rom(
            model2b->main_data_ep,
            model2b->main_data_ep_size,
            STF_MODEL2B_MAIN_DATA_EP_START,
            address,
            output,
            size
        ) ||
        read_attached_rom(
            model2b->main_data_ep,
            model2b->main_data_ep_size,
            STF_MODEL2B_MAIN_DATA_EP_MIRROR_START,
            address,
            output,
            size
        )) {
        return STF_OK;
    }

    if (range_contains(
            STF_MODEL2B_STAGE_PALETTE_DATA,
            STF_MODEL2B_PALETTE_SIZE,
            address,
            size
        )) {
        return read_storage(
            model2b->palette_ram,
            STF_MODEL2B_PALETTE_SIZE,
            STF_MODEL2B_STAGE_PALETTE_DATA,
            address,
            output,
            size
        );
    }

    if (range_contains(
            STF_MODEL2B_COLORXLAT_START,
            STF_MODEL2B_COLORXLAT_SIZE,
            address,
            size
        )) {
        return read_storage(
            model2b->color_xlat_ram,
            STF_MODEL2B_COLORXLAT_SIZE,
            STF_MODEL2B_COLORXLAT_START,
            address,
            output,
            size
        );
    }

    if (range_contains(
            STF_MODEL2B_BACKUP_RAM_START,
            STF_MODEL2B_BACKUP_RAM_SIZE,
            address,
            size
        )) {
        return read_storage(
            model2b->backup_ram,
            STF_MODEL2B_BACKUP_RAM_SIZE,
            STF_MODEL2B_BACKUP_RAM_START,
            address,
            output,
            size
        );
    }

    if (range_contains(
            STF_MODEL2B_TEXTURE0_START,
            STF_MODEL2B_TEXTURE0_SIZE,
            address,
            size
        )) {
        return read_storage(
            model2b->texture0_ram,
            STF_MODEL2B_TEXTURE0_SIZE,
            STF_MODEL2B_TEXTURE0_START,
            address,
            output,
            size
        );
    }

    if (range_contains(
            STF_MODEL2B_TEXTURE1_START,
            STF_MODEL2B_TEXTURE1_SIZE,
            address,
            size
        )) {
        return read_storage(
            model2b->texture1_ram,
            STF_MODEL2B_TEXTURE1_SIZE,
            STF_MODEL2B_TEXTURE1_START,
            address,
            output,
            size
        );
    }

    if (range_contains(
            STF_MODEL2B_LUMA_START,
            STF_MODEL2B_LUMA_SIZE,
            address,
            size
        )) {
        return read_storage(
            model2b->luma_ram,
            STF_MODEL2B_LUMA_SIZE,
            STF_MODEL2B_LUMA_START,
            address,
            output,
            size
        );
    }

    if (model2b->device_read != NULL) {
        status = model2b->device_read(
            model2b->device_context,
            address,
            output,
            size
        );
        if (status == STF_OK) {
            return STF_OK;
        }
    }

    record_fault(model2b, 0, address, size, status);
    return status;
}

static stf_status model2b_write(
    void *context,
    uint32_t address,
    const void *data,
    size_t size
)
{
    stf_model2b_bus *model2b = (stf_model2b_bus *)context;
    stf_status status = STF_ERROR_UNSUPPORTED;

    if (model2b == NULL || data == NULL) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    if (size == 0u) {
        return STF_OK;
    }

    if (range_contains(
            STF_MODEL2B_WORK_RAM_BASE,
            model2b->work_ram_size,
            address,
            size
        )) {
        return write_storage(
            model2b->work_ram,
            model2b->work_ram_size,
            STF_MODEL2B_WORK_RAM_BASE,
            address,
            data,
            size
        );
    }

    if (range_contains(
            STF_MODEL2B_GEO_START,
            model2b->geometry_ram_size,
            address,
            size
        )) {
        return write_storage(
            model2b->geometry_ram,
            model2b->geometry_ram_size,
            STF_MODEL2B_GEO_START,
            address,
            data,
            size
        );
    }

    if (range_contains(
            STF_MODEL2B_BUFF_RAM_START,
            model2b->buffer_ram_size,
            address,
            size
        )) {
        return write_storage(
            model2b->buffer_ram,
            model2b->buffer_ram_size,
            STF_MODEL2B_BUFF_RAM_START,
            address,
            data,
            size
        );
    }

    if (range_contains(
            STF_MODEL2B_STAGE_PALETTE_DATA,
            STF_MODEL2B_PALETTE_SIZE,
            address,
            size
        )) {
        return write_storage(
            model2b->palette_ram,
            STF_MODEL2B_PALETTE_SIZE,
            STF_MODEL2B_STAGE_PALETTE_DATA,
            address,
            data,
            size
        );
    }

    if (range_contains(
            STF_MODEL2B_COLORXLAT_START,
            STF_MODEL2B_COLORXLAT_SIZE,
            address,
            size
        )) {
        return write_storage(
            model2b->color_xlat_ram,
            STF_MODEL2B_COLORXLAT_SIZE,
            STF_MODEL2B_COLORXLAT_START,
            address,
            data,
            size
        );
    }

    if (range_contains(
            STF_MODEL2B_BACKUP_RAM_START,
            STF_MODEL2B_BACKUP_RAM_SIZE,
            address,
            size
        )) {
        return write_storage(
            model2b->backup_ram,
            STF_MODEL2B_BACKUP_RAM_SIZE,
            STF_MODEL2B_BACKUP_RAM_START,
            address,
            data,
            size
        );
    }

    if (range_contains(
            STF_MODEL2B_TEXTURE0_START,
            STF_MODEL2B_TEXTURE0_SIZE,
            address,
            size
        )) {
        return write_storage(
            model2b->texture0_ram,
            STF_MODEL2B_TEXTURE0_SIZE,
            STF_MODEL2B_TEXTURE0_START,
            address,
            data,
            size
        );
    }

    if (range_contains(
            STF_MODEL2B_TEXTURE1_START,
            STF_MODEL2B_TEXTURE1_SIZE,
            address,
            size
        )) {
        return write_storage(
            model2b->texture1_ram,
            STF_MODEL2B_TEXTURE1_SIZE,
            STF_MODEL2B_TEXTURE1_START,
            address,
            data,
            size
        );
    }

    if (range_contains(
            STF_MODEL2B_LUMA_START,
            STF_MODEL2B_LUMA_SIZE,
            address,
            size
        )) {
        return write_storage(
            model2b->luma_ram,
            STF_MODEL2B_LUMA_SIZE,
            STF_MODEL2B_LUMA_START,
            address,
            data,
            size
        );
    }

    if (write_hits_attached_rom(
            model2b->rom,
            model2b->rom_size,
            STF_MODEL2B_ROM_BASE,
            address,
            size
        ) ||
        write_hits_attached_rom(
            model2b->main_data,
            model2b->main_data_size,
            STF_MODEL2B_MAIN_DATA_START,
            address,
            size
        ) ||
        write_hits_attached_rom(
            model2b->main_data_ep,
            model2b->main_data_ep_size,
            STF_MODEL2B_MAIN_DATA_EP_START,
            address,
            size
        ) ||
        write_hits_attached_rom(
            model2b->main_data_ep,
            model2b->main_data_ep_size,
            STF_MODEL2B_MAIN_DATA_EP_MIRROR_START,
            address,
            size
        )) {
        record_fault(model2b, 1, address, size, STF_ERROR_UNSUPPORTED);
        return STF_ERROR_UNSUPPORTED;
    }

    if (model2b->device_write != NULL) {
        status = model2b->device_write(
            model2b->device_context,
            address,
            data,
            size
        );
        if (status == STF_OK) {
            return STF_OK;
        }
    }

    record_fault(model2b, 1, address, size, status);
    return status;
}

stf_status stf_model2b_bus_init(stf_model2b_bus *model2b)
{
    if (model2b == NULL) {
        return STF_ERROR_INVALID_ARGUMENT;
    }

    memset(model2b, 0, sizeof(*model2b));

    model2b->work_ram = (uint8_t *)calloc(1u, STF_MODEL2B_WORK_RAM_SIZE);
    model2b->geometry_ram = (uint8_t *)calloc(1u, STF_MODEL2B_GEO_RAM_SIZE);
    model2b->buffer_ram = (uint8_t *)calloc(1u, STF_MODEL2B_BUFF_RAM_SIZE);
    model2b->palette_ram = (uint8_t *)calloc(1u, STF_MODEL2B_PALETTE_SIZE);
    model2b->color_xlat_ram = (uint8_t *)calloc(1u, STF_MODEL2B_COLORXLAT_SIZE);
    model2b->backup_ram = (uint8_t *)malloc(STF_MODEL2B_BACKUP_RAM_SIZE);
    model2b->texture0_ram = (uint8_t *)calloc(1u, STF_MODEL2B_TEXTURE0_SIZE);
    model2b->texture1_ram = (uint8_t *)calloc(1u, STF_MODEL2B_TEXTURE1_SIZE);
    model2b->luma_ram = (uint8_t *)calloc(1u, STF_MODEL2B_LUMA_SIZE);

    if (model2b->work_ram == NULL ||
        model2b->geometry_ram == NULL ||
        model2b->buffer_ram == NULL ||
        model2b->palette_ram == NULL ||
        model2b->color_xlat_ram == NULL ||
        model2b->backup_ram == NULL ||
        model2b->texture0_ram == NULL ||
        model2b->texture1_ram == NULL ||
        model2b->luma_ram == NULL) {
        stf_model2b_bus_destroy(model2b);
        return STF_ERROR_OUT_OF_MEMORY;
    }

    model2b->work_ram_size = STF_MODEL2B_WORK_RAM_SIZE;
    model2b->geometry_ram_size = STF_MODEL2B_GEO_RAM_SIZE;
    model2b->buffer_ram_size = STF_MODEL2B_BUFF_RAM_SIZE;
    memset(model2b->backup_ram, 0xff, STF_MODEL2B_BACKUP_RAM_SIZE);

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
    free(model2b->geometry_ram);
    free(model2b->buffer_ram);
    free(model2b->palette_ram);
    free(model2b->color_xlat_ram);
    free(model2b->backup_ram);
    free(model2b->texture0_ram);
    free(model2b->texture1_ram);
    free(model2b->luma_ram);
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

stf_status stf_model2b_bus_attach_main_data(
    stf_model2b_bus *model2b,
    const uint8_t *data,
    size_t data_size
)
{
    if (model2b == NULL || data == NULL || data_size == 0u ||
        data_size > STF_MODEL2B_MAIN_DATA_SIZE) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    model2b->main_data = data;
    model2b->main_data_size = data_size;
    return STF_OK;
}

stf_status stf_model2b_bus_attach_main_data_ep(
    stf_model2b_bus *model2b,
    const uint8_t *data,
    size_t data_size
)
{
    if (model2b == NULL || data == NULL || data_size == 0u ||
        data_size > STF_MODEL2B_MAIN_DATA_EP_SIZE) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    model2b->main_data_ep = data;
    model2b->main_data_ep_size = data_size;
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

void stf_model2b_bus_clear_fault(stf_model2b_bus *model2b)
{
    if (model2b != NULL) {
        memset(&model2b->last_fault, 0, sizeof(model2b->last_fault));
    }
}

const stf_model2b_fault *stf_model2b_bus_last_fault(
    const stf_model2b_bus *model2b
)
{
    return model2b != NULL ? &model2b->last_fault : NULL;
}

stf_i960_bus *stf_model2b_bus_i960(stf_model2b_bus *model2b)
{
    return model2b != NULL ? &model2b->i960 : NULL;
}

const stf_i960_bus *stf_model2b_bus_i960_const(const stf_model2b_bus *model2b)
{
    return model2b != NULL ? &model2b->i960 : NULL;
}
