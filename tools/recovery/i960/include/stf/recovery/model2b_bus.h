#ifndef STF_RECOVERY_MODEL2B_BUS_H
#define STF_RECOVERY_MODEL2B_BUS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "stf/recovery/i960/bus.h"

enum {
    STF_MODEL2B_ROM_BASE = 0x00000000u,

    STF_MODEL2B_EXTRA_RAM_BASE = 0x00200000u,
    STF_MODEL2B_EXTRA_RAM_SIZE = 0x00040000u,

    STF_MODEL2B_WORK_RAM_BASE = 0x00500000u,
    STF_MODEL2B_WORK_RAM_SIZE = 0x00100000u,

    STF_MODEL2B_GEO_BASE = 0x00800000u,
    STF_MODEL2B_GEO_SIZE = 0x00004000u,
    STF_MODEL2B_GEO_PROGRAM_BASE = 0x00804000u,
    STF_MODEL2B_GEO_PROGRAM_SIZE = 0x00004000u,

    STF_MODEL2B_COPRO_FUNCTION_BASE = 0x00880000u,
    STF_MODEL2B_COPRO_FUNCTION_SIZE = 0x00004000u,
    STF_MODEL2B_COPRO_FIFO_BASE = 0x00884000u,
    STF_MODEL2B_COPRO_FIFO_SIZE = 0x00004000u,
    STF_MODEL2B_COPRO_IOP_BASE = 0x008C0000u,
    STF_MODEL2B_COPRO_IOP_SIZE = 0x00001000u,

    STF_MODEL2B_BUFFER_RAM_BASE = 0x00900000u,
    STF_MODEL2B_BUFFER_RAM_SIZE = 0x00020000u,
    STF_MODEL2B_BUFFER_RAM_MIRROR_SIZE = 0x00080000u,

    STF_MODEL2B_COPRO_CONTROL = 0x00980000u,
    STF_MODEL2B_FIFO_CONTROL = 0x00980004u,
    STF_MODEL2B_GEO_CONTROL = 0x00980008u,
    STF_MODEL2B_VIDEO_CONTROL = 0x0098000Cu,
    STF_MODEL2B_COPRO_STATUS = 0x00980014u,
    STF_MODEL2B_COPRO_BANK_CONTROL = 0x00980020u,
    STF_MODEL2B_TGP_ID_BASE = 0x00980030u,
    STF_MODEL2B_TGP_ID_SIZE = 0x10u,

    STF_MODEL2B_CPU_CONTROL_BASE = 0x00E00000u,
    STF_MODEL2B_CPU_CONTROL_SIZE = 0x38u
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

    uint8_t *extra_ram;
    size_t extra_ram_size;

    uint8_t *work_ram;
    size_t work_ram_size;

    uint8_t *buffer_ram;
    size_t buffer_ram_size;

    uint8_t cpu_control[STF_MODEL2B_CPU_CONTROL_SIZE];

    const uint8_t *rom;
    size_t rom_size;

    uint32_t copro_control;
    uint32_t geo_control;
    uint32_t video_control;
    uint32_t geo_write_start_address;
    uint32_t geo_read_start_address;

    uint64_t copro_upload_words;
    uint64_t geo_upload_words;
    uint64_t copro_fifo_input_words;
    uint64_t copro_function_words;
    uint64_t copro_iop_writes;
    uint64_t geo_fifo_words;

    uint32_t last_copro_input_word;
    uint32_t last_geo_word;

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
