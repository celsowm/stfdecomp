#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stf/recovery/i960/decoder.h"
#include "stf/recovery/i960/executor.h"
#include "stf/recovery/model2b_bus.h"
#include "stf/recovery/model2b_map.h"

enum {
    MAX_REGISTER_SEEDS = 64,
    MAX_WRITE_SEEDS = 256,
    MAX_WATCHES = 64
};

typedef struct register_seed {
    unsigned index;
    uint32_t value;
} register_seed;

typedef struct write_seed {
    uint32_t address;
    uint32_t value;
} write_seed;

typedef struct watch_range {
    uint32_t address;
    size_t size;
} watch_range;

typedef struct options {
    const char *rom_path;
    const char *work_ram_path;
    const char *trace_path;
    const char *state_path;
    uint32_t start;
    uint32_t stop;
    uint32_t sat;
    uint32_t prcb;
    uint64_t max_steps;
    int stop_on_self_branch;
    register_seed registers[MAX_REGISTER_SEEDS];
    size_t register_count;
    write_seed writes[MAX_WRITE_SEEDS];
    size_t write_count;
    watch_range watches[MAX_WATCHES];
    size_t watch_count;
} options;

typedef struct trace_writer {
    FILE *file;
} trace_writer;

static void usage(const char *program)
{
    fprintf(
        stderr,
        "usage: %s --rom FILE --start ADDRESS [options]\n"
        "\n"
        "options:\n"
        "  --stop ADDRESS          stop before executing ADDRESS\n"
        "  --max-steps N           default: 100000\n"
        "  --sat VALUE             initial SAT register\n"
        "  --prcb VALUE            initial PRCB register\n"
        "  --reg REG=VALUE         seed register; repeatable\n"
        "  --write32 ADDR=VALUE    seed work RAM; repeatable\n"
        "  --work-ram FILE         preload up to 1 MiB work RAM\n"
        "  --watch ADDR:SIZE       include ROM/RAM bytes in final state\n"
        "  --trace FILE            write JSONL execution/memory trace\n"
        "  --state FILE            write final JSON state snapshot\n"
        "  --no-self-branch-stop   allow intentional self branches\n"
        "\n"
        "register names: pfp, sp, rip, r0..r15, g0..g14, fp\n",
        program
    );
}

static int parse_u32(const char *text, uint32_t *value)
{
    char *end = NULL;
    unsigned long long parsed = 0u;

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
    unsigned long long parsed = 0u;

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

static int parse_register(const char *text, unsigned *index)
{
    uint32_t numeric = 0u;

    if (strcmp(text, "pfp") == 0) {
        *index = 0u;
        return 1;
    }
    if (strcmp(text, "sp") == 0) {
        *index = 1u;
        return 1;
    }
    if (strcmp(text, "rip") == 0) {
        *index = 2u;
        return 1;
    }
    if (strcmp(text, "fp") == 0) {
        *index = STF_I960_FP_REGISTER;
        return 1;
    }
    if (text[0] == 'r' && parse_u32(text + 1, &numeric) && numeric <= 15u) {
        *index = (unsigned)numeric;
        return 1;
    }
    if (text[0] == 'g' && parse_u32(text + 1, &numeric) && numeric <= 14u) {
        *index = 16u + (unsigned)numeric;
        return 1;
    }
    if (parse_u32(text, &numeric) && numeric < STF_I960_REGISTER_COUNT) {
        *index = (unsigned)numeric;
        return 1;
    }
    return 0;
}

static int split_pair(
    const char *text,
    char separator,
    char *left,
    size_t left_size,
    const char **right
)
{
    const char *mark = strchr(text, separator);
    size_t length = 0u;

    if (mark == NULL || mark == text || mark[1] == '\0') {
        return 0;
    }
    length = (size_t)(mark - text);
    if (length + 1u > left_size) {
        return 0;
    }
    memcpy(left, text, length);
    left[length] = '\0';
    *right = mark + 1;
    return 1;
}

static int parse_register_seed(const char *text, register_seed *seed)
{
    char left[32];
    const char *right = NULL;

    if (!split_pair(text, '=', left, sizeof(left), &right) ||
        !parse_register(left, &seed->index) ||
        !parse_u32(right, &seed->value)) {
        return 0;
    }
    return 1;
}

static int parse_write_seed(const char *text, write_seed *seed)
{
    char left[32];
    const char *right = NULL;

    if (!split_pair(text, '=', left, sizeof(left), &right) ||
        !parse_u32(left, &seed->address) ||
        !parse_u32(right, &seed->value)) {
        return 0;
    }
    return 1;
}

static int parse_watch(const char *text, watch_range *watch)
{
    char left[32];
    const char *right = NULL;
    uint32_t size = 0u;

    if (!split_pair(text, ':', left, sizeof(left), &right) ||
        !parse_u32(left, &watch->address) ||
        !parse_u32(right, &size) ||
        size == 0u) {
        return 0;
    }
    watch->size = size;
    return 1;
}

static int parse_options(int argc, char **argv, options *result)
{
    int index = 1;

    memset(result, 0, sizeof(*result));
    result->max_steps = 100000u;
    result->stop_on_self_branch = 1;

    while (index < argc) {
        const char *arg = argv[index++];

        if (strcmp(arg, "--rom") == 0 && index < argc) {
            result->rom_path = argv[index++];
        } else if (strcmp(arg, "--work-ram") == 0 && index < argc) {
            result->work_ram_path = argv[index++];
        } else if (strcmp(arg, "--trace") == 0 && index < argc) {
            result->trace_path = argv[index++];
        } else if (strcmp(arg, "--state") == 0 && index < argc) {
            result->state_path = argv[index++];
        } else if (strcmp(arg, "--start") == 0 && index < argc) {
            if (!parse_u32(argv[index++], &result->start)) {
                return 0;
            }
        } else if (strcmp(arg, "--stop") == 0 && index < argc) {
            if (!parse_u32(argv[index++], &result->stop)) {
                return 0;
            }
        } else if (strcmp(arg, "--sat") == 0 && index < argc) {
            if (!parse_u32(argv[index++], &result->sat)) {
                return 0;
            }
        } else if (strcmp(arg, "--prcb") == 0 && index < argc) {
            if (!parse_u32(argv[index++], &result->prcb)) {
                return 0;
            }
        } else if (strcmp(arg, "--max-steps") == 0 && index < argc) {
            if (!parse_u64(argv[index++], &result->max_steps) ||
                result->max_steps == 0u) {
                return 0;
            }
        } else if (strcmp(arg, "--reg") == 0 && index < argc) {
            if (result->register_count >= MAX_REGISTER_SEEDS ||
                !parse_register_seed(
                    argv[index++],
                    &result->registers[result->register_count]
                )) {
                return 0;
            }
            ++result->register_count;
        } else if (strcmp(arg, "--write32") == 0 && index < argc) {
            if (result->write_count >= MAX_WRITE_SEEDS ||
                !parse_write_seed(
                    argv[index++],
                    &result->writes[result->write_count]
                )) {
                return 0;
            }
            ++result->write_count;
        } else if (strcmp(arg, "--watch") == 0 && index < argc) {
            if (result->watch_count >= MAX_WATCHES ||
                !parse_watch(argv[index++], &result->watches[result->watch_count])) {
                return 0;
            }
            ++result->watch_count;
        } else if (strcmp(arg, "--no-self-branch-stop") == 0) {
            result->stop_on_self_branch = 0;
        } else {
            return 0;
        }
    }

    return result->rom_path != NULL;
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

static int load_work_ram(stf_model2b_bus *model2b, const char *path)
{
    size_t size = 0u;
    uint8_t *data = read_file(path, &size);

    if (data == NULL || size > model2b->work_ram_size) {
        free(data);
        return 0;
    }
    memcpy(model2b->work_ram, data, size);
    free(data);
    return 1;
}

static void print_hex_bytes(FILE *file, const uint8_t *bytes, size_t size)
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
        ",\"kind\":\"%s\",\"address\":%" PRIu32
        ",\"size\":%zu",
        event->step,
        event->kind == STF_I960_BUS_ACCESS_WRITE ? "write" : "read",
        event->address,
        event->size
    );
    if (event->byte_count != 0u) {
        fputs(",\"bytes\":\"", writer->file);
        print_hex_bytes(writer->file, event->bytes, event->byte_count);
        fputc('"', writer->file);
    }
    if (event->status != STF_OK) {
        fprintf(
            writer->file,
            ",\"status\":%d,\"status_text\":\"%s\"",
            (int)event->status,
            stf_status_string(event->status)
        );
    }
    fputs("}\n", writer->file);
}

static void write_step_trace(
    FILE *file,
    const stf_i960_trace_event *event,
    const stf_i960_cpu *cpu
)
{
    char text[160];

    if (file == NULL || event == NULL || cpu == NULL) {
        return;
    }
    text[0] = '\0';
    (void)stf_i960_format_instruction(
        &event->instruction,
        text,
        sizeof(text)
    );
    fprintf(
        file,
        "{\"type\":\"step\",\"step\":%" PRIu64
        ",\"ip_before\":%" PRIu32
        ",\"ip_after\":%" PRIu32
        ",\"word\":%" PRIu32
        ",\"size\":%u,\"mnemonic\":\"%s\",\"call_depth\":%" PRIu32 "}\n",
        event->step,
        event->ip_before,
        event->ip_after,
        event->instruction.words[0],
        (unsigned)event->instruction.size,
        event->instruction.mnemonic != NULL ? event->instruction.mnemonic : "",
        cpu->local_frame_depth
    );
}

static void write_halt_trace(
    FILE *file,
    uint64_t step,
    const stf_i960_cpu *cpu,
    stf_status status,
    const char *reason,
    const stf_model2b_fault *fault
)
{
    if (file == NULL) {
        return;
    }

    fprintf(
        file,
        "{\"type\":\"halt\",\"step\":%" PRIu64
        ",\"ip\":%" PRIu32 ",\"status\":%d,\"status_text\":\"%s\""
        ",\"reason\":\"%s\"",
        step,
        cpu->ip,
        (int)status,
        stf_status_string(status),
        reason
    );
    if (fault != NULL && fault->valid) {
        fprintf(
            file,
            ",\"bus_fault\":{\"kind\":\"%s\",\"address\":%" PRIu32
            ",\"size\":%zu,\"status\":%d}",
            fault->write ? "write" : "read",
            fault->address,
            fault->size,
            (int)fault->status
        );
    }
    fputs("}\n", file);
}

static const uint8_t *watch_pointer(
    const stf_model2b_bus *model2b,
    const watch_range *watch
)
{
    const uint64_t end = (uint64_t)watch->address + (uint64_t)watch->size;
    const uint64_t ram_end =
        (uint64_t)STF_MODEL2B_WORK_RAM_BASE + model2b->work_ram_size;

    if ((uint64_t)watch->address >= STF_MODEL2B_WORK_RAM_BASE &&
        end <= ram_end) {
        return model2b->work_ram +
            (watch->address - STF_MODEL2B_WORK_RAM_BASE);
    }
    if ((uint64_t)watch->address < model2b->rom_size &&
        end <= model2b->rom_size) {
        return model2b->rom + watch->address;
    }
    return NULL;
}

static int write_state(
    const char *path,
    const stf_i960_cpu *cpu,
    const stf_model2b_bus *model2b,
    stf_status status,
    const char *reason,
    const watch_range *watches,
    size_t watch_count
)
{
    FILE *file = NULL;
    size_t index = 0u;
    const stf_model2b_fault *fault = stf_model2b_bus_last_fault(model2b);

    file = fopen(path, "w");
    if (file == NULL) {
        return 0;
    }

    fprintf(
        file,
        "{\n"
        "  \"halt\": {\"reason\": \"%s\", \"status\": %d, "
        "\"status_text\": \"%s\"",
        reason,
        (int)status,
        stf_status_string(status)
    );
    if (fault != NULL && fault->valid) {
        fprintf(
            file,
            ", \"bus_fault\": {\"kind\": \"%s\", \"address\": %" PRIu32
            ", \"size\": %zu, \"status\": %d}",
            fault->write ? "write" : "read",
            fault->address,
            fault->size,
            (int)fault->status
        );
    }
    fputs("},\n", file);

    fprintf(
        file,
        "  \"cpu\": {\n"
        "    \"ip\": %" PRIu32 ",\n"
        "    \"sat\": %" PRIu32 ",\n"
        "    \"prcb\": %" PRIu32 ",\n"
        "    \"process_control\": %" PRIu32 ",\n"
        "    \"arithmetic_control\": %" PRIu32 ",\n"
        "    \"interrupt_control\": %" PRIu32 ",\n"
        "    \"compare_result\": %d,\n"
        "    \"executed_instructions\": %" PRIu64 ",\n"
        "    \"procedure_calls\": %" PRIu64 ",\n"
        "    \"procedure_returns\": %" PRIu64 ",\n"
        "    \"local_frame_depth\": %" PRIu32 ",\n"
        "    \"maximum_local_frame_depth\": %" PRIu32 ",\n"
        "    \"registers\": [",
        cpu->ip,
        cpu->sat,
        cpu->prcb,
        cpu->process_control,
        cpu->arithmetic_control,
        cpu->interrupt_control,
        (int)cpu->compare_result,
        cpu->executed_instructions,
        cpu->procedure_calls,
        cpu->procedure_returns,
        cpu->local_frame_depth,
        cpu->maximum_local_frame_depth
    );

    for (index = 0u; index < STF_I960_REGISTER_COUNT; ++index) {
        fprintf(
            file,
            "%s%" PRIu32,
            index == 0u ? "" : ", ",
            cpu->registers[index]
        );
    }
    fputs("]\n  },\n", file);

    fputs("  \"memory\": [", file);
    for (index = 0u; index < watch_count; ++index) {
        const uint8_t *data = watch_pointer(model2b, &watches[index]);
        size_t byte_index = 0u;

        if (data == NULL) {
            fclose(file);
            return 0;
        }
        fprintf(
            file,
            "%s\n    {\"address\": %" PRIu32 ", \"size\": %zu, \"bytes\": [",
            index == 0u ? "" : ",",
            watches[index].address,
            watches[index].size
        );
        for (byte_index = 0u; byte_index < watches[index].size; ++byte_index) {
            fprintf(
                file,
                "%s%u",
                byte_index == 0u ? "" : ", ",
                (unsigned)data[byte_index]
            );
        }
        fputs("]}", file);
    }
    fputs(watch_count == 0u ? "]\n" : "\n  ]\n", file);
    fputs("}\n", file);

    return fclose(file) == 0;
}

static int apply_seeds(
    stf_model2b_bus *model2b,
    stf_i960_cpu *cpu,
    const options *opts
)
{
    size_t index = 0u;

    for (index = 0u; index < opts->register_count; ++index) {
        cpu->registers[opts->registers[index].index] =
            opts->registers[index].value;
    }
    for (index = 0u; index < opts->write_count; ++index) {
        if (stf_i960_bus_write_u32(
                stf_model2b_bus_i960(model2b),
                opts->writes[index].address,
                opts->writes[index].value
            ) != STF_OK) {
            fprintf(
                stderr,
                "cannot seed 0x%08" PRIx32 "\n",
                opts->writes[index].address
            );
            return 0;
        }
    }
    return 1;
}

int main(int argc, char **argv)
{
    options opts;
    uint8_t *rom = NULL;
    size_t rom_size = 0u;
    stf_model2b_bus model2b;
    stf_i960_cpu cpu;
    trace_writer trace;
    stf_status status = STF_OK;
    const char *halt_reason = "maximum steps";
    uint64_t iteration = 0u;
    int exit_code = 0;

    memset(&model2b, 0, sizeof(model2b));
    memset(&trace, 0, sizeof(trace));

    if (!parse_options(argc, argv, &opts)) {
        usage(argv[0]);
        return 2;
    }

    rom = read_file(opts.rom_path, &rom_size);
    if (rom == NULL) {
        fprintf(stderr, "cannot read ROM image: %s\n", opts.rom_path);
        return 3;
    }
    if ((uint64_t)opts.start + 4u > rom_size) {
        fprintf(stderr, "start address is outside ROM image\n");
        free(rom);
        return 4;
    }

    status = stf_model2b_bus_init(&model2b);
    if (status != STF_OK ||
        stf_model2b_bus_attach_program(&model2b, rom, rom_size) != STF_OK) {
        fprintf(stderr, "cannot initialize Model 2B bus\n");
        free(rom);
        stf_model2b_bus_destroy(&model2b);
        return 5;
    }

    if (opts.work_ram_path != NULL &&
        !load_work_ram(&model2b, opts.work_ram_path)) {
        fprintf(stderr, "cannot preload work RAM: %s\n", opts.work_ram_path);
        stf_model2b_bus_destroy(&model2b);
        free(rom);
        return 6;
    }

    stf_i960_cpu_reset(&cpu, opts.sat, opts.prcb, opts.start);
    if (!apply_seeds(&model2b, &cpu, &opts)) {
        stf_model2b_bus_destroy(&model2b);
        free(rom);
        return 7;
    }

    if (opts.trace_path != NULL) {
        trace.file = fopen(opts.trace_path, "w");
        if (trace.file == NULL) {
            fprintf(stderr, "cannot open trace output: %s\n", opts.trace_path);
            stf_model2b_bus_destroy(&model2b);
            free(rom);
            return 8;
        }
        stf_i960_bus_set_trace(
            stf_model2b_bus_i960(&model2b),
            bus_trace_callback,
            &trace
        );
    }

    stf_model2b_bus_clear_fault(&model2b);

    for (iteration = 0u; iteration < opts.max_steps; ++iteration) {
        stf_i960_trace_event event;
        const uint64_t next_step = cpu.executed_instructions + 1u;
        const uint32_t ip_before = cpu.ip;

        if (opts.stop != 0u && cpu.ip == opts.stop) {
            halt_reason = "stop address";
            break;
        }

        stf_i960_bus_set_trace_step(
            stf_model2b_bus_i960(&model2b),
            next_step
        );
        status = stf_i960_step(
            &cpu,
            stf_model2b_bus_i960(&model2b),
            &event
        );
        if (status != STF_OK) {
            const stf_model2b_fault *fault =
                stf_model2b_bus_last_fault(&model2b);
            halt_reason =
                fault != NULL && fault->valid
                    ? "unmapped Model 2B access"
                    : "unsupported i960 path";
            write_halt_trace(
                trace.file,
                next_step,
                &cpu,
                status,
                halt_reason,
                fault
            );
            exit_code = 10;
            break;
        }

        write_step_trace(trace.file, &event, &cpu);

        if (opts.stop_on_self_branch && cpu.ip == ip_before) {
            halt_reason = "self branch";
            break;
        }
    }

    if (iteration == opts.max_steps) {
        halt_reason = "maximum steps";
    }

    if (trace.file != NULL) {
        fclose(trace.file);
        trace.file = NULL;
    }

    if (opts.state_path != NULL &&
        !write_state(
            opts.state_path,
            &cpu,
            &model2b,
            status,
            halt_reason,
            opts.watches,
            opts.watch_count
        )) {
        fprintf(stderr, "cannot write state snapshot: %s\n", opts.state_path);
        if (exit_code == 0) {
            exit_code = 11;
        }
    }

    printf(
        "halt=%s status=%s ip=0x%08" PRIx32
        " executed=%" PRIu64 "\n",
        halt_reason,
        stf_status_string(status),
        cpu.ip,
        cpu.executed_instructions
    );

    {
        const stf_model2b_fault *fault =
            stf_model2b_bus_last_fault(&model2b);
        if (fault != NULL && fault->valid) {
            {
                const char *region = stf_model2b_region_hint(fault->address);
                const char *symbol = stf_model2b_symbol_hint(fault->address);
                printf(
                    "model2b_fault=%s address=0x%08" PRIx32
                    " size=%zu status=%s region=%s symbol=%s\n",
                    fault->write ? "write" : "read",
                    fault->address,
                    fault->size,
                    stf_status_string(fault->status),
                    region,
                    symbol != NULL ? symbol : "-"
                );
            }
        }
    }

    stf_model2b_bus_destroy(&model2b);
    free(rom);
    return exit_code;
}
