/*
 * Derived from celsowm/vf2-decomp i960 recovery code.
 * Copyright (c) 2026, vf2-decomp contributors.
 * BSD-3-Clause terms are retained in LICENSE.vf2-decomp in this directory.
 */
#include "stf/recovery/i960/decoder.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static stf_status append_text(
    char *output,
    size_t output_size,
    size_t *position,
    const char *format,
    ...
)
{
    va_list arguments;
    int written = 0;

    if (*position >= output_size) {
        return STF_ERROR_OUT_OF_BOUNDS;
    }

    va_start(arguments, format);
    written = vsnprintf(
        output + *position,
        output_size - *position,
        format,
        arguments
    );
    va_end(arguments);

    if (written < 0 || (size_t)written >= output_size - *position) {
        return STF_ERROR_OUT_OF_BOUNDS;
    }

    *position += (size_t)written;
    return STF_OK;
}

static stf_status format_memory(
    const stf_i960_memory_operand *memory,
    char *output,
    size_t output_size,
    size_t *position
)
{
    stf_status status = STF_OK;

    if (memory->ip_relative || memory->absolute) {
        status = append_text(
            output,
            output_size,
            position,
            "0x%08x",
            (unsigned)memory->resolved_address
        );
    } else if (memory->displacement != 0) {
        status = append_text(
            output,
            output_size,
            position,
            "0x%08x",
            (unsigned)(uint32_t)memory->displacement
        );
    }
    if (status != STF_OK) {
        return status;
    }

    if (memory->has_base) {
        status = append_text(
            output,
            output_size,
            position,
            "(%s)",
            stf_i960_register_name(memory->base)
        );
        if (status != STF_OK) {
            return status;
        }
    }

    if (memory->has_index) {
        if (memory->scale == 0u) {
            status = append_text(
                output,
                output_size,
                position,
                "[%s]",
                stf_i960_register_name(memory->index)
            );
        } else {
            status = append_text(
                output,
                output_size,
                position,
                "[%s*%u]",
                stf_i960_register_name(memory->index),
                1u << memory->scale
            );
        }
    }

    return status;
}

static stf_status format_operand(
    const stf_i960_operand *operand,
    char *output,
    size_t output_size,
    size_t *position
)
{
    switch (operand->kind) {
    case STF_I960_OPERAND_REGISTER:
        return append_text(
            output,
            output_size,
            position,
            "%s",
            stf_i960_register_name(operand->value.reg)
        );
    case STF_I960_OPERAND_FP_REGISTER:
        return append_text(
            output,
            output_size,
            position,
            "%s",
            stf_i960_fp_register_name(operand->value.reg)
        );
    case STF_I960_OPERAND_SPECIAL_REGISTER:
        return append_text(
            output,
            output_size,
            position,
            "sf%u",
            (unsigned)operand->value.reg
        );
    case STF_I960_OPERAND_LITERAL:
        return append_text(
            output,
            output_size,
            position,
            "%d",
            operand->value.literal
        );
    case STF_I960_OPERAND_ADDRESS:
        return append_text(
            output,
            output_size,
            position,
            "0x%08x",
            (unsigned)operand->value.address
        );
    case STF_I960_OPERAND_MEMORY:
        return format_memory(
            &operand->value.memory,
            output,
            output_size,
            position
        );
    case STF_I960_OPERAND_NONE:
    default:
        return STF_ERROR_UNSUPPORTED;
    }
}

stf_status stf_i960_format_instruction(
    const stf_i960_instruction *instruction,
    char *output,
    size_t output_size
)
{
    size_t position = 0u;
    size_t index = 0u;
    stf_status status = STF_OK;

    if (instruction == NULL || output == NULL || output_size == 0u) {
        return STF_ERROR_INVALID_ARGUMENT;
    }

    output[0] = '\0';
    if (!instruction->valid) {
        return append_text(
            output,
            output_size,
            &position,
            ".word 0x%08x",
            (unsigned)instruction->words[0]
        );
    }

    status = append_text(
        output,
        output_size,
        &position,
        "%-9s",
        instruction->mnemonic
    );
    if (status != STF_OK) {
        return status;
    }

    for (index = 0u; index < instruction->operand_count; ++index) {
        if (index != 0u) {
            status = append_text(output, output_size, &position, ", ");
            if (status != STF_OK) {
                return status;
            }
        }
        status = format_operand(
            &instruction->operands[index],
            output,
            output_size,
            &position
        );
        if (status != STF_OK) {
            return status;
        }
    }

    return STF_OK;
}
