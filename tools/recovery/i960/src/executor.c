/*
 * Derived from celsowm/vf2-decomp i960 recovery code.
 * Copyright (c) 2026, vf2-decomp contributors.
 * BSD-3-Clause terms are retained in LICENSE.vf2-decomp.
 * Hardware access was refactored behind stf_i960_bus for Model 2B recovery.
 */
#include "stf/recovery/i960/executor.h"

#include <limits.h>
#include <math.h>
#include <string.h>

#include "stf/recovery/i960/decoder.h"


static uint32_t align_stack_frame(uint32_t stack_pointer)
{
    return (stack_pointer + UINT32_C(63)) & ~UINT32_C(63);
}

static stf_status procedure_call_typed(
    stf_i960_cpu *cpu,
    uint32_t target,
    uint32_t return_address,
    uint32_t call_type,
    uint32_t stack_pointer
)
{
    uint32_t old_frame_pointer = 0u;
    uint32_t new_frame_pointer = 0u;

    if (cpu->local_frame_depth >= STF_I960_MAX_LOCAL_FRAMES) {
        return STF_ERROR_OUT_OF_BOUNDS;
    }

    cpu->registers[2] = return_address;
    memcpy(
        cpu->local_frames[cpu->local_frame_depth].registers,
        cpu->registers,
        STF_I960_LOCAL_REGISTER_COUNT * sizeof(uint32_t)
    );

    old_frame_pointer = cpu->registers[STF_I960_FP_REGISTER];
    new_frame_pointer = align_stack_frame(stack_pointer);

    memset(cpu->registers, 0, STF_I960_LOCAL_REGISTER_COUNT * sizeof(uint32_t));
    cpu->registers[0] = (old_frame_pointer & ~UINT32_C(7)) | (call_type & UINT32_C(7));
    cpu->registers[1] = new_frame_pointer + UINT32_C(64);
    cpu->registers[STF_I960_FP_REGISTER] = new_frame_pointer;
    cpu->ip = target;

    ++cpu->local_frame_depth;
    if (cpu->local_frame_depth > cpu->maximum_local_frame_depth) {
        cpu->maximum_local_frame_depth = cpu->local_frame_depth;
    }
    ++cpu->procedure_calls;
    if ((call_type & UINT32_C(7)) == UINT32_C(7)) {
        ++cpu->interrupt_entries;
    }
    return STF_OK;
}

static stf_status procedure_call(
    stf_i960_cpu *cpu,
    uint32_t target,
    uint32_t return_address
)
{
    return procedure_call_typed(
        cpu,
        target,
        return_address,
        0u,
        cpu->registers[1]
    );
}

static stf_status procedure_return(stf_i960_cpu *cpu, stf_i960_bus *bus)
{
    const uint32_t call_type = cpu->registers[0] & UINT32_C(7);
    const uint32_t current_frame_pointer = cpu->registers[STF_I960_FP_REGISTER];
    uint32_t previous_frame_pointer = 0u;
    uint32_t saved_process_control = 0u;
    uint32_t saved_arithmetic_control = 0u;
    stf_status status = STF_OK;

    if (cpu->local_frame_depth == 0u) {
        return STF_ERROR_UNSUPPORTED;
    }
    if (call_type != 0u && call_type != 7u) {
        return STF_ERROR_UNSUPPORTED;
    }
    if (call_type == 7u) {
        status = stf_i960_bus_read_u32(
            bus,
            current_frame_pointer - UINT32_C(16),
            &saved_process_control
        );
        if (status == STF_OK) {
            status = stf_i960_bus_read_u32(
                bus,
                current_frame_pointer - UINT32_C(12),
                &saved_arithmetic_control
            );
        }
        if (status != STF_OK) {
            return status;
        }
    }

    previous_frame_pointer = cpu->registers[0] & ~UINT32_C(63);
    --cpu->local_frame_depth;
    memcpy(
        cpu->registers,
        cpu->local_frames[cpu->local_frame_depth].registers,
        STF_I960_LOCAL_REGISTER_COUNT * sizeof(uint32_t)
    );
    cpu->registers[STF_I960_FP_REGISTER] = previous_frame_pointer;
    cpu->ip = cpu->registers[2];
    ++cpu->procedure_returns;

    if (call_type == 7u) {
        cpu->process_control = saved_process_control;
        cpu->arithmetic_control = saved_arithmetic_control;
        ++cpu->interrupt_returns;
    }
    return STF_OK;
}

static uint32_t register_value(const stf_i960_cpu *cpu, uint8_t reg)
{
    return reg < STF_I960_REGISTER_COUNT ? cpu->registers[reg] : 0u;
}

static float bits_to_float(uint32_t bits)
{
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static uint32_t float_to_bits(float value)
{
    uint32_t bits = 0u;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static stf_status set_register(
    stf_i960_cpu *cpu,
    const stf_i960_operand *operand,
    uint32_t value
)
{
    if (operand->kind != STF_I960_OPERAND_REGISTER ||
        operand->value.reg >= STF_I960_REGISTER_COUNT) {
        return STF_ERROR_UNSUPPORTED;
    }
    cpu->registers[operand->value.reg] = value;
    return STF_OK;
}

static stf_status operand_value(
    const stf_i960_cpu *cpu,
    const stf_i960_operand *operand,
    uint32_t *value
)
{
    if (cpu == NULL || operand == NULL || value == NULL) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    switch (operand->kind) {
    case STF_I960_OPERAND_REGISTER:
    case STF_I960_OPERAND_FP_REGISTER:
    case STF_I960_OPERAND_SPECIAL_REGISTER:
        *value = register_value(cpu, operand->value.reg);
        return STF_OK;
    case STF_I960_OPERAND_LITERAL:
        *value = (uint32_t)operand->value.literal;
        return STF_OK;
    case STF_I960_OPERAND_ADDRESS:
        *value = operand->value.address;
        return STF_OK;
    default:
        return STF_ERROR_UNSUPPORTED;
    }
}

static stf_status effective_address(
    const stf_i960_cpu *cpu,
    const stf_i960_memory_operand *memory,
    uint32_t *address
)
{
    uint32_t result = 0u;
    if (cpu == NULL || memory == NULL || address == NULL) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    if (memory->absolute || memory->ip_relative) {
        result = memory->resolved_address;
    } else {
        result = (uint32_t)memory->displacement;
    }
    if (memory->has_base) {
        result += register_value(cpu, memory->base);
    }
    if (memory->has_index) {
        result += register_value(cpu, memory->index) << memory->scale;
    }
    *address = result;
    return STF_OK;
}

static stf_status memory_operand_address(
    const stf_i960_cpu *cpu,
    const stf_i960_operand *operand,
    uint32_t *address
)
{
    if (operand == NULL || operand->kind != STF_I960_OPERAND_MEMORY) {
        return STF_ERROR_UNSUPPORTED;
    }
    return effective_address(cpu, &operand->value.memory, address);
}

static void set_compare_result(
    stf_i960_cpu *cpu,
    stf_i960_compare_result result
)
{
    uint32_t condition_bits = 0u;
    cpu->compare_result = result;
    if (result == STF_I960_COMPARE_LESS) {
        condition_bits = 4u;
    } else if (result == STF_I960_COMPARE_EQUAL) {
        condition_bits = 2u;
    } else if (result == STF_I960_COMPARE_GREATER) {
        condition_bits = 1u;
    }
    cpu->arithmetic_control =
        (cpu->arithmetic_control & ~UINT32_C(7)) | condition_bits;
}

static void compare_unsigned(stf_i960_cpu *cpu, uint32_t left, uint32_t right)
{
    if (left < right) {
        set_compare_result(cpu, STF_I960_COMPARE_LESS);
    } else if (left > right) {
        set_compare_result(cpu, STF_I960_COMPARE_GREATER);
    } else {
        set_compare_result(cpu, STF_I960_COMPARE_EQUAL);
    }
}

static void compare_signed(stf_i960_cpu *cpu, uint32_t left, uint32_t right)
{
    const int32_t signed_left = (int32_t)left;
    const int32_t signed_right = (int32_t)right;
    if (signed_left < signed_right) {
        set_compare_result(cpu, STF_I960_COMPARE_LESS);
    } else if (signed_left > signed_right) {
        set_compare_result(cpu, STF_I960_COMPARE_GREATER);
    } else {
        set_compare_result(cpu, STF_I960_COMPARE_EQUAL);
    }
}

static stf_status convert_real_to_integer(
    const stf_i960_cpu *cpu,
    uint32_t bits,
    bool truncate,
    uint32_t *result
)
{
    const float value = bits_to_float(bits);
    double rounded = 0.0;
    unsigned mode = 0u;

    if (cpu == NULL || result == NULL || !isfinite(value)) {
        return STF_ERROR_UNSUPPORTED;
    }
    if (truncate) {
        rounded = trunc((double)value);
    } else {
        mode = (unsigned)((cpu->arithmetic_control >> 30u) & 3u);
        switch (mode) {
        case 0u: {
            const double lower = floor((double)value);
            const double fraction = (double)value - lower;
            if (fraction < 0.5) {
                rounded = lower;
            } else if (fraction > 0.5) {
                rounded = lower + 1.0;
            } else {
                rounded = fmod(fabs(lower), 2.0) == 0.0
                    ? lower : lower + 1.0;
            }
            break;
        }
        case 1u:
            rounded = floor((double)value);
            break;
        case 2u:
            rounded = ceil((double)value);
            break;
        case 3u:
        default:
            rounded = trunc((double)value);
            break;
        }
    }
    if (rounded < (double)INT32_MIN || rounded > (double)INT32_MAX) {
        return STF_ERROR_UNSUPPORTED;
    }
    *result = (uint32_t)(int32_t)rounded;
    return STF_OK;
}

static bool condition_matches(
    const char *mnemonic,
    stf_i960_compare_result result
)
{
    if (strcmp(mnemonic, "be") == 0) {
        return result == STF_I960_COMPARE_EQUAL;
    }
    if (strcmp(mnemonic, "bne") == 0) {
        return result != STF_I960_COMPARE_EQUAL;
    }
    if (strcmp(mnemonic, "bl") == 0) {
        return result == STF_I960_COMPARE_LESS;
    }
    if (strcmp(mnemonic, "ble") == 0) {
        return result == STF_I960_COMPARE_LESS || result == STF_I960_COMPARE_EQUAL;
    }
    if (strcmp(mnemonic, "bg") == 0) {
        return result == STF_I960_COMPARE_GREATER;
    }
    if (strcmp(mnemonic, "bge") == 0) {
        return result == STF_I960_COMPARE_GREATER || result == STF_I960_COMPARE_EQUAL;
    }
    if (strcmp(mnemonic, "bno") == 0) {
        /* i960 bno is the CC==000 case (unordered / false). */
        return result == STF_I960_COMPARE_NONE;
    }
    if (strcmp(mnemonic, "bo") == 0) {
        /* bo uses mask 111, so any non-zero condition code is ordered / true. */
        return result != STF_I960_COMPARE_NONE;
    }
    return false;
}

/* COBR test* (op 0x20-0x27): write 0xffffffff/0 into src1 from compare CC. */
static bool test_condition_matches(
    const char *mnemonic,
    stf_i960_compare_result result
)
{
    if (strcmp(mnemonic, "testno") == 0) {
        return condition_matches("bno", result);
    }
    if (strcmp(mnemonic, "testg") == 0) {
        return condition_matches("bg", result);
    }
    if (strcmp(mnemonic, "teste") == 0) {
        return condition_matches("be", result);
    }
    if (strcmp(mnemonic, "testge") == 0) {
        return condition_matches("bge", result);
    }
    if (strcmp(mnemonic, "testl") == 0) {
        return condition_matches("bl", result);
    }
    if (strcmp(mnemonic, "testne") == 0) {
        return condition_matches("bne", result);
    }
    if (strcmp(mnemonic, "testle") == 0) {
        return condition_matches("ble", result);
    }
    if (strcmp(mnemonic, "testo") == 0) {
        return condition_matches("bo", result);
    }
    return false;
}

static bool direct_compare_condition(
    const char *mnemonic,
    uint32_t left,
    uint32_t right
)
{
    if (strcmp(mnemonic, "bbs") == 0) {
        return (right & (UINT32_C(1) << (left & 31u))) != 0u;
    }
    if (strcmp(mnemonic, "bbc") == 0) {
        return (right & (UINT32_C(1) << (left & 31u))) == 0u;
    }
    const bool is_signed = strncmp(mnemonic, "cmpi", 4u) == 0;
    const int32_t signed_left = (int32_t)left;
    const int32_t signed_right = (int32_t)right;
    const char *suffix = mnemonic + 4;
    if (strcmp(suffix, "bg") == 0) {
        return is_signed ? signed_left > signed_right : left > right;
    }
    if (strcmp(suffix, "be") == 0) {
        return left == right;
    }
    if (strcmp(suffix, "bge") == 0) {
        return is_signed ? signed_left >= signed_right : left >= right;
    }
    if (strcmp(suffix, "bl") == 0) {
        return is_signed ? signed_left < signed_right : left < right;
    }
    if (strcmp(suffix, "bne") == 0) {
        return left != right;
    }
    if (strcmp(suffix, "ble") == 0) {
        return is_signed ? signed_left <= signed_right : left <= right;
    }
    if (strcmp(suffix, "bno") == 0) {
        /* Integer/ordinal compares always produce an ordered non-zero CC. */
        return false;
    }
    if (strcmp(suffix, "bo") == 0) {
        return true;
    }
    return false;
}

static stf_status execute_load(
    stf_i960_cpu *cpu,
    stf_i960_bus *bus,
    const stf_i960_instruction *instruction,
    size_t word_count,
    size_t scalar_size,
    bool sign_extend
)
{
    uint32_t address = 0u;
    uint8_t destination = 0u;
    size_t index = 0u;
    stf_status status = memory_operand_address(cpu, &instruction->operands[0], &address);
    if (status != STF_OK || instruction->operands[1].kind != STF_I960_OPERAND_REGISTER) {
        return STF_ERROR_UNSUPPORTED;
    }
    destination = instruction->operands[1].value.reg;
    if (word_count > 1u) {
        if ((size_t)destination + word_count > STF_I960_REGISTER_COUNT) {
            return STF_ERROR_OUT_OF_BOUNDS;
        }
        for (index = 0u; index < word_count; ++index) {
            status = stf_i960_bus_read_u32(
                bus,
                address + (uint32_t)(index * 4u),
                &cpu->registers[destination + index]
            );
            if (status != STF_OK) {
                return status;
            }
        }
        return STF_OK;
    }
    if (scalar_size == 4u) {
        return stf_i960_bus_read_u32(bus, address, &cpu->registers[destination]);
    }
    {
        uint8_t data[2] = {0u, 0u};
        uint32_t value = 0u;
        status = stf_i960_bus_read(bus, address, data, scalar_size);
        if (status != STF_OK) {
            return status;
        }
        if (scalar_size == 1u) {
            value = sign_extend ? (uint32_t)(int32_t)(int8_t)data[0] : data[0];
        } else {
            const uint16_t raw = (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8u));
            value = sign_extend ? (uint32_t)(int32_t)(int16_t)raw : raw;
        }
        cpu->registers[destination] = value;
    }
    return STF_OK;
}

static stf_status execute_store(
    stf_i960_cpu *cpu,
    stf_i960_bus *bus,
    const stf_i960_instruction *instruction,
    size_t word_count,
    size_t scalar_size
)
{
    uint32_t address = 0u;
    uint32_t source_value = 0u;
    size_t index = 0u;
    stf_status status = memory_operand_address(cpu, &instruction->operands[1], &address);
    if (status != STF_OK) {
        return status;
    }
    if (instruction->operands[0].kind != STF_I960_OPERAND_REGISTER) {
        return STF_ERROR_UNSUPPORTED;
    }
    if (word_count > 1u) {
        const uint8_t source = instruction->operands[0].value.reg;
        if ((size_t)source + word_count > STF_I960_REGISTER_COUNT) {
            return STF_ERROR_OUT_OF_BOUNDS;
        }
        for (index = 0u; index < word_count; ++index) {
            status = stf_i960_bus_write_u32(
                bus,
                address + (uint32_t)(index * 4u),
                cpu->registers[source + index]
            );
            if (status != STF_OK) {
                return status;
            }
        }
        return STF_OK;
    }
    status = operand_value(cpu, &instruction->operands[0], &source_value);
    if (status != STF_OK) {
        return status;
    }
    if (scalar_size == 4u) {
        return stf_i960_bus_write_u32(bus, address, source_value);
    }
    {
        uint8_t data[2];
        data[0] = (uint8_t)source_value;
        data[1] = (uint8_t)(source_value >> 8u);
        return stf_i960_bus_write(bus, address, data, scalar_size);
    }
}

static stf_status execute_iac(
    stf_i960_cpu *cpu,
    const stf_i960_bus *bus,
    uint32_t packet_address
)
{
    uint32_t packet[4];
    size_t index = 0u;
    for (index = 0u; index < 4u; ++index) {
        stf_status status = stf_i960_bus_read_u32(
            bus,
            packet_address + (uint32_t)(index * 4u),
            &packet[index]
        );
        if (status != STF_OK) {
            return status;
        }
    }
    switch (packet[0] >> 24u) {
    case 0x80u:
        return STF_ERROR_UNSUPPORTED;
    case 0x89u:
    case 0x8fu:
    case 0x92u:
        return STF_OK;
    case 0x93u:
        cpu->sat = packet[1];
        cpu->prcb = packet[2];
        cpu->ip = packet[3];
        cpu->reinitialized = true;
        return STF_OK;
    default:
        return STF_ERROR_UNSUPPORTED;
    }
}

static stf_status execute_synmov(
    stf_i960_cpu *cpu,
    stf_i960_bus *bus,
    const stf_i960_instruction *instruction
)
{
    uint32_t destination = 0u;
    uint32_t source = 0u;
    uint32_t value = 0u;
    stf_status status = operand_value(cpu, &instruction->operands[0], &destination);
    if (status != STF_OK) {
        return status;
    }
    status = operand_value(cpu, &instruction->operands[1], &source);
    if (status != STF_OK) {
        return status;
    }
    status = stf_i960_bus_read_u32(bus, source, &value);
    if (status != STF_OK) {
        return status;
    }
    cpu->compare_result = STF_I960_COMPARE_EQUAL;
    if (destination == UINT32_C(0xff000004)) {
        cpu->interrupt_control = value;
        return STF_OK;
    }
    return stf_i960_bus_write_u32(bus, destination, value);
}

static stf_status execute_synmovq(
    stf_i960_cpu *cpu,
    stf_i960_bus *bus,
    const stf_i960_instruction *instruction
)
{
    uint32_t destination = 0u;
    uint32_t source = 0u;
    uint8_t data[16];
    stf_status status = operand_value(cpu, &instruction->operands[0], &destination);
    if (status != STF_OK) {
        return status;
    }
    status = operand_value(cpu, &instruction->operands[1], &source);
    if (status != STF_OK) {
        return status;
    }
    cpu->compare_result = STF_I960_COMPARE_EQUAL;
    if (destination == 0xff000010u) {
        return execute_iac(cpu, bus, source);
    }
    status = stf_i960_bus_read(bus, source, data, sizeof(data));
    if (status != STF_OK) {
        return status;
    }
    return stf_i960_bus_write(bus, destination, data, sizeof(data));
}

static stf_status execute_instruction(
    stf_i960_cpu *cpu,
    stf_i960_bus *bus,
    const stf_i960_instruction *instruction,
    uint32_t next_ip
)
{
    uint32_t first = 0u;
    uint32_t second = 0u;
    uint32_t address = 0u;
    stf_status status = STF_OK;
    const char *mnemonic = instruction->mnemonic;

    if (strcmp(mnemonic, "lda") == 0) {
        status = memory_operand_address(cpu, &instruction->operands[0], &address);
        return status == STF_OK ? set_register(cpu, &instruction->operands[1], address) : status;
    }
    if (strcmp(mnemonic, "ld") == 0) {
        return execute_load(cpu, bus, instruction, 1u, 4u, false);
    }
    if (strcmp(mnemonic, "ldob") == 0) {
        return execute_load(cpu, bus, instruction, 1u, 1u, false);
    }
    if (strcmp(mnemonic, "ldib") == 0) {
        return execute_load(cpu, bus, instruction, 1u, 1u, true);
    }
    if (strcmp(mnemonic, "ldos") == 0) {
        return execute_load(cpu, bus, instruction, 1u, 2u, false);
    }
    if (strcmp(mnemonic, "ldis") == 0) {
        return execute_load(cpu, bus, instruction, 1u, 2u, true);
    }
    if (strcmp(mnemonic, "ldl") == 0) {
        return execute_load(cpu, bus, instruction, 2u, 4u, false);
    }
    if (strcmp(mnemonic, "ldt") == 0) {
        return execute_load(cpu, bus, instruction, 3u, 4u, false);
    }
    if (strcmp(mnemonic, "ldq") == 0) {
        return execute_load(cpu, bus, instruction, 4u, 4u, false);
    }
    if (strcmp(mnemonic, "st") == 0) {
        return execute_store(cpu, bus, instruction, 1u, 4u);
    }
    if (strcmp(mnemonic, "stob") == 0 || strcmp(mnemonic, "stib") == 0) {
        return execute_store(cpu, bus, instruction, 1u, 1u);
    }
    if (strcmp(mnemonic, "stos") == 0 || strcmp(mnemonic, "stis") == 0) {
        return execute_store(cpu, bus, instruction, 1u, 2u);
    }
    if (strcmp(mnemonic, "stl") == 0) {
        return execute_store(cpu, bus, instruction, 2u, 4u);
    }
    if (strcmp(mnemonic, "stt") == 0) {
        return execute_store(cpu, bus, instruction, 3u, 4u);
    }
    if (strcmp(mnemonic, "stq") == 0) {
        return execute_store(cpu, bus, instruction, 4u, 4u);
    }
    if (strcmp(mnemonic, "mov") == 0) {
        status = operand_value(cpu, &instruction->operands[0], &first);
        return status == STF_OK ? set_register(cpu, &instruction->operands[1], first) : status;
    }
    if (strcmp(mnemonic, "movl") == 0 || strcmp(mnemonic, "movt") == 0 ||
        strcmp(mnemonic, "movq") == 0) {
        const size_t count = strcmp(mnemonic, "movl") == 0 ? 2u :
                             (strcmp(mnemonic, "movt") == 0 ? 3u : 4u);
        size_t index = 0u;
        uint8_t source = 0u;
        uint8_t destination = 0u;
        if (instruction->operands[0].kind != STF_I960_OPERAND_REGISTER ||
            instruction->operands[1].kind != STF_I960_OPERAND_REGISTER) {
            return STF_ERROR_UNSUPPORTED;
        }
        source = instruction->operands[0].value.reg;
        destination = instruction->operands[1].value.reg;
        if ((size_t)source + count > STF_I960_REGISTER_COUNT ||
            (size_t)destination + count > STF_I960_REGISTER_COUNT) {
            return STF_ERROR_OUT_OF_BOUNDS;
        }
        for (index = 0u; index < count; ++index) {
            cpu->registers[destination + index] = cpu->registers[source + index];
        }
        return STF_OK;
    }
    if (strcmp(mnemonic, "dmovt") == 0) {
        /* v0344: measured at fa_coli 0x508d4 as dmovt r3, r3
         * (word 0x64181203) inside the 0x502a4 subtree. Double-word
         * register copy like movl; no compare-state effects (mirrors
         * mov/movt/movl/movq). Only the register-to-register form is
         * admitted; anything else stays fail-closed. Trace-fault
         * machinery does not exist in this executor and the measured
         * path observes no trace event. */
        if (instruction->operands[0].kind != STF_I960_OPERAND_REGISTER ||
            instruction->operands[1].kind != STF_I960_OPERAND_REGISTER) {
            return STF_ERROR_UNSUPPORTED;
        }
        {
            size_t dm_index = 0u;
            uint8_t dm_source = instruction->operands[0].value.reg;
            uint8_t dm_destination = instruction->operands[1].value.reg;

            if ((size_t)dm_source + 2u > STF_I960_REGISTER_COUNT ||
                (size_t)dm_destination + 2u > STF_I960_REGISTER_COUNT) {
                return STF_ERROR_OUT_OF_BOUNDS;
            }
            for (dm_index = 0u; dm_index < 2u; ++dm_index) {
                cpu->registers[dm_destination + dm_index] =
                    cpu->registers[dm_source + dm_index];
            }
        }
        return STF_OK;
    }
    if (strcmp(mnemonic, "addo") == 0 || strcmp(mnemonic, "addi") == 0 ||
        strcmp(mnemonic, "subo") == 0 || strcmp(mnemonic, "subi") == 0 ||
        strcmp(mnemonic, "and") == 0 || strcmp(mnemonic, "andnot") == 0 ||
        strcmp(mnemonic, "notand") == 0 || strcmp(mnemonic, "or") == 0 ||
        strcmp(mnemonic, "ornot") == 0 || strcmp(mnemonic, "notor") == 0 ||
        strcmp(mnemonic, "xor") == 0 || strcmp(mnemonic, "xnor") == 0 ||
        strcmp(mnemonic, "nor") == 0 || strcmp(mnemonic, "nand") == 0 ||
        strcmp(mnemonic, "shlo") == 0 ||
        strcmp(mnemonic, "shli") == 0 || strcmp(mnemonic, "shro") == 0 ||
        strcmp(mnemonic, "shri") == 0 || strcmp(mnemonic, "rotate") == 0 ||
        strcmp(mnemonic, "shrdi") == 0) {
        status = operand_value(cpu, &instruction->operands[0], &first);
        if (status != STF_OK) {
            return status;
        }
        status = operand_value(cpu, &instruction->operands[1], &second);
        if (status != STF_OK) {
            return status;
        }
        if (strcmp(mnemonic, "addo") == 0 || strcmp(mnemonic, "addi") == 0) {
            address = second + first;
        } else if (strcmp(mnemonic, "subo") == 0 || strcmp(mnemonic, "subi") == 0) {
            address = second - first;
        } else if (strcmp(mnemonic, "and") == 0) {
            address = second & first;
        } else if (strcmp(mnemonic, "andnot") == 0) {
            address = second & ~first;
        } else if (strcmp(mnemonic, "notand") == 0) {
            address = (~second) & first;
        } else if (strcmp(mnemonic, "or") == 0) {
            address = second | first;
        } else if (strcmp(mnemonic, "ornot") == 0) {
            address = second | ~first;
        } else if (strcmp(mnemonic, "notor") == 0) {
            address = (~second) | first;
        } else if (strcmp(mnemonic, "xor") == 0) {
            address = second ^ first;
        } else if (strcmp(mnemonic, "xnor") == 0) {
            address = ~(second ^ first);
        } else if (strcmp(mnemonic, "nor") == 0) {
            address = (~second) & (~first);
        } else if (strcmp(mnemonic, "nand") == 0) {
            address = (~second) | (~first);
        } else if (strcmp(mnemonic, "shlo") == 0 || strcmp(mnemonic, "shli") == 0) {
            address = first >= 32u ? 0u : second << first;
        } else if (strcmp(mnemonic, "shro") == 0) {
            address = first >= 32u ? 0u : second >> first;
        } else if (strcmp(mnemonic, "shri") == 0) {
            address = first >= 32u
                ? ((int32_t)second < 0 ? UINT32_MAX : 0u)
                : (uint32_t)((int32_t)second >> first);
        } else if (strcmp(mnemonic, "rotate") == 0) {
            const uint32_t count = first & 31u;
            address = count == 0u
                ? second
                : (second << count) | (second >> (32u - count));
        } else {
            /* SHRDI divides a signed integer by 2^count and rounds toward zero,
             * unlike SHRI's conventional arithmetic shift for negative values. */
            if (first >= 32u) {
                address = 0u;
            } else if (first == 0u || (int32_t)second >= 0) {
                address = (uint32_t)((int32_t)second >> first);
            } else {
                const uint32_t bias = (UINT32_C(1) << first) - 1u;
                address = (uint32_t)(((int32_t)second + (int32_t)bias) >> first);
            }
        }
        return set_register(cpu, &instruction->operands[2], address);
    }
    if (strcmp(mnemonic, "clrbit") == 0 || strcmp(mnemonic, "setbit") == 0 ||
        strcmp(mnemonic, "alterbit") == 0 || strcmp(mnemonic, "notbit") == 0) {
        status = operand_value(cpu, &instruction->operands[0], &first);
        if (status != STF_OK) {
            return status;
        }
        status = operand_value(cpu, &instruction->operands[1], &second);
        if (status != STF_OK) {
            return status;
        }
        if (strcmp(mnemonic, "clrbit") == 0) {
            address = second & ~(UINT32_C(1) << (first & 31u));
        } else if (strcmp(mnemonic, "setbit") == 0) {
            address = second | (UINT32_C(1) << (first & 31u));
        } else if (strcmp(mnemonic, "alterbit") == 0) {
            address = (cpu->arithmetic_control & UINT32_C(2)) != 0u
                ? second | (UINT32_C(1) << (first & 31u))
                : second & ~(UINT32_C(1) << (first & 31u));
        } else {
            address = second ^ (UINT32_C(1) << (first & 31u));
        }
        return set_register(cpu, &instruction->operands[2], address);
    }
    if (strcmp(mnemonic, "addc") == 0 || strcmp(mnemonic, "subc") == 0) {
        uint64_t wide = 0u;
        uint32_t result = 0u;
        const uint32_t carry_in = (cpu->arithmetic_control >> 1u) & 1u;
        status = operand_value(cpu, &instruction->operands[0], &first);
        if (status != STF_OK) {
            return status;
        }
        status = operand_value(cpu, &instruction->operands[1], &second);
        if (status != STF_OK) {
            return status;
        }
        if (strcmp(mnemonic, "addc") == 0) {
            wide = (uint64_t)second + (uint64_t)first + carry_in;
            result = (uint32_t)wide;
            cpu->arithmetic_control &= ~UINT32_C(3);
            if ((wide >> 32u) != 0u) {
                cpu->arithmetic_control |= UINT32_C(2);
            }
            if (((result ^ first) & (result ^ second) & UINT32_C(0x80000000)) != 0u) {
                cpu->arithmetic_control |= UINT32_C(1);
            }
        } else {
            wide = (uint64_t)second - ((uint64_t)first + carry_in);
            result = (uint32_t)wide;
            cpu->arithmetic_control &= ~UINT32_C(3);
            if ((wide >> 32u) != 0u) {
                cpu->arithmetic_control |= UINT32_C(2);
            }
            if (((second ^ first) & (second ^ result) & UINT32_C(0x80000000)) != 0u) {
                cpu->arithmetic_control |= UINT32_C(1);
            }
        }
        return set_register(cpu, &instruction->operands[2], result);
    }
    if (strcmp(mnemonic, "addr") == 0 || strcmp(mnemonic, "subr") == 0 ||
        strcmp(mnemonic, "mulr") == 0 || strcmp(mnemonic, "divr") == 0) {
        float left = 0.0f;
        float right = 0.0f;
        float result = 0.0f;
        status = operand_value(cpu, &instruction->operands[0], &first);
        if (status != STF_OK) {
            return status;
        }
        status = operand_value(cpu, &instruction->operands[1], &second);
        if (status != STF_OK) {
            return status;
        }
        left = bits_to_float(first);
        right = bits_to_float(second);
        if (strcmp(mnemonic, "addr") == 0) {
            result = right + left;
        } else if (strcmp(mnemonic, "subr") == 0) {
            result = right - left;
        } else if (strcmp(mnemonic, "mulr") == 0) {
            result = right * left;
        } else {
            result = right / left;
        }
        return set_register(cpu, &instruction->operands[2], float_to_bits(result));
    }
    if (strcmp(mnemonic, "cmpr") == 0 || strcmp(mnemonic, "cmpor") == 0) {
        const float left = bits_to_float((status = operand_value(cpu, &instruction->operands[0], &first)) == STF_OK ? first : 0u);
        float right = 0.0f;
        if (status != STF_OK) {
            return status;
        }
        status = operand_value(cpu, &instruction->operands[1], &second);
        if (status != STF_OK) {
            return status;
        }
        right = bits_to_float(second);
        if (left < right) {
            set_compare_result(cpu, STF_I960_COMPARE_LESS);
        } else if (left > right) {
            set_compare_result(cpu, STF_I960_COMPARE_GREATER);
        } else {
            set_compare_result(cpu, STF_I960_COMPARE_EQUAL);
        }
        return STF_OK;
    }
    if (strcmp(mnemonic, "mulo") == 0 || strcmp(mnemonic, "muli") == 0) {
        status = operand_value(cpu, &instruction->operands[0], &first);
        if (status != STF_OK) {
            return status;
        }
        status = operand_value(cpu, &instruction->operands[1], &second);
        if (status != STF_OK) {
            return status;
        }
        address = first * second;
        if (strcmp(mnemonic, "mulo") == 0) {
            /* v0344: the fa_coli 0x502a4 digit loop (0x508dc) can only
             * terminate via the following bo, which reads the overflow
             * latch — but no in-loop instruction sets it, so the oracle
             * spins forever while hardware exits on the mulo overflow.
             * Sticky-set OVERFLOW when the true unsigned product does
             * not fit 32 bits; otherwise leave the latch alone (smallest
             * blast radius: existing paths with non-overflowing mulos
             * are bit-identical). muli intentionally untouched.
             * Assumption grade: architecture-inferred, pins-validated
             * (see note); re-scope if any pin moves. */
            uint64_t wide = (uint64_t)first * (uint64_t)second;

            if (wide > (uint64_t)UINT32_MAX) {
                cpu->compare_result = STF_I960_COMPARE_OVERFLOW;
            }
        }
        return set_register(cpu, &instruction->operands[2], address);
    }
    if (strcmp(mnemonic, "ediv") == 0) {
        uint32_t divisor = 0u;
        uint32_t dividend_low = 0u;
        uint32_t dividend_high = 0u;
        uint64_t dividend = 0u;
        uint64_t quotient = 0u;
        uint32_t remainder = 0u;
        uint8_t source_register = 0u;
        uint8_t destination_register = 0u;

        status = operand_value(cpu, &instruction->operands[0], &divisor);
        if (status != STF_OK || divisor == 0u ||
            instruction->operands[1].kind != STF_I960_OPERAND_REGISTER ||
            instruction->operands[2].kind != STF_I960_OPERAND_REGISTER) {
            return status == STF_OK ? STF_ERROR_UNSUPPORTED : status;
        }
        source_register = instruction->operands[1].value.reg;
        destination_register = instruction->operands[2].value.reg;
        if (source_register + 1u >= STF_I960_REGISTER_COUNT ||
            destination_register + 1u >= STF_I960_REGISTER_COUNT) {
            return STF_ERROR_UNSUPPORTED;
        }
        dividend_low = cpu->registers[source_register];
        dividend_high = cpu->registers[source_register + 1u];
        dividend = ((uint64_t)dividend_high << 32u) | (uint64_t)dividend_low;
        quotient = dividend / (uint64_t)divisor;
        if (quotient > UINT32_MAX) {
            return STF_ERROR_UNSUPPORTED;
        }
        remainder = (uint32_t)(dividend % (uint64_t)divisor);
        cpu->registers[destination_register] = remainder;
        cpu->registers[destination_register + 1u] = (uint32_t)quotient;
        return STF_OK;
    }
    if (strcmp(mnemonic, "divo") == 0 || strcmp(mnemonic, "divi") == 0 ||
        strcmp(mnemonic, "remo") == 0 || strcmp(mnemonic, "remi") == 0 ||
        strcmp(mnemonic, "modi") == 0) {
        status = operand_value(cpu, &instruction->operands[0], &first);
        if (status != STF_OK) {
            return status;
        }
        status = operand_value(cpu, &instruction->operands[1], &second);
        if (status != STF_OK) {
            return status;
        }
        if (first == 0u) {
            return STF_ERROR_UNSUPPORTED;
        }
        if (strcmp(mnemonic, "divo") == 0) {
            address = second / first;
        } else if (strcmp(mnemonic, "remo") == 0) {
            address = second % first;
        } else {
            const int32_t divisor = (int32_t)first;
            const int32_t dividend = (int32_t)second;
            int32_t result = 0;
            if (divisor == 0) {
                return STF_ERROR_UNSUPPORTED;
            }
            if (dividend == INT32_MIN && divisor == -1) {
                result = strcmp(mnemonic, "divi") == 0 ? INT32_MIN : 0;
            } else if (strcmp(mnemonic, "divi") == 0) {
                result = dividend / divisor;
            } else {
                result = dividend % divisor;
                if (strcmp(mnemonic, "modi") == 0 && result != 0 &&
                    ((result < 0) != (divisor < 0))) {
                    result += divisor;
                }
            }
            address = (uint32_t)result;
        }
        return set_register(cpu, &instruction->operands[2], address);
    }
    if (strcmp(mnemonic, "not") == 0) {
        status = operand_value(cpu, &instruction->operands[0], &first);
        return status == STF_OK ? set_register(cpu, &instruction->operands[1], ~first) : status;
    }
    if (strcmp(mnemonic, "chkbit") == 0) {
        status = operand_value(cpu, &instruction->operands[0], &first);
        if (status != STF_OK) {
            return status;
        }
        status = operand_value(cpu, &instruction->operands[1], &second);
        if (status != STF_OK) {
            return status;
        }
        set_compare_result(
            cpu,
            (second & (UINT32_C(1) << (first & 31u))) != 0u
                ? STF_I960_COMPARE_EQUAL
                : STF_I960_COMPARE_NONE
        );
        return STF_OK;
    }
    if (strcmp(mnemonic, "scanbit") == 0 || strcmp(mnemonic, "spanbit") == 0) {
        int bit = 31;
        uint32_t result = UINT32_C(31);
        bool found = false;
        status = operand_value(cpu, &instruction->operands[0], &first);
        if (status != STF_OK) {
            return status;
        }
        for (bit = 31; bit >= 0; --bit) {
            const bool selected = strcmp(mnemonic, "scanbit") == 0
                ? (first & (UINT32_C(1) << (uint32_t)bit)) != 0u
                : (first & (UINT32_C(1) << (uint32_t)bit)) == 0u;
            if (selected) {
                result = (uint32_t)bit;
                found = true;
                break;
            }
        }
        /* Bno after scanbit/spanbit tests NoBit: taken only on a miss.
         * OVERFLOW keeps bno from firing on a hit; NONE fires on miss.
         * Architectural dest on miss is 31. */
        cpu->compare_result = found
            ? STF_I960_COMPARE_OVERFLOW
            : STF_I960_COMPARE_NONE;
        return set_register(cpu, &instruction->operands[1], result);
    }
    if (strcmp(mnemonic, "cmpo") == 0 || strcmp(mnemonic, "cmpi") == 0) {
        status = operand_value(cpu, &instruction->operands[0], &first);
        if (status != STF_OK) {
            return status;
        }
        status = operand_value(cpu, &instruction->operands[1], &second);
        if (status != STF_OK) {
            return status;
        }
        if (strcmp(mnemonic, "cmpo") == 0) {
            compare_unsigned(cpu, first, second);
        } else {
            compare_signed(cpu, first, second);
        }
        return STF_OK;
    }
    if (strcmp(mnemonic, "concmpo") == 0 ||
        strcmp(mnemonic, "concmpi") == 0) {
        /* Conditional compare implements the i960 two-sided range idiom:
         * once the preceding comparison has produced equality, preserve it;
         * otherwise compare the new operands and replace the condition code. */
        if ((cpu->arithmetic_control & UINT32_C(2)) != 0u) {
            return STF_OK;
        }
        status = operand_value(cpu, &instruction->operands[0], &first);
        if (status != STF_OK) {
            return status;
        }
        status = operand_value(cpu, &instruction->operands[1], &second);
        if (status != STF_OK) {
            return status;
        }
        if (strcmp(mnemonic, "concmpo") == 0) {
            compare_unsigned(cpu, first, second);
        } else {
            compare_signed(cpu, first, second);
        }
        return STF_OK;
    }
    if (strcmp(mnemonic, "cvtri") == 0 ||
        strcmp(mnemonic, "cvtzri") == 0) {
        uint32_t converted = 0u;
        status = operand_value(cpu, &instruction->operands[0], &first);
        if (status != STF_OK) {
            return status;
        }
        status = convert_real_to_integer(
            cpu, first, strcmp(mnemonic, "cvtzri") == 0, &converted
        );
        if (status != STF_OK) {
            return status;
        }
        return set_register(cpu, &instruction->operands[1], converted);
    }
    if (strcmp(mnemonic, "cvtir") == 0) {
        status = operand_value(cpu, &instruction->operands[0], &first);
        if (status != STF_OK) {
            return status;
        }
        return set_register(
            cpu, &instruction->operands[1],
            float_to_bits((float)(int32_t)first)
        );
    }
    if (strcmp(mnemonic, "cvtilr") == 0) {
        uint8_t source = 0u;
        uint64_t raw = 0u;
        int64_t value = 0;
        if (instruction->operands[0].kind != STF_I960_OPERAND_REGISTER ||
            instruction->operands[0].value.reg + 1u >= STF_I960_REGISTER_COUNT) {
            return STF_ERROR_UNSUPPORTED;
        }
        source = instruction->operands[0].value.reg;
        raw = (uint64_t)cpu->registers[source] |
            ((uint64_t)cpu->registers[source + 1u] << 32u);
        memcpy(&value, &raw, sizeof(value));
        return set_register(
            cpu, &instruction->operands[1],
            float_to_bits((float)value)
        );
    }
    if (strcmp(mnemonic, "cmpdeco") == 0 || strcmp(mnemonic, "cmpdeci") == 0 ||
        strcmp(mnemonic, "cmpinco") == 0 || strcmp(mnemonic, "cmpinci") == 0) {
        status = operand_value(cpu, &instruction->operands[0], &first);
        if (status != STF_OK) {
            return status;
        }
        status = operand_value(cpu, &instruction->operands[1], &second);
        if (status != STF_OK) {
            return status;
        }
        if (strcmp(mnemonic, "cmpdeci") == 0 || strcmp(mnemonic, "cmpinci") == 0) {
            compare_signed(cpu, first, second);
        } else {
            compare_unsigned(cpu, first, second);
        }
        address = (strncmp(mnemonic, "cmpdec", 6u) == 0) ? second - 1u : second + 1u;
        return set_register(cpu, &instruction->operands[2], address);
    }
    if (strcmp(mnemonic, "b") == 0) {
        cpu->ip = instruction->target;
        return STF_OK;
    }
    if (instruction->format == STF_I960_FORMAT_COBR &&
        instruction->operand_count == 1u &&
        strncmp(mnemonic, "test", 4u) == 0) {
        const uint32_t value =
            test_condition_matches(mnemonic, cpu->compare_result)
                ? UINT32_MAX
                : 0u;
        return set_register(cpu, &instruction->operands[0], value);
    }
    if (instruction->flow == STF_I960_FLOW_BRANCH && instruction->conditional &&
        instruction->format == STF_I960_FORMAT_CTRL) {
        if (condition_matches(mnemonic, cpu->compare_result)) {
            cpu->ip = instruction->target;
        }
        return STF_OK;
    }
    if (instruction->flow == STF_I960_FLOW_BRANCH && instruction->conditional &&
        instruction->format == STF_I960_FORMAT_COBR && instruction->operand_count >= 2u) {
        status = operand_value(cpu, &instruction->operands[0], &first);
        if (status != STF_OK) {
            return status;
        }
        status = operand_value(cpu, &instruction->operands[1], &second);
        if (status != STF_OK) {
            return status;
        }
        /* Hardware COBR cmpo/cmpi set condition codes; a following be/bne/bl
         * observes this compare. Recovered-C exits must pin the same CC.
         * Keep compare_result and AC low condition bits in lockstep. */
        if (strncmp(mnemonic, "cmpo", 4u) == 0 ||
            strncmp(mnemonic, "cmpi", 4u) == 0) {
            const int is_signed = strncmp(mnemonic, "cmpi", 4u) == 0;
            uint32_t condition_bits = 0u;
            if (first == second) {
                cpu->compare_result = STF_I960_COMPARE_EQUAL;
                condition_bits = 2u;
            } else if (is_signed
                           ? ((int32_t)first < (int32_t)second)
                           : (first < second)) {
                cpu->compare_result = STF_I960_COMPARE_LESS;
                condition_bits = 4u;
            } else {
                cpu->compare_result = STF_I960_COMPARE_GREATER;
                condition_bits = 1u;
            }
            cpu->arithmetic_control =
                (cpu->arithmetic_control & ~UINT32_C(7)) | condition_bits;
        }
        if (direct_compare_condition(mnemonic, first, second)) {
            cpu->ip = instruction->target;
        }
        return STF_OK;
    }
    if (strcmp(mnemonic, "bal") == 0) {
        cpu->registers[STF_I960_G14_REGISTER] = next_ip;
        cpu->ip = instruction->target;
        return STF_OK;
    }
    if (strcmp(mnemonic, "call") == 0) {
        return procedure_call(cpu, instruction->target, next_ip);
    }
    if (strcmp(mnemonic, "bx") == 0 || strcmp(mnemonic, "balx") == 0 ||
        strcmp(mnemonic, "callx") == 0) {
        status = memory_operand_address(cpu, &instruction->operands[0], &address);
        if (status != STF_OK) {
            return status;
        }
        if (strcmp(mnemonic, "callx") == 0) {
            return procedure_call(cpu, address, next_ip);
        }
        if (strcmp(mnemonic, "balx") == 0) {
            if (instruction->operand_count < 2u) {
                return STF_ERROR_UNSUPPORTED;
            }
            status = set_register(cpu, &instruction->operands[1], next_ip);
            if (status != STF_OK) {
                return status;
            }
        }
        cpu->ip = address;
        return STF_OK;
    }
    if (strcmp(mnemonic, "modpc") == 0 || strcmp(mnemonic, "modac") == 0) {
        uint32_t old_value = 0u;
        uint32_t new_value = 0u;
        status = operand_value(cpu, &instruction->operands[0], &first);
        if (status != STF_OK) {
            return status;
        }
        status = operand_value(cpu, &instruction->operands[1], &second);
        if (status != STF_OK) {
            return status;
        }
        old_value = strcmp(mnemonic, "modpc") == 0
            ? cpu->process_control
            : cpu->arithmetic_control;
        new_value = (old_value & ~first) | (second & first);
        status = set_register(cpu, &instruction->operands[2], old_value);
        if (status != STF_OK) {
            return status;
        }
        if (strcmp(mnemonic, "modpc") == 0) {
            cpu->process_control = new_value;
        } else {
            cpu->arithmetic_control = new_value;
        }
        return STF_OK;
    }
    if (strcmp(mnemonic, "synmov") == 0) {
        return execute_synmov(cpu, bus, instruction);
    }
    if (strcmp(mnemonic, "synmovq") == 0) {
        return execute_synmovq(cpu, bus, instruction);
    }
    if (strcmp(mnemonic, "ret") == 0) {
        return procedure_return(cpu, bus);
    }
    return STF_ERROR_UNSUPPORTED;
}

stf_status stf_i960_cpu_enter_procedure(
    stf_i960_cpu *cpu,
    uint32_t target,
    uint32_t return_address
)
{
    if (cpu == NULL) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    return procedure_call(cpu, target, return_address);
}

stf_status stf_i960_cpu_return_procedure(
    stf_i960_cpu *cpu,
    stf_i960_bus *bus
)
{
    if (cpu == NULL || bus == NULL) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    return procedure_return(cpu, bus);
}

stf_status stf_i960_cpu_enter_interrupt(
    stf_i960_cpu *cpu,
    stf_i960_bus *bus,
    uint32_t vector,
    uint32_t level
)
{
    uint32_t interrupt_table = 0u;
    uint32_t interrupt_stack = 0u;
    uint32_t handler = 0u;
    uint32_t stack_pointer = 0u;
    stf_status status = STF_OK;

    if (cpu == NULL || bus == NULL || vector < 8u || level > 31u) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    status = stf_i960_bus_read_u32(bus, cpu->prcb + 20u, &interrupt_table);
    if (status == STF_OK) {
        status = stf_i960_bus_read_u32(bus, cpu->prcb + 24u, &interrupt_stack);
    }
    if (status == STF_OK) {
        status = stf_i960_bus_read_u32(
            bus,
            interrupt_table + 36u + (vector - 8u) * 4u,
            &handler
        );
    }
    if (status != STF_OK || handler == 0u) {
        return status == STF_OK ? STF_ERROR_UNSUPPORTED : status;
    }

    stack_pointer = (cpu->process_control & UINT32_C(0x2000)) != 0u
        ? cpu->registers[1]
        : interrupt_stack;
    stack_pointer = align_stack_frame(stack_pointer) + UINT32_C(64);

    status = procedure_call_typed(cpu, handler, cpu->ip, 7u, stack_pointer);
    if (status != STF_OK) {
        return status;
    }
    status = stf_i960_bus_write_u32(
        bus,
        cpu->registers[STF_I960_FP_REGISTER] - UINT32_C(16),
        cpu->process_control
    );
    if (status == STF_OK) {
        status = stf_i960_bus_write_u32(
            bus,
            cpu->registers[STF_I960_FP_REGISTER] - UINT32_C(12),
            cpu->arithmetic_control
        );
    }
    if (status == STF_OK) {
        status = stf_i960_bus_write_u32(
            bus,
            cpu->registers[STF_I960_FP_REGISTER] - UINT32_C(8),
            vector - 8u
        );
    }
    if (status != STF_OK) {
        return status;
    }

    cpu->process_control &= ~UINT32_C(0x00001f00);
    cpu->process_control |= (level << 16u) | UINT32_C(0x2002);
    return STF_OK;
}

void stf_i960_cpu_reset(
    stf_i960_cpu *cpu,
    uint32_t sat,
    uint32_t prcb,
    uint32_t start_ip
)
{
    if (cpu != NULL) {
        memset(cpu, 0, sizeof(*cpu));
        cpu->sat = sat;
        cpu->prcb = prcb;
        cpu->ip = start_ip;
        cpu->compare_result = STF_I960_COMPARE_NONE;
    }
}

stf_status stf_i960_cpu_reset_from_bus(
    stf_i960_cpu *cpu,
    stf_i960_bus *bus,
    uint32_t sat,
    uint32_t prcb,
    uint32_t start_ip
)
{
    uint32_t interrupt_stack = 0u;
    stf_status status = STF_OK;
    if (cpu == NULL || bus == NULL) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    stf_i960_cpu_reset(cpu, sat, prcb, start_ip);
    status = stf_i960_bus_read_u32(bus, prcb + UINT32_C(24), &interrupt_stack);
    if (status != STF_OK) {
        return status;
    }
    cpu->registers[STF_I960_FP_REGISTER] = interrupt_stack;
    cpu->registers[1] = interrupt_stack + UINT32_C(64);
    return STF_OK;
}

stf_status stf_i960_step_legacy(
    stf_i960_cpu *cpu,
    stf_i960_bus *bus,
    stf_i960_trace_event *event
)
{
    stf_i960_instruction instruction;
    const uint32_t ip_before = cpu != NULL ? cpu->ip : 0u;
    uint32_t next_ip = 0u;
    stf_status status = STF_OK;
    if (cpu == NULL || bus == NULL || bus->program_image == NULL) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    status = stf_i960_decode(
        bus->program_image,
        bus->program_size,
        cpu->ip,
        &instruction
    );
    if (status != STF_OK || !instruction.valid) {
        return status == STF_OK ? STF_ERROR_UNSUPPORTED : status;
    }
    /* Literal-source multi-register moves (movt 0, r8 at fa_coli 0x236b8):
     * src1 is an inline literal; the destination and following registers
     * receive that literal then zeros. */
    if ((strcmp(instruction.mnemonic, "movl") == 0 ||
         strcmp(instruction.mnemonic, "movt") == 0 ||
         strcmp(instruction.mnemonic, "movq") == 0) &&
        instruction.operand_count == 2u &&
        instruction.operands[0].kind == STF_I960_OPERAND_LITERAL &&
        instruction.operands[1].kind == STF_I960_OPERAND_REGISTER) {
        const size_t count = strcmp(instruction.mnemonic, "movl") == 0 ? 2u :
                             (strcmp(instruction.mnemonic, "movt") == 0 ? 3u : 4u);
        const size_t alignment = count == 2u ? 2u : 4u;
        const uint8_t destination = instruction.operands[1].value.reg;
        size_t index = 0u;
        if ((size_t)destination + count > STF_I960_REGISTER_COUNT ||
            ((size_t)destination % alignment) != 0u) {
            return STF_ERROR_UNSUPPORTED;
        }
        cpu->registers[destination] =
            (uint32_t)instruction.operands[0].value.literal;
        for (index = 1u; index < count; ++index) {
            cpu->registers[destination + index] = 0u;
        }
        cpu->ip = ip_before + instruction.size;
        ++cpu->executed_instructions;
        if (event != NULL) {
            memset(event, 0, sizeof(*event));
            event->step = cpu->executed_instructions;
            event->ip_before = ip_before;
            event->ip_after = cpu->ip;
            event->instruction = instruction;
        }
        return STF_OK;
    }
    next_ip = cpu->ip + instruction.size;
    cpu->ip = next_ip;
    status = execute_instruction(cpu, bus, &instruction, next_ip);
    if (status != STF_OK) {
        cpu->ip = ip_before;
        return status;
    }
    ++cpu->executed_instructions;
    if (event != NULL) {
        memset(event, 0, sizeof(*event));
        event->step = cpu->executed_instructions;
        event->ip_before = ip_before;
        event->ip_after = cpu->ip;
        event->instruction = instruction;
    }
    return STF_OK;
}

stf_status stf_i960_run(
    stf_i960_cpu *cpu,
    stf_i960_bus *bus,
    const stf_i960_run_options *options,
    stf_i960_run_result *result
)
{
    stf_i960_run_options local_options;
    stf_i960_run_result local_result;
    uint64_t start_count = 0u;
    if (cpu == NULL || bus == NULL) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    memset(&local_options, 0, sizeof(local_options));
    local_options.max_steps = 2000000u;
    local_options.stop_on_self_branch = true;
    if (options != NULL) {
        local_options = *options;
        if (local_options.max_steps == 0u) {
            local_options.max_steps = 2000000u;
        }
    }
    memset(&local_result, 0, sizeof(local_result));
    local_result.status = STF_OK;
    start_count = cpu->executed_instructions;

    while (cpu->executed_instructions - start_count < local_options.max_steps) {
        stf_i960_trace_event event;
        stf_status status = STF_OK;
        if (local_options.stop_address != 0u && cpu->ip == local_options.stop_address) {
            local_result.halt_reason = STF_I960_HALT_STOP_ADDRESS;
            break;
        }
        status = stf_i960_step(cpu, bus, &event);
        if (status != STF_OK) {
            local_result.status = status;
            local_result.halt_address = cpu->ip;
            local_result.halt_reason = status == STF_ERROR_OUT_OF_BOUNDS
                ? STF_I960_HALT_MEMORY_FAULT
                : STF_I960_HALT_UNSUPPORTED_INSTRUCTION;
            break;
        }
        if (local_options.trace_callback != NULL) {
            local_options.trace_callback(&event, cpu, local_options.trace_user_data);
        }
        if (local_options.stop_on_self_branch && event.ip_after == event.ip_before) {
            local_result.halt_reason = STF_I960_HALT_SELF_BRANCH;
            break;
        }
    }
    if (local_result.halt_reason == STF_I960_HALT_NONE) {
        local_result.halt_reason = STF_I960_HALT_MAX_STEPS;
    }
    local_result.halt_address = cpu->ip;
    local_result.executed_instructions = cpu->executed_instructions - start_count;
    if (result != NULL) {
        *result = local_result;
    }
    return local_result.status;
}

const char *stf_i960_halt_reason_name(stf_i960_halt_reason reason)
{
    switch (reason) {
    case STF_I960_HALT_NONE:
        return "none";
    case STF_I960_HALT_STOP_ADDRESS:
        return "stop address";
    case STF_I960_HALT_MAX_STEPS:
        return "maximum steps";
    case STF_I960_HALT_SELF_BRANCH:
        return "self branch";
    case STF_I960_HALT_INVALID_INSTRUCTION:
        return "invalid instruction";
    case STF_I960_HALT_UNSUPPORTED_INSTRUCTION:
        return "unsupported instruction";
    case STF_I960_HALT_MEMORY_FAULT:
        return "memory fault";
    default:
        return "unknown";
    }
}
