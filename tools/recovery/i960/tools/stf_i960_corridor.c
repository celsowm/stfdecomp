#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stf/recovery/i960/decoder.h"
#include "stf/recovery/i960/executor.h"
#include "stf/recovery/model2b_bus.h"
#include "stf/recovery/model2b_map.h"

typedef struct trace_writer {
    FILE *file;
} trace_writer;

enum {
    MAX_PROBE_WRITE_RANGES = 32,
    MAX_PROBE_READ_STUBS = 64
};

typedef struct probe_write_range {
    uint32_t start;
    uint32_t end;
} probe_write_range;

typedef struct probe_read_stub {
    uint32_t address;
    uint32_t value;
} probe_read_stub;

typedef struct probe_device {
    probe_write_range write_ranges[MAX_PROBE_WRITE_RANGES];
    size_t write_range_count;
    probe_read_stub read_stubs[MAX_PROBE_READ_STUBS];
    size_t read_stub_count;
    uint64_t accepted_accesses;
} probe_device;

typedef struct runner_options {
    const char *rom_path;
    const char *trace_path;
    const char *state_path;
    const char *work_ram_in_path;
    const char *work_ram_out_path;
    uint32_t entry;
    uint32_t stop;
    uint32_t sat;
    uint32_t prcb;
    uint64_t max_steps;
    uint32_t stack_base;
    int have_stack;
    int reset_from_prcb;
    uint32_t registers[STF_I960_REGISTER_COUNT];
    uint8_t register_set[STF_I960_REGISTER_COUNT];
    probe_device probe;
} runner_options;

static void usage(const char *argv0)
{
    fprintf(
        stderr,
        "usage: %s --rom FILE [options]\n"
        "\n"
        "options:\n"
        "  --entry ADDR          start IP (default 0)\n"
        "  --stop ADDR           stop before executing ADDR\n"
        "  --steps N             maximum executed instructions (default 100000)\n"
        "  --sat ADDR            system address table value\n"
        "  --prcb ADDR           PRCB address\n"
        "  --reset-from-prcb      initialize fp/sp from PRCB + 24\n"
        "  --stack ADDR          explicit frame base; sp becomes ADDR + 64\n"
        "  --reg REG=VALUE       initialize register (r0..r15, g0..g14, fp, sp, pfp, rip)\n"
        "  --work-ram-in FILE    preload beginning of 1 MiB work RAM\n"
        "  --work-ram-out FILE   dump final 1 MiB work RAM\n"
        "  --trace FILE          write JSONL step/memory trace\n"
        "  --state FILE          write final CPU state JSON\n"
        "  --allow-write A:B     explicitly accept device writes in [A,B)\n"
        "  --stub-read A=VALUE   explicitly return VALUE for a device read at A\n"
        "\n"
        "All numbers accept C syntax (for example 0x00500000). Unknown Model 2B\n"
        "hardware accesses fail closed. Probe allowances are exploratory and are\n"
        "counted in the final report; they are never enabled implicitly.\n",
        argv0
    );
}

static int parse_u32(const char *text, uint32_t *value)
{
    char *end = NULL;
    unsigned long long parsed = 0ull;

    if (text == NULL || value == NULL || *text == '\0') {
        return 0;
    }
    errno = 0;
    parsed = strtoull(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || parsed > UINT32_MAX) {
        return 0;
    }
    *value = (uint32_t)parsed;
    return 1;
}

static int parse_u64(const char *text, uint64_t *value)
{
    char *end = NULL;
    unsigned long long parsed = 0ull;

    if (text == NULL || value == NULL || *text == '\0') {
        return 0;
    }
    errno = 0;
    parsed = strtoull(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0') {
        return 0;
    }
    *value = (uint64_t)parsed;
    return 1;
}

static int register_index(const char *name)
{
    char *end = NULL;
    long index = 0;

    if (name == NULL || *name == '\0') {
        return -1;
    }
    if (strcmp(name, "pfp") == 0) return 0;
    if (strcmp(name, "sp") == 0) return 1;
    if (strcmp(name, "rip") == 0) return 2;
    if (strcmp(name, "fp") == 0) return STF_I960_FP_REGISTER;

    if (name[0] == 'r') {
        index = strtol(name + 1, &end, 10);
        if (end != name + 1 && *end == '\0' && index >= 0 && index <= 15) {
            return (int)index;
        }
    }
    if (name[0] == 'g') {
        index = strtol(name + 1, &end, 10);
        if (end != name + 1 && *end == '\0' && index >= 0 && index <= 14) {
            return 16 + (int)index;
        }
    }

    index = strtol(name, &end, 0);
    if (end != name && *end == '\0' && index >= 0 &&
        index < STF_I960_REGISTER_COUNT) {
        return (int)index;
    }
    return -1;
}

static int parse_write_range(const char *text, probe_write_range *range)
{
    char left[32];
    const char *colon = NULL;
    size_t length = 0u;

    if (text == NULL || range == NULL) {
        return 0;
    }
    colon = strchr(text, ':');
    if (colon == NULL) {
        return 0;
    }
    length = (size_t)(colon - text);
    if (length == 0u || length >= sizeof(left)) {
        return 0;
    }
    memcpy(left, text, length);
    left[length] = '\0';

    if (!parse_u32(left, &range->start) ||
        !parse_u32(colon + 1, &range->end) ||
        range->end <= range->start) {
        return 0;
    }
    return 1;
}

static int parse_read_stub(const char *text, probe_read_stub *stub)
{
    char left[32];
    const char *equal = NULL;
    size_t length = 0u;

    if (text == NULL || stub == NULL) {
        return 0;
    }
    equal = strchr(text, '=');
    if (equal == NULL) {
        return 0;
    }
    length = (size_t)(equal - text);
    if (length == 0u || length >= sizeof(left)) {
        return 0;
    }
    memcpy(left, text, length);
    left[length] = '\0';
    return parse_u32(left, &stub->address) &&
           parse_u32(equal + 1, &stub->value);
}

static int parse_register_assignment(
    const char *text,
    int *index,
    uint32_t *value
)
{
    char name[32];
    const char *equal = NULL;
    size_t length = 0u;

    if (text == NULL || index == NULL || value == NULL) {
        return 0;
    }
    equal = strchr(text, '=');
    if (equal == NULL) {
        return 0;
    }
    length = (size_t)(equal - text);
    if (length == 0u || length >= sizeof(name)) {
        return 0;
    }
    memcpy(name, text, length);
    name[length] = '\0';

    *index = register_index(name);
    return *index >= 0 && parse_u32(equal + 1, value);
}

static int parse_arguments(int argc, char **argv, runner_options *options)
{
    int index = 0;

    memset(options, 0, sizeof(*options));
    options->max_steps = UINT64_C(100000);

    for (index = 1; index < argc; ++index) {
        const char *arg = argv[index];

        if (strcmp(arg, "--rom") == 0 && index + 1 < argc) {
            options->rom_path = argv[++index];
        } else if (strcmp(arg, "--entry") == 0 && index + 1 < argc) {
            if (!parse_u32(argv[++index], &options->entry)) return 0;
        } else if (strcmp(arg, "--stop") == 0 && index + 1 < argc) {
            if (!parse_u32(argv[++index], &options->stop)) return 0;
        } else if (strcmp(arg, "--steps") == 0 && index + 1 < argc) {
            if (!parse_u64(argv[++index], &options->max_steps)) return 0;
        } else if (strcmp(arg, "--sat") == 0 && index + 1 < argc) {
            if (!parse_u32(argv[++index], &options->sat)) return 0;
        } else if (strcmp(arg, "--prcb") == 0 && index + 1 < argc) {
            if (!parse_u32(argv[++index], &options->prcb)) return 0;
        } else if (strcmp(arg, "--stack") == 0 && index + 1 < argc) {
            if (!parse_u32(argv[++index], &options->stack_base)) return 0;
            options->have_stack = 1;
        } else if (strcmp(arg, "--reset-from-prcb") == 0) {
            options->reset_from_prcb = 1;
        } else if (strcmp(arg, "--trace") == 0 && index + 1 < argc) {
            options->trace_path = argv[++index];
        } else if (strcmp(arg, "--state") == 0 && index + 1 < argc) {
            options->state_path = argv[++index];
        } else if (strcmp(arg, "--work-ram-in") == 0 && index + 1 < argc) {
            options->work_ram_in_path = argv[++index];
        } else if (strcmp(arg, "--work-ram-out") == 0 && index + 1 < argc) {
            options->work_ram_out_path = argv[++index];
        } else if (strcmp(arg, "--reg") == 0 && index + 1 < argc) {
            int reg = -1;
            uint32_t value = 0u;
            if (!parse_register_assignment(argv[++index], &reg, &value)) {
                return 0;
            }
            options->registers[reg] = value;
            options->register_set[reg] = 1u;
        } else if (strcmp(arg, "--allow-write") == 0 && index + 1 < argc) {
            if (options->probe.write_range_count >= MAX_PROBE_WRITE_RANGES ||
                !parse_write_range(
                    argv[++index],
                    &options->probe.write_ranges[options->probe.write_range_count]
                )) {
                return 0;
            }
            ++options->probe.write_range_count;
        } else if (strcmp(arg, "--stub-read") == 0 && index + 1 < argc) {
            if (options->probe.read_stub_count >= MAX_PROBE_READ_STUBS ||
                !parse_read_stub(
                    argv[++index],
                    &options->probe.read_stubs[options->probe.read_stub_count]
                )) {
                return 0;
            }
            ++options->probe.read_stub_count;
        } else if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
            return -1;
        } else {
            return 0;
        }
    }

    return options->rom_path != NULL ? 1 : 0;
}

static uint8_t *read_file(const char *path, size_t *size)
{
    FILE *file = NULL;
    long length = 0;
    uint8_t *data = NULL;

    *size = 0u;
    file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    length = ftell(file);
    if (length <= 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    data = (uint8_t *)malloc((size_t)length);
    if (data == NULL) {
        fclose(file);
        return NULL;
    }
    if (fread(data, 1u, (size_t)length, file) != (size_t)length) {
        free(data);
        fclose(file);
        return NULL;
    }
    fclose(file);
    *size = (size_t)length;
    return data;
}

static int load_work_ram(const char *path, stf_model2b_bus *model2b)
{
    FILE *file = NULL;
    size_t count = 0u;

    if (path == NULL) {
        return 1;
    }
    file = fopen(path, "rb");
    if (file == NULL) {
        return 0;
    }
    count = fread(model2b->work_ram, 1u, model2b->work_ram_size, file);
    if (ferror(file) != 0) {
        fclose(file);
        return 0;
    }
    if (count == model2b->work_ram_size) {
        int extra = fgetc(file);
        if (extra != EOF) {
            fclose(file);
            return 0;
        }
    }
    fclose(file);
    return 1;
}

static int dump_work_ram(const char *path, const stf_model2b_bus *model2b)
{
    FILE *file = NULL;
    int ok = 0;

    if (path == NULL) {
        return 1;
    }
    file = fopen(path, "wb");
    if (file == NULL) {
        return 0;
    }
    ok = fwrite(model2b->work_ram, 1u, model2b->work_ram_size, file) ==
         model2b->work_ram_size;
    if (fclose(file) != 0) {
        ok = 0;
    }
    return ok;
}

static stf_status probe_device_read(
    void *context,
    uint32_t address,
    void *output,
    size_t size
)
{
    probe_device *probe = (probe_device *)context;
    size_t index = 0u;
    uint32_t value = 0u;
    uint8_t bytes[4];

    if (probe == NULL || output == NULL || size == 0u || size > sizeof(bytes)) {
        return STF_ERROR_UNSUPPORTED;
    }
    for (index = 0u; index < probe->read_stub_count; ++index) {
        if (probe->read_stubs[index].address == address) {
            value = probe->read_stubs[index].value;
            bytes[0] = (uint8_t)value;
            bytes[1] = (uint8_t)(value >> 8u);
            bytes[2] = (uint8_t)(value >> 16u);
            bytes[3] = (uint8_t)(value >> 24u);
            memcpy(output, bytes, size);
            ++probe->accepted_accesses;
            return STF_OK;
        }
    }
    return STF_ERROR_UNSUPPORTED;
}

static stf_status probe_device_write(
    void *context,
    uint32_t address,
    const void *data,
    size_t size
)
{
    probe_device *probe = (probe_device *)context;
    size_t index = 0u;
    uint64_t access_end = (uint64_t)address + (uint64_t)size;

    (void)data;
    if (probe == NULL || size == 0u) {
        return STF_ERROR_UNSUPPORTED;
    }
    for (index = 0u; index < probe->write_range_count; ++index) {
        if ((uint64_t)address >= probe->write_ranges[index].start &&
            access_end <= probe->write_ranges[index].end) {
            ++probe->accepted_accesses;
            return STF_OK;
        }
    }
    return STF_ERROR_UNSUPPORTED;
}

static void write_hex_bytes(FILE *file, const uint8_t *bytes, size_t size)
{
    size_t index = 0u;
    for (index = 0u; index < size; ++index) {
        fprintf(file, "%02x", (unsigned)bytes[index]);
    }
}

static void bus_trace_callback(
    const stf_i960_bus_trace_event *event,
    void *user_data
)
{
    trace_writer *writer = (trace_writer *)user_data;

    if (writer == NULL || writer->file == NULL || event == NULL) {
        return;
    }
    fprintf(
        writer->file,
        "{\"type\":\"memory\",\"step\":%" PRIu64
        ",\"kind\":\"%s\",\"address\":%u,\"size\":%zu,\"status\":\"%s\"",
        event->step,
        event->kind == STF_I960_BUS_ACCESS_WRITE ? "write" : "read",
        event->address,
        event->size,
        stf_status_string(event->status)
    );
    if (event->byte_count != 0u) {
        fputs(",\"bytes\":\"", writer->file);
        write_hex_bytes(writer->file, event->bytes, event->byte_count);
        fputc('"', writer->file);
    }
    fputs("}\n", writer->file);
}

static void step_trace_callback(
    const stf_i960_trace_event *event,
    const stf_i960_cpu *cpu,
    void *user_data
)
{
    trace_writer *writer = (trace_writer *)user_data;
    char formatted[256];

    (void)cpu;
    if (writer == NULL || writer->file == NULL || event == NULL) {
        return;
    }
    if (stf_i960_format_instruction(
            &event->instruction,
            formatted,
            sizeof(formatted)
        ) != STF_OK) {
        snprintf(formatted, sizeof(formatted), "%s", event->instruction.mnemonic);
    }
    fprintf(
        writer->file,
        "{\"type\":\"step\",\"step\":%" PRIu64
        ",\"ip_before\":%u,\"ip_after\":%u,\"mnemonic\":\"%s\","
        "\"text\":\"%s\"}\n",
        event->step,
        event->ip_before,
        event->ip_after,
        event->instruction.mnemonic,
        formatted
    );
}

static int write_state(
    const char *path,
    const stf_i960_cpu *cpu,
    const probe_device *probe,
    const stf_model2b_bus *model2b
)
{
    FILE *file = NULL;
    size_t index = 0u;
    int ok = 1;

    if (path == NULL) {
        return 1;
    }
    file = fopen(path, "w");
    if (file == NULL) {
        return 0;
    }

    fprintf(file, "{\n");
    fprintf(file, "  \"cpu\": {\n");
    fprintf(file, "    \"sat\": %u,\n", cpu->sat);
    fprintf(file, "    \"prcb\": %u,\n", cpu->prcb);
    fprintf(file, "    \"ip\": %u,\n", cpu->ip);
    fprintf(file, "    \"process_control\": %u,\n", cpu->process_control);
    fprintf(file, "    \"arithmetic_control\": %u,\n", cpu->arithmetic_control);
    fprintf(file, "    \"interrupt_control\": %u,\n", cpu->interrupt_control);
    fprintf(file, "    \"compare_result\": %d,\n", (int)cpu->compare_result);
    fprintf(
        file,
        "    \"executed_instructions\": %" PRIu64 ",\n",
        cpu->executed_instructions
    );
    fprintf(file, "    \"procedure_calls\": %" PRIu64 ",\n", cpu->procedure_calls);
    fprintf(file, "    \"procedure_returns\": %" PRIu64 ",\n", cpu->procedure_returns);
    fprintf(file, "    \"local_frame_depth\": %u,\n", cpu->local_frame_depth);
    fprintf(
        file,
        "    \"maximum_local_frame_depth\": %u,\n",
        cpu->maximum_local_frame_depth
    );
    fprintf(file, "    \"registers\": [");
    for (index = 0u; index < STF_I960_REGISTER_COUNT; ++index) {
        fprintf(file, "%s%u", index == 0u ? "" : ", ", cpu->registers[index]);
    }
    fprintf(file, "]\n");
    fprintf(file, "  },\n");
    fprintf(file, "  \"model2b\": {\n");
    fprintf(
        file,
        "    \"geo_write_start_address\": %u,\n",
        model2b != NULL ? model2b->geo_write_start_address : 0u
    );
    fprintf(
        file,
        "    \"geo_read_start_address\": %u,\n",
        model2b != NULL ? model2b->geo_read_start_address : 0u
    );
    fprintf(
        file,
        "    \"geo_fifo_words\": %" PRIu64 ",\n",
        model2b != NULL ? model2b->geo_fifo_words : UINT64_C(0)
    );
    fprintf(
        file,
        "    \"geo_upload_words\": %" PRIu64 ",\n",
        model2b != NULL ? model2b->geo_upload_words : UINT64_C(0)
    );
    fprintf(
        file,
        "    \"copro_upload_words\": %" PRIu64 ",\n",
        model2b != NULL ? model2b->copro_upload_words : UINT64_C(0)
    );
    fprintf(
        file,
        "    \"copro_fifo_input_words\": %" PRIu64 ",\n",
        model2b != NULL ? model2b->copro_fifo_input_words : UINT64_C(0)
    );
    fprintf(
        file,
        "    \"copro_function_words\": %" PRIu64 ",\n",
        model2b != NULL ? model2b->copro_function_words : UINT64_C(0)
    );
    fprintf(
        file,
        "    \"copro_iop_writes\": %" PRIu64 "\n",
        model2b != NULL ? model2b->copro_iop_writes : UINT64_C(0)
    );
    fprintf(file, "  },\n");
    fprintf(file, "  \"probe\": {\n");
    fprintf(
        file,
        "    \"accepted_device_accesses\": %" PRIu64 ",\n",
        probe != NULL ? probe->accepted_accesses : UINT64_C(0)
    );
    fprintf(
        file,
        "    \"exploratory\": %s\n",
        probe != NULL && probe->accepted_accesses != 0u ? "true" : "false"
    );
    fprintf(file, "  }\n");
    fprintf(file, "}\n");

    if (ferror(file) != 0 || fclose(file) != 0) {
        ok = 0;
    }
    return ok;
}

static void apply_registers(
    const runner_options *options,
    stf_i960_cpu *cpu
)
{
    size_t index = 0u;

    if (options->have_stack) {
        cpu->registers[STF_I960_FP_REGISTER] = options->stack_base;
        cpu->registers[1] = options->stack_base + UINT32_C(64);
    }
    for (index = 0u; index < STF_I960_REGISTER_COUNT; ++index) {
        if (options->register_set[index] != 0u) {
            cpu->registers[index] = options->registers[index];
        }
    }
}

int main(int argc, char **argv)
{
    runner_options options;
    uint8_t *rom = NULL;
    size_t rom_size = 0u;
    stf_model2b_bus model2b;
    stf_i960_bus *bus = NULL;
    stf_i960_cpu cpu;
    stf_i960_run_options run_options;
    stf_i960_run_result result;
    stf_status status = STF_OK;
    trace_writer trace;
    int parse_status = parse_arguments(argc, argv, &options);
    int exit_code = 0;

    memset(&model2b, 0, sizeof(model2b));
    memset(&cpu, 0, sizeof(cpu));
    memset(&run_options, 0, sizeof(run_options));
    memset(&result, 0, sizeof(result));
    memset(&trace, 0, sizeof(trace));

    if (parse_status <= 0) {
        usage(argv[0]);
        return parse_status < 0 ? 0 : 64;
    }

    rom = read_file(options.rom_path, &rom_size);
    if (rom == NULL) {
        fprintf(stderr, "failed to read ROM: %s\n", options.rom_path);
        return 66;
    }

    status = stf_model2b_bus_init(&model2b);
    if (status != STF_OK) {
        fprintf(stderr, "Model 2B bus init failed: %s\n", stf_status_string(status));
        free(rom);
        return 70;
    }
    status = stf_model2b_bus_attach_program(&model2b, rom, rom_size);
    if (status != STF_OK) {
        fprintf(stderr, "attach ROM failed: %s\n", stf_status_string(status));
        stf_model2b_bus_destroy(&model2b);
        free(rom);
        return 70;
    }
    if (!load_work_ram(options.work_ram_in_path, &model2b)) {
        fprintf(stderr, "failed to load work RAM image\n");
        exit_code = 66;
        goto cleanup;
    }

    bus = stf_model2b_bus_i960(&model2b);
    if (options.probe.write_range_count != 0u ||
        options.probe.read_stub_count != 0u) {
        stf_model2b_bus_set_device_callbacks(
            &model2b,
            &options.probe,
            probe_device_read,
            probe_device_write
        );
    }
    if (options.trace_path != NULL) {
        trace.file = fopen(options.trace_path, "w");
        if (trace.file == NULL) {
            fprintf(stderr, "failed to open trace: %s\n", options.trace_path);
            exit_code = 73;
            goto cleanup;
        }
        stf_i960_bus_set_trace(bus, bus_trace_callback, &trace);
    }

    if (options.reset_from_prcb) {
        status = stf_i960_cpu_reset_from_bus(
            &cpu,
            bus,
            options.sat,
            options.prcb,
            options.entry
        );
    } else {
        stf_i960_cpu_reset(
            &cpu,
            options.sat,
            options.prcb,
            options.entry
        );
    }
    if (status != STF_OK) {
        fprintf(stderr, "CPU reset failed: %s\n", stf_status_string(status));
        exit_code = 70;
        goto cleanup;
    }
    apply_registers(&options, &cpu);

    run_options.stop_address = options.stop;
    run_options.max_steps = options.max_steps;
    run_options.stop_on_self_branch = true;
    if (trace.file != NULL) {
        run_options.trace_callback = step_trace_callback;
        run_options.trace_user_data = &trace;
    }

    stf_model2b_bus_clear_fault(&model2b);
    status = stf_i960_run(&cpu, bus, &run_options, &result);

    printf(
        "halt=%s status=%s ip=0x%08" PRIx32 " executed=%" PRIu64 "\n",
        stf_i960_halt_reason_name(result.halt_reason),
        stf_status_string(result.status),
        result.halt_address,
        result.executed_instructions
    );

    {
        const stf_model2b_fault *fault = stf_model2b_bus_last_fault(&model2b);
        if (fault != NULL && fault->valid) {
            {
                const char *region = stf_model2b_region_hint(fault->address);
                const char *symbol = stf_model2b_symbol_hint(fault->address);
                printf(
                    "first-unmodeled-access kind=%s address=0x%08" PRIx32
                    " size=%zu status=%s region=%s symbol=%s\n",
                    fault->write ? "write" : "read",
                    fault->address,
                    fault->size,
                    stf_status_string(fault->status),
                    region,
                    symbol != NULL ? symbol : "-"
                );
            }
            if (trace.file != NULL) {
                {
                    const char *region = stf_model2b_region_hint(fault->address);
                    const char *symbol = stf_model2b_symbol_hint(fault->address);
                    fprintf(
                        trace.file,
                        "{\"type\":\"fault\",\"kind\":\"%s\","
                        "\"address\":%u,\"size\":%zu,\"status\":\"%s\","
                        "\"region\":\"%s\",\"symbol\":\"%s\"}\n",
                        fault->write ? "write" : "read",
                        fault->address,
                        fault->size,
                        stf_status_string(fault->status),
                        region,
                        symbol != NULL ? symbol : ""
                    );
                }
            }
        }
    }

    printf(
        "model2b geo_words=%" PRIu64
        " geo_upload=%" PRIu64
        " copro_upload=%" PRIu64
        " copro_fifo_in=%" PRIu64
        " copro_function=%" PRIu64
        " copro_iop=%" PRIu64 "\n",
        model2b.geo_fifo_words,
        model2b.geo_upload_words,
        model2b.copro_upload_words,
        model2b.copro_fifo_input_words,
        model2b.copro_function_words,
        model2b.copro_iop_writes
    );

    if (options.probe.accepted_accesses != 0u) {
        printf(
            "probe-device-accesses=%" PRIu64
            " state=exploratory-not-reference\n",
            options.probe.accepted_accesses
        );
    }

    if (!write_state(options.state_path, &cpu, &options.probe, &model2b)) {
        fprintf(stderr, "failed to write state JSON\n");
        exit_code = 74;
    }
    if (!dump_work_ram(options.work_ram_out_path, &model2b)) {
        fprintf(stderr, "failed to dump work RAM\n");
        exit_code = 74;
    }

    if (exit_code == 0 && status != STF_OK) {
        exit_code = 2;
    }

cleanup:
    if (trace.file != NULL) {
        fclose(trace.file);
    }
    stf_model2b_bus_destroy(&model2b);
    free(rom);
    return exit_code;
}
