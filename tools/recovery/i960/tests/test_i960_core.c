#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "stf/recovery/i960/decoder.h"
#include "stf/recovery/i960/executor.h"
#include "stf/recovery/model2b_bus.h"

static void write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

static int test_decoder(void)
{
    uint8_t image[32] = {0u};
    stf_i960_instruction instruction;
    char text[128];

    write_le32(image + 0u, 0x8c803000u);
    write_le32(image + 4u, 0x00e00000u);
    if (stf_i960_decode(image, sizeof(image), 0u, &instruction) != STF_OK) {
        return 1;
    }
    if (strcmp(instruction.mnemonic, "lda") != 0 || instruction.size != 8u ||
        instruction.operand_count != 2u ||
        instruction.operands[0].value.memory.resolved_address != 0x00e00000u) {
        return 2;
    }
    if (stf_i960_format_instruction(&instruction, text, sizeof(text)) != STF_OK ||
        strstr(text, "0x00e00000") == NULL || strstr(text, "g0") == NULL) {
        return 3;
    }

    write_le32(image + 8u, 0x59981901u);
    if (stf_i960_decode(image, sizeof(image), 8u, &instruction) != STF_OK ||
        strcmp(instruction.mnemonic, "subo") != 0) {
        return 4;
    }

    write_le32(image + 12u, 0x12000014u);
    if (stf_i960_decode(image, sizeof(image), 12u, &instruction) != STF_OK ||
        strcmp(instruction.mnemonic, "be") != 0 ||
        instruction.target != 0x20u || !instruction.conditional) {
        return 5;
    }

    write_le32(image + 16u, 0x0a000000u);
    if (stf_i960_decode(image, sizeof(image), 16u, &instruction) != STF_OK ||
        instruction.flow != STF_I960_FLOW_RETURN ||
        instruction.has_fallthrough) {
        return 6;
    }

    return 0;
}

static int test_executor_memory_loop(void)
{
    uint8_t image[128];
    stf_model2b_bus model2b;
    stf_i960_cpu cpu;
    stf_i960_run_options options;
    stf_i960_run_result result;
    stf_status status = STF_OK;

    memset(image, 0xff, sizeof(image));
    write_le32(image + 0u, 0x8c703000u);
    write_le32(image + 4u, STF_MODEL2B_WORK_RAM_BASE);
    write_le32(image + 8u, 0x5c781e00u);
    write_le32(image + 12u, 0x8c683000u);
    write_le32(image + 16u, 3u);
    write_le32(image + 20u, 0x927b9000u);
    write_le32(image + 24u, 0x8c73a004u);
    write_le32(image + 28u, 0x5a6b4b01u);
    write_le32(image + 32u, 0x14fffff4u);

    if (stf_model2b_bus_init(&model2b) != STF_OK) {
        return 1;
    }
    if (stf_model2b_bus_attach_program(&model2b, image, sizeof(image)) != STF_OK) {
        stf_model2b_bus_destroy(&model2b);
        return 2;
    }

    memset(model2b.work_ram, 0xa5, model2b.work_ram_size);
    stf_i960_cpu_reset(&cpu, 0u, 0u, 0u);
    memset(&options, 0, sizeof(options));
    options.stop_address = 36u;
    options.max_steps = 64u;
    options.stop_on_self_branch = true;

    status = stf_i960_run(
        &cpu,
        stf_model2b_bus_i960(&model2b),
        &options,
        &result
    );

    if (status != STF_OK || result.halt_reason != STF_I960_HALT_STOP_ADDRESS ||
        cpu.ip != 36u || cpu.registers[13] != 0u ||
        cpu.registers[14] != STF_MODEL2B_WORK_RAM_BASE + 12u ||
        model2b.work_ram[0] != 0u || model2b.work_ram[4] != 0u ||
        model2b.work_ram[8] != 0u || model2b.work_ram[12] != 0xa5u) {
        stf_model2b_bus_destroy(&model2b);
        return 3;
    }

    stf_model2b_bus_destroy(&model2b);
    return 0;
}

static int test_call_return_frames(void)
{
    uint8_t image[128];
    stf_model2b_bus model2b;
    stf_i960_cpu cpu;
    stf_i960_run_options options;
    stf_i960_run_result result;
    stf_status status = STF_OK;

    memset(image, 0xff, sizeof(image));
    write_le32(image + 0x00u, 0x09000020u);
    write_le32(image + 0x20u, 0x09000020u);
    write_le32(image + 0x24u, 0x0a000000u);
    write_le32(image + 0x40u, 0x0a000000u);

    if (stf_model2b_bus_init(&model2b) != STF_OK) {
        return 1;
    }
    if (stf_model2b_bus_attach_program(&model2b, image, sizeof(image)) != STF_OK) {
        stf_model2b_bus_destroy(&model2b);
        return 2;
    }

    stf_i960_cpu_reset(&cpu, 0u, 0u, 0u);
    cpu.registers[1] = 0x00501003u;
    cpu.registers[3] = 0x12345678u;
    cpu.registers[STF_I960_FP_REGISTER] = STF_MODEL2B_WORK_RAM_BASE;

    memset(&options, 0, sizeof(options));
    options.stop_address = 4u;
    options.max_steps = 16u;
    options.stop_on_self_branch = true;

    status = stf_i960_run(
        &cpu,
        stf_model2b_bus_i960(&model2b),
        &options,
        &result
    );

    if (status != STF_OK || result.halt_reason != STF_I960_HALT_STOP_ADDRESS ||
        cpu.ip != 4u || cpu.local_frame_depth != 0u ||
        cpu.maximum_local_frame_depth != 2u ||
        cpu.procedure_calls != 2u || cpu.procedure_returns != 2u ||
        cpu.registers[1] != 0x00501003u ||
        cpu.registers[3] != 0x12345678u ||
        cpu.registers[STF_I960_FP_REGISTER] != STF_MODEL2B_WORK_RAM_BASE) {
        stf_model2b_bus_destroy(&model2b);
        return 3;
    }

    stf_model2b_bus_destroy(&model2b);
    return 0;
}

static int test_chkbit_boolean_branches(void)
{
    uint8_t image[16] = {0u};
    stf_model2b_bus model2b;
    stf_i960_cpu cpu;
    stf_status status = STF_OK;

    /*
     * chkbit 31, r5
     * bno +8
     *
     * Intel documents chkbit as CC=000 for clear and CC=010 for set.
     * Therefore bno is branch-if-false and bo is branch-if-true.
     */
    write_le32(image + 0u, UINT32_C(0x5A014F1F));
    write_le32(image + 4u, UINT32_C(0x10000008));

    if (stf_model2b_bus_init(&model2b) != STF_OK) {
        return 1;
    }
    if (stf_model2b_bus_attach_program(&model2b, image, sizeof(image)) != STF_OK) {
        stf_model2b_bus_destroy(&model2b);
        return 2;
    }

    stf_i960_cpu_reset(&cpu, 0u, 0u, 0u);
    cpu.registers[5] = 0u;

    status = stf_i960_step(
        &cpu, stf_model2b_bus_i960(&model2b), NULL
    );
    if (status != STF_OK || cpu.ip != 4u ||
        cpu.compare_result != STF_I960_COMPARE_NONE ||
        (cpu.arithmetic_control & UINT32_C(7)) != 0u) {
        stf_model2b_bus_destroy(&model2b);
        return 3;
    }

    status = stf_i960_step(
        &cpu, stf_model2b_bus_i960(&model2b), NULL
    );
    if (status != STF_OK || cpu.ip != 12u) {
        stf_model2b_bus_destroy(&model2b);
        return 4;
    }

    stf_i960_cpu_reset(&cpu, 0u, 0u, 0u);
    cpu.registers[5] = UINT32_C(0x80000000);

    status = stf_i960_step(
        &cpu, stf_model2b_bus_i960(&model2b), NULL
    );
    if (status != STF_OK || cpu.ip != 4u ||
        cpu.compare_result != STF_I960_COMPARE_EQUAL ||
        (cpu.arithmetic_control & UINT32_C(7)) != UINT32_C(2)) {
        stf_model2b_bus_destroy(&model2b);
        return 5;
    }

    status = stf_i960_step(
        &cpu, stf_model2b_bus_i960(&model2b), NULL
    );
    if (status != STF_OK || cpu.ip != 8u) {
        stf_model2b_bus_destroy(&model2b);
        return 6;
    }

    write_le32(image + 4u, UINT32_C(0x17000008));
    stf_i960_cpu_reset(&cpu, 0u, 0u, 0u);
    cpu.registers[5] = UINT32_C(0x80000000);

    status = stf_i960_step(
        &cpu, stf_model2b_bus_i960(&model2b), NULL
    );
    if (status != STF_OK) {
        stf_model2b_bus_destroy(&model2b);
        return 7;
    }
    status = stf_i960_step(
        &cpu, stf_model2b_bus_i960(&model2b), NULL
    );
    if (status != STF_OK || cpu.ip != 12u) {
        stf_model2b_bus_destroy(&model2b);
        return 8;
    }

    stf_model2b_bus_destroy(&model2b);
    return 0;
}

static int test_model2b_fail_closed(void)
{
    uint8_t image[16] = {0u};
    uint32_t value = 0u;
    stf_model2b_bus model2b;
    stf_status status = STF_OK;

    if (stf_model2b_bus_init(&model2b) != STF_OK) {
        return 1;
    }
    if (stf_model2b_bus_attach_program(&model2b, image, sizeof(image)) != STF_OK) {
        stf_model2b_bus_destroy(&model2b);
        return 2;
    }

    status = stf_i960_bus_read_u32(
        stf_model2b_bus_i960(&model2b),
        0x00884000u,
        &value
    );
    if (status != STF_ERROR_UNSUPPORTED) {
        stf_model2b_bus_destroy(&model2b);
        return 3;
    }

    status = stf_i960_bus_write_u32(
        stf_model2b_bus_i960(&model2b),
        0x00000000u,
        0x12345678u
    );
    if (status != STF_ERROR_UNSUPPORTED) {
        stf_model2b_bus_destroy(&model2b);
        return 4;
    }

    stf_model2b_bus_destroy(&model2b);
    return 0;
}

int main(void)
{
    int result = 0;

    result = test_decoder();
    if (result != 0) {
        fprintf(stderr, "decoder test failed: %d\n", result);
        return 10 + result;
    }

    result = test_executor_memory_loop();
    if (result != 0) {
        fprintf(stderr, "executor memory-loop test failed: %d\n", result);
        return 20 + result;
    }

    result = test_call_return_frames();
    if (result != 0) {
        fprintf(stderr, "call/return test failed: %d\n", result);
        return 30 + result;
    }

    result = test_chkbit_boolean_branches();
    if (result != 0) {
        fprintf(stderr, "chkbit boolean-branch test failed: %d\n", result);
        return 40 + result;
    }

    result = test_model2b_fail_closed();
    if (result != 0) {
        fprintf(stderr, "Model 2B fail-closed test failed: %d\n", result);
        return 50 + result;
    }

    puts("stf recovery i960 tests: ok");
    return 0;
}
