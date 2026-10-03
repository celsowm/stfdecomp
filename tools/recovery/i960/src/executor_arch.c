/*
 * Derived from celsowm/vf2-decomp i960 recovery code.
 * Copyright (c) 2026, vf2-decomp contributors.
 * BSD-3-Clause terms are retained in LICENSE.vf2-decomp.
 * Hardware access was refactored behind stf_i960_bus for Model 2B recovery.
 */
#include "stf/recovery/i960/executor.h"

#include <string.h>

#include "stf/recovery/i960/decoder.h"

stf_status stf_i960_step_legacy(
    stf_i960_cpu *cpu,
    stf_i960_bus *bus,
    stf_i960_trace_event *event
);

static stf_status arch_operand_value(
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
        if (operand->value.reg >= STF_I960_REGISTER_COUNT) {
            return STF_ERROR_OUT_OF_BOUNDS;
        }
        *value = cpu->registers[operand->value.reg];
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

static void arch_set_compare(
    stf_i960_cpu *cpu,
    stf_i960_compare_result result
)
{
    uint32_t bits = 0u;

    if (result == STF_I960_COMPARE_LESS) {
        bits = UINT32_C(4);
    } else if (result == STF_I960_COMPARE_EQUAL) {
        bits = UINT32_C(2);
    } else if (result == STF_I960_COMPARE_GREATER) {
        bits = UINT32_C(1);
    }
    cpu->compare_result = result;
    cpu->arithmetic_control =
        (cpu->arithmetic_control & ~UINT32_C(7)) | bits;
}

static stf_i960_compare_result arch_compare(
    uint32_t left,
    uint32_t right,
    int is_signed
)
{
    if (is_signed != 0) {
        const int32_t signed_left = (int32_t)left;
        const int32_t signed_right = (int32_t)right;
        if (signed_left < signed_right) {
            return STF_I960_COMPARE_LESS;
        }
        if (signed_left > signed_right) {
            return STF_I960_COMPARE_GREATER;
        }
        return STF_I960_COMPARE_EQUAL;
    }
    if (left < right) {
        return STF_I960_COMPARE_LESS;
    }
    if (left > right) {
        return STF_I960_COMPARE_GREATER;
    }
    return STF_I960_COMPARE_EQUAL;
}

static int arch_compare_branch_matches(
    const char *mnemonic,
    stf_i960_compare_result result
)
{
    const char *suffix = mnemonic + 4;

    if (strcmp(suffix, "bg") == 0) {
        return result == STF_I960_COMPARE_GREATER;
    }
    if (strcmp(suffix, "be") == 0) {
        return result == STF_I960_COMPARE_EQUAL;
    }
    if (strcmp(suffix, "bge") == 0) {
        return result == STF_I960_COMPARE_GREATER ||
               result == STF_I960_COMPARE_EQUAL;
    }
    if (strcmp(suffix, "bl") == 0) {
        return result == STF_I960_COMPARE_LESS;
    }
    if (strcmp(suffix, "bne") == 0) {
        return result != STF_I960_COMPARE_EQUAL;
    }
    if (strcmp(suffix, "ble") == 0) {
        return result == STF_I960_COMPARE_LESS ||
               result == STF_I960_COMPARE_EQUAL;
    }
    if (strcmp(suffix, "bo") == 0) {
        return 1;
    }
    if (strcmp(suffix, "bno") == 0) {
        return 0;
    }
    return 0;
}

static void arch_set_ip(
    stf_i960_cpu *cpu,
    stf_i960_trace_event *event,
    const stf_i960_instruction *instruction,
    uint32_t ip_before,
    int branch
)
{
    cpu->ip = branch != 0
        ? instruction->target
        : ip_before + (uint32_t)instruction->size;
    if (event != NULL) {
        event->ip_after = cpu->ip;
    }
}

static stf_status arch_fix_direct_compare(
    stf_i960_cpu *cpu,
    stf_i960_trace_event *event,
    const stf_i960_instruction *instruction,
    uint32_t ip_before,
    uint32_t first,
    uint32_t second
)
{
    const char *mnemonic = instruction->mnemonic;

    if (strcmp(mnemonic, "bbs") == 0 || strcmp(mnemonic, "bbc") == 0) {
        const int set =
            (second & (UINT32_C(1) << (first & UINT32_C(31)))) != 0u;
        arch_set_compare(
            cpu, set != 0 ? STF_I960_COMPARE_EQUAL : STF_I960_COMPARE_NONE
        );
        arch_set_ip(
            cpu, event, instruction, ip_before,
            strcmp(mnemonic, "bbs") == 0 ? set : !set
        );
        return STF_OK;
    }
    if (strncmp(mnemonic, "cmpob", 5u) == 0 ||
        strncmp(mnemonic, "cmpib", 5u) == 0) {
        const int is_signed = strncmp(mnemonic, "cmpib", 5u) == 0;
        const stf_i960_compare_result result =
            arch_compare(first, second, is_signed);
        arch_set_compare(cpu, result);
        arch_set_ip(
            cpu, event, instruction, ip_before,
            arch_compare_branch_matches(mnemonic, result)
        );
    }
    return STF_OK;
}

static stf_status arch_execute_literal_multi_move(
    stf_i960_cpu *cpu,
    stf_i960_trace_event *event,
    const stf_i960_instruction *instruction,
    uint32_t ip_before
)
{
    const char *mnemonic = instruction->mnemonic;
    const size_t count = strcmp(mnemonic, "movl") == 0 ? 2u :
                         (strcmp(mnemonic, "movt") == 0 ? 3u : 4u);
    const size_t alignment = count == 2u ? 2u : 4u;
    uint8_t destination = 0u;
    size_t index = 0u;

    if (instruction->operands[0].kind != STF_I960_OPERAND_LITERAL ||
        instruction->operands[1].kind != STF_I960_OPERAND_REGISTER) {
        return STF_ERROR_UNSUPPORTED;
    }
    destination = instruction->operands[1].value.reg;
    if ((size_t)destination + count > STF_I960_REGISTER_COUNT) {
        return STF_ERROR_OUT_OF_BOUNDS;
    }
    if (((size_t)destination % alignment) != 0u) {
        return STF_ERROR_UNSUPPORTED;
    }

    cpu->registers[destination] =
        (uint32_t)instruction->operands[0].value.literal;
    for (index = 1u; index < count; ++index) {
        cpu->registers[destination + index] = 0u;
    }
    cpu->ip = ip_before + (uint32_t)instruction->size;
    ++cpu->executed_instructions;
    if (event != NULL) {
        memset(event, 0, sizeof(*event));
        event->step = cpu->executed_instructions;
        event->ip_before = ip_before;
        event->ip_after = cpu->ip;
        event->instruction = *instruction;
    }
    return STF_OK;
}

stf_status stf_i960_step(
    stf_i960_cpu *cpu,
    stf_i960_bus *bus,
    stf_i960_trace_event *event
)
{
    stf_i960_instruction instruction;
    const uint32_t ip_before = cpu != NULL ? cpu->ip : 0u;
    uint32_t first = 0u;
    uint32_t second = 0u;
    int direct_compare = 0;
    int literal_multi_move = 0;
    stf_status decode_status = STF_OK;
    stf_status status = STF_OK;

    if (cpu == NULL || bus == NULL) {
        return STF_ERROR_INVALID_ARGUMENT;
    }
    /*
     * Memory events emitted while executing this instruction must carry the
     * same step id as the instruction event. This keeps JSONL traces directly
     * consumable by trace_fields.py even though memory events are emitted
     * before the post-step callback.
     */
    stf_i960_bus_set_trace_step(bus, cpu->executed_instructions + UINT64_C(1));
    memset(&instruction, 0, sizeof(instruction));
    decode_status = stf_i960_decode(
        bus->program_image, bus->program_size, ip_before, &instruction
    );
    if (decode_status == STF_OK) {
        const char *mnemonic = instruction.mnemonic;
        direct_compare =
            strcmp(mnemonic, "bbs") == 0 || strcmp(mnemonic, "bbc") == 0 ||
            strncmp(mnemonic, "cmpob", 5u) == 0 ||
            strncmp(mnemonic, "cmpib", 5u) == 0;
        literal_multi_move =
            (strcmp(mnemonic, "movl") == 0 ||
             strcmp(mnemonic, "movt") == 0 ||
             strcmp(mnemonic, "movq") == 0) &&
            instruction.operands[0].kind == STF_I960_OPERAND_LITERAL;
        if (direct_compare != 0) {
            decode_status = arch_operand_value(
                cpu, &instruction.operands[0], &first
            );
            if (decode_status == STF_OK) {
                decode_status = arch_operand_value(
                    cpu, &instruction.operands[1], &second
                );
            }
        }
    }

    if (decode_status != STF_OK) {
        return decode_status;
    }
    if (literal_multi_move != 0) {
        return arch_execute_literal_multi_move(
            cpu, event, &instruction, ip_before
        );
    }

    status = stf_i960_step_legacy(cpu, bus, event);
    if (status != STF_OK) {
        return status;
    }
    if (direct_compare != 0) {
        return arch_fix_direct_compare(
            cpu, event, &instruction, ip_before, first, second
        );
    }
    if ((strcmp(instruction.mnemonic, "bo") == 0 ||
         strcmp(instruction.mnemonic, "bno") == 0) &&
        cpu->compare_result != STF_I960_COMPARE_OVERFLOW &&
        cpu->compare_result != STF_I960_COMPARE_NONE) {
        /* bo/bno after an integer compare (cc LESS/EQUAL/GREATER, AC
         * low bits in lockstep) test orderedness via AC: integer
         * results are always ordered, so bo is taken and bno is not.
         * After scanbit/spanbit (cc OVERFLOW on hit, NONE on miss; AC
         * untouched by the producer) the legacy decision above is
         * already exact (bno taken only on a miss). Re-deciding from
         * stale AC mis-executes the scan loop (measured: live
         * fa_coli 0x22404 first-contact shape spins with AC&7 != 0),
         * so the scanbit domain stays with legacy. */
        const int ordered = (cpu->arithmetic_control & UINT32_C(7)) != 0u;
        arch_set_ip(
            cpu, event, &instruction, ip_before,
            strcmp(instruction.mnemonic, "bo") == 0 ? ordered : !ordered
        );
    }
    return STF_OK;
}
