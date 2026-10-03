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

static uint32_t read_le_value(const void *data, size_t size)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t value = 0u;
    size_t index = 0u;

    for (index = 0u; index < size && index < 4u; ++index) {
        value |= (uint32_t)bytes[index] << (index * 8u);
    }
    return value;
}

static void write_le_value(void *output, size_t size, uint32_t value)
{
    uint8_t *bytes = (uint8_t *)output;
    size_t index = 0u;

    for (index = 0u; index < size; ++index) {
        bytes[index] = (uint8_t)(value >> (index * 8u));
    }
}

static int read_register32(
    uint32_t base,
    uint32_t value,
    uint32_t address,
    void *output,
    size_t size
)
{
    uint8_t bytes[4];
    size_t offset = 0u;

    if (!range_contains(base, 4u, address, size)) {
        return 0;
    }
    write_le_value(bytes, sizeof(bytes), value);
    offset = (size_t)(address - base);
    memcpy(output, bytes + offset, size);
    return 1;
}

static int write_register32(
    uint32_t base,
    uint32_t *value,
    uint32_t address,
    const void *data,
    size_t size
)
{
    uint8_t bytes[4];
    size_t offset = 0u;

    if (value == NULL || !range_contains(base, 4u, address, size)) {
        return 0;
    }
    write_le_value(bytes, sizeof(bytes), *value);
    offset = (size_t)(address - base);
    memcpy(bytes + offset, data, size);
    *value = read_le_value(bytes, sizeof(bytes));
    return 1;
}

static int buffer_ram_offset(
    uint32_t address,
    size_t size,
    size_t *offset
)
{
    size_t local = 0u;

    if (!range_contains(
            STF_MODEL2B_BUFFER_RAM_BASE,
            STF_MODEL2B_BUFFER_RAM_MIRROR_SIZE,
            address,
            size
        )) {
        return 0;
    }
    local = (size_t)(address - STF_MODEL2B_BUFFER_RAM_BASE) &
            (STF_MODEL2B_BUFFER_RAM_SIZE - 1u);
    if (local + size > STF_MODEL2B_BUFFER_RAM_SIZE) {
        return 0;
    }
    *offset = local;
    return 1;
}

static stf_status push_geo_data(stf_model2b_bus *model2b, uint32_t value)
{
    const uint32_t offset = model2b->geo_write_start_address;

    if ((offset & 3u) != 0u ||
        (uint64_t)offset + 4u > model2b->buffer_ram_size) {
        return STF_ERROR_OUT_OF_BOUNDS;
    }
    write_le_value(model2b->buffer_ram + offset, 4u, value);
    model2b->geo_write_start_address += 4u;
    model2b->last_geo_word = value;
    ++model2b->geo_fifo_words;
    return STF_OK;
}

static stf_status read_builtin_device(
    stf_model2b_bus *model2b,
    uint32_t address,
    void *output,
    size_t size
)
{
    static const uint8_t tgp_id[STF_MODEL2B_TGP_ID_SIZE] = {
        0u, 'T', 'A', 'H', 0u, 'A', 'K', 'O',
        0u, 'Z', 'A', 'K', 0u, 'M', 'T', 'K'
    };
    uint32_t value = 0u;

    if (range_contains(
            STF_MODEL2B_GEO_BASE,
            STF_MODEL2B_GEO_SIZE,
            address,
            size
        )) {
        if (read_register32(
                STF_MODEL2B_GEO_BASE + 0x2008u,
                model2b->geo_write_start_address,
                address,
                output,
                size
            )) {
            return STF_OK;
        }
        if (read_register32(
                STF_MODEL2B_GEO_BASE + 0x3008u,
                model2b->geo_read_start_address,
                address,
                output,
                size
            )) {
            return STF_OK;
        }
        return STF_ERROR_UNSUPPORTED;
    }

    if (range_contains(
            STF_MODEL2B_GEO_PROGRAM_BASE,
            STF_MODEL2B_GEO_PROGRAM_SIZE,
            address,
            size
        )) {
        if (size > 4u) {
            return STF_ERROR_UNSUPPORTED;
        }
        write_le_value(output, size, UINT32_MAX);
        return STF_OK;
    }

    /*
     * Coprocessor FIFO reads require actual SHARC/TGP output. They are kept
     * fail-closed until a reference response is supplied by a probe callback
     * or a coprocessor model.
     */
    if (range_contains(
            STF_MODEL2B_COPRO_FIFO_BASE,
            STF_MODEL2B_COPRO_FIFO_SIZE,
            address,
            size
        )) {
        return STF_ERROR_UNSUPPORTED;
    }

    if (read_register32(
            STF_MODEL2B_COPRO_CONTROL,
            model2b->copro_control,
            address,
            output,
            size
        )) {
        return STF_OK;
    }

    if (read_register32(
            STF_MODEL2B_FIFO_CONTROL,
            1u,
            address,
            output,
            size
        )) {
        return STF_OK;
    }

    if (read_register32(
            STF_MODEL2B_VIDEO_CONTROL,
            model2b->video_control & 3u,
            address,
            output,
            size
        )) {
        return STF_OK;
    }

    if (read_register32(
            STF_MODEL2B_COPRO_STATUS,
            model2b->copro_upload_words == 0u ? UINT32_MAX : 0u,
            address,
            output,
            size
        )) {
        return STF_OK;
    }

    if (read_register32(
            STF_MODEL2B_COPRO_BANK_CONTROL,
            0u,
            address,
            output,
            size
        )) {
        return STF_OK;
    }

    if (range_contains(
            STF_MODEL2B_TGP_ID_BASE,
            STF_MODEL2B_TGP_ID_SIZE,
            address,
            size
        )) {
        memcpy(
            output,
            tgp_id + (address - STF_MODEL2B_TGP_ID_BASE),
            size
        );
        return STF_OK;
    }

    value = 0u;
    (void)value;
    return STF_ERROR_UNSUPPORTED;
}

static stf_status write_builtin_device(
    stf_model2b_bus *model2b,
    uint32_t address,
    const void *data,
    size_t size
)
{
    uint32_t value = 0u;

    if (range_contains(
            STF_MODEL2B_GEO_BASE,
            STF_MODEL2B_GEO_SIZE,
            address,
            size
        )) {
        const uint32_t offset = address - STF_MODEL2B_GEO_BASE;

        if (size != 4u || (address & 3u) != 0u) {
            return STF_ERROR_UNSUPPORTED;
        }
        value = read_le_value(data, size);

        if (offset < 0x1000u) {
            uint32_t encoded = 0u;

            if ((value & UINT32_C(0x80000000)) != 0u) {
                encoded = value & UINT32_C(0x800fffff);
                encoded |= ((offset >> 4u) & UINT32_C(0x3f)) << 23u;
                return push_geo_data(model2b, encoded);
            }
            if ((offset & 0x0fu) == 0u) {
                encoded = value & UINT32_C(0x000fffff);
                encoded |= ((offset >> 4u) & UINT32_C(0x3f)) << 23u;
                if (((offset >> 4u) & UINT32_C(0xc0)) != 0u &&
                    ((offset >> 4u) & UINT32_C(0x3f)) == 1u) {
                    encoded |= ((offset >> 10u) & 3u) << 29u;
                }
                return push_geo_data(model2b, encoded);
            }
            return STF_OK;
        }

        if (offset == 0x1008u) {
            model2b->geo_write_start_address = value & UINT32_C(0x000fffff);
            return STF_OK;
        }
        if (offset == 0x3008u) {
            model2b->geo_read_start_address = value & UINT32_C(0x000fffff);
            return STF_OK;
        }
        return STF_ERROR_UNSUPPORTED;
    }

    if (range_contains(
            STF_MODEL2B_GEO_PROGRAM_BASE,
            STF_MODEL2B_GEO_PROGRAM_SIZE,
            address,
            size
        )) {
        if (size != 4u) {
            return STF_ERROR_UNSUPPORTED;
        }
        value = read_le_value(data, size);
        if ((model2b->geo_control & UINT32_C(0x80000000)) != 0u) {
            ++model2b->geo_upload_words;
            return STF_OK;
        }
        return push_geo_data(model2b, value);
    }

    if (range_contains(
            STF_MODEL2B_COPRO_FUNCTION_BASE,
            STF_MODEL2B_COPRO_FUNCTION_SIZE,
            address,
            size
        )) {
        uint32_t encoded = 0u;
        uint32_t offset = address - STF_MODEL2B_COPRO_FUNCTION_BASE;

        if (size != 4u || (address & 3u) != 0u) {
            return STF_ERROR_UNSUPPORTED;
        }
        value = read_le_value(data, size);
        encoded = value & UINT32_C(0x800fffff);
        encoded |= ((offset >> 2u) & UINT32_C(0xff)) << 23u;
        model2b->last_copro_input_word = encoded;
        ++model2b->copro_function_words;
        ++model2b->copro_fifo_input_words;
        return STF_OK;
    }

    if (range_contains(
            STF_MODEL2B_COPRO_FIFO_BASE,
            STF_MODEL2B_COPRO_FIFO_SIZE,
            address,
            size
        )) {
        if (size != 4u) {
            return STF_ERROR_UNSUPPORTED;
        }
        value = read_le_value(data, size);
        model2b->last_copro_input_word = value;
        if ((model2b->copro_control & UINT32_C(0x80000000)) != 0u) {
            ++model2b->copro_upload_words;
        } else {
            ++model2b->copro_fifo_input_words;
        }
        return STF_OK;
    }

    if (range_contains(
            STF_MODEL2B_COPRO_IOP_BASE,
            STF_MODEL2B_COPRO_IOP_SIZE,
            address,
            size
        )) {
        if (size != 4u) {
            return STF_ERROR_UNSUPPORTED;
        }
        ++model2b->copro_iop_writes;
        return STF_OK;
    }

    if (range_contains(STF_MODEL2B_COPRO_CONTROL, 4u, address, size)) {
        const uint32_t old_value = model2b->copro_control;
        if (!write_register32(
                STF_MODEL2B_COPRO_CONTROL,
                &model2b->copro_control,
                address,
                data,
                size
            )) {
            return STF_ERROR_UNSUPPORTED;
        }
        if (((old_value ^ model2b->copro_control) & UINT32_C(0x80000000)) != 0u &&
            (model2b->copro_control & UINT32_C(0x80000000)) != 0u) {
            model2b->copro_upload_words = 0u;
        }
        return STF_OK;
    }

    if (range_contains(STF_MODEL2B_GEO_CONTROL, 4u, address, size)) {
        const uint32_t old_value = model2b->geo_control;
        if (!write_register32(
                STF_MODEL2B_GEO_CONTROL,
                &model2b->geo_control,
                address,
                data,
                size
            )) {
            return STF_ERROR_UNSUPPORTED;
        }
        if (((old_value ^ model2b->geo_control) & UINT32_C(0x80000000)) != 0u &&
            (model2b->geo_control & UINT32_C(0x80000000)) != 0u) {
            model2b->geo_upload_words = 0u;
        }
        return STF_OK;
    }

    if (range_contains(STF_MODEL2B_VIDEO_CONTROL, 4u, address, size)) {
        return write_register32(
            STF_MODEL2B_VIDEO_CONTROL,
            &model2b->video_control,
            address,
            data,
            size
        ) ? STF_OK : STF_ERROR_UNSUPPORTED;
    }

    if (range_contains(STF_MODEL2B_COPRO_BANK_CONTROL, 4u, address, size)) {
        return STF_OK;
    }

    return STF_ERROR_UNSUPPORTED;
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
    size_t offset = 0u;

    if (model2b == NULL || output == NULL) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    if (size == 0u) {
        return STF_OK;
    }

    if (model2b->extra_ram != NULL &&
        range_contains(
            STF_MODEL2B_EXTRA_RAM_BASE,
            model2b->extra_ram_size,
            address,
            size
        )) {
        memcpy(
            output,
            model2b->extra_ram + (address - STF_MODEL2B_EXTRA_RAM_BASE),
            size
        );
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

    if (model2b->buffer_ram != NULL &&
        buffer_ram_offset(address, size, &offset)) {
        memcpy(output, model2b->buffer_ram + offset, size);
        return STF_OK;
    }

    if (range_contains(
            STF_MODEL2B_CPU_CONTROL_BASE,
            sizeof(model2b->cpu_control),
            address,
            size
        )) {
        memcpy(
            output,
            model2b->cpu_control + (address - STF_MODEL2B_CPU_CONTROL_BASE),
            size
        );
        return STF_OK;
    }

    if (model2b->rom != NULL &&
        range_contains(STF_MODEL2B_ROM_BASE, model2b->rom_size, address, size)) {
        memcpy(output, model2b->rom + (address - STF_MODEL2B_ROM_BASE), size);
        return STF_OK;
    }

    /*
     * Explicit probe stubs override bounded device behavior. This is useful
     * for replaying a measured FIFO/status response without pretending the
     * coprocessor itself is implemented.
     */
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

    status = read_builtin_device(model2b, address, output, size);
    if (status == STF_OK) {
        return STF_OK;
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
    size_t offset = 0u;

    if (model2b == NULL || data == NULL) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    if (size == 0u) {
        return STF_OK;
    }

    if (model2b->extra_ram != NULL &&
        range_contains(
            STF_MODEL2B_EXTRA_RAM_BASE,
            model2b->extra_ram_size,
            address,
            size
        )) {
        memcpy(
            model2b->extra_ram + (address - STF_MODEL2B_EXTRA_RAM_BASE),
            data,
            size
        );
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

    if (model2b->buffer_ram != NULL &&
        buffer_ram_offset(address, size, &offset)) {
        memcpy(model2b->buffer_ram + offset, data, size);
        return STF_OK;
    }

    if (range_contains(
            STF_MODEL2B_CPU_CONTROL_BASE,
            sizeof(model2b->cpu_control),
            address,
            size
        )) {
        memcpy(
            model2b->cpu_control + (address - STF_MODEL2B_CPU_CONTROL_BASE),
            data,
            size
        );
        return STF_OK;
    }

    if (model2b->rom != NULL &&
        range_contains(STF_MODEL2B_ROM_BASE, model2b->rom_size, address, size)) {
        record_fault(model2b, 1, address, size, STF_ERROR_UNSUPPORTED);
        return STF_ERROR_UNSUPPORTED;
    }

    status = write_builtin_device(model2b, address, data, size);
    if (status == STF_OK) {
        return STF_OK;
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

static void initialize_buffer_ram(stf_model2b_bus *model2b)
{
    size_t offset = 0u;

    for (offset = 0u; offset + 4u <= model2b->buffer_ram_size; offset += 4u) {
        write_le_value(
            model2b->buffer_ram + offset,
            4u,
            UINT32_C(0x07800f0f)
        );
    }
}

stf_status stf_model2b_bus_init(stf_model2b_bus *model2b)
{
    if (model2b == NULL) {
        return STF_ERROR_INVALID_ARGUMENT;
    }

    memset(model2b, 0, sizeof(*model2b));

    model2b->extra_ram = (uint8_t *)calloc(1u, STF_MODEL2B_EXTRA_RAM_SIZE);
    model2b->work_ram = (uint8_t *)calloc(1u, STF_MODEL2B_WORK_RAM_SIZE);
    model2b->buffer_ram = (uint8_t *)malloc(STF_MODEL2B_BUFFER_RAM_SIZE);

    if (model2b->extra_ram == NULL ||
        model2b->work_ram == NULL ||
        model2b->buffer_ram == NULL) {
        stf_model2b_bus_destroy(model2b);
        return STF_ERROR_OUT_OF_MEMORY;
    }

    model2b->extra_ram_size = STF_MODEL2B_EXTRA_RAM_SIZE;
    model2b->work_ram_size = STF_MODEL2B_WORK_RAM_SIZE;
    model2b->buffer_ram_size = STF_MODEL2B_BUFFER_RAM_SIZE;
    initialize_buffer_ram(model2b);

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
    free(model2b->extra_ram);
    free(model2b->work_ram);
    free(model2b->buffer_ram);
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
