#ifndef STF_I960_INSTRUCTION_H
#define STF_I960_INSTRUCTION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum stf_i960_format {
    STF_I960_FORMAT_INVALID = 0,
    STF_I960_FORMAT_CTRL,
    STF_I960_FORMAT_COBR,
    STF_I960_FORMAT_REG,
    STF_I960_FORMAT_MEM
} stf_i960_format;

typedef enum stf_i960_operand_kind {
    STF_I960_OPERAND_NONE = 0,
    STF_I960_OPERAND_REGISTER,
    STF_I960_OPERAND_FP_REGISTER,
    STF_I960_OPERAND_SPECIAL_REGISTER,
    STF_I960_OPERAND_LITERAL,
    STF_I960_OPERAND_ADDRESS,
    STF_I960_OPERAND_MEMORY
} stf_i960_operand_kind;

typedef enum stf_i960_flow {
    STF_I960_FLOW_NONE = 0,
    STF_I960_FLOW_BRANCH,
    STF_I960_FLOW_CALL,
    STF_I960_FLOW_RETURN,
    STF_I960_FLOW_FAULT
} stf_i960_flow;

typedef struct stf_i960_memory_operand {
    bool has_base;
    bool has_index;
    bool ip_relative;
    bool absolute;
    uint8_t base;
    uint8_t index;
    uint8_t scale;
    int32_t displacement;
    uint32_t resolved_address;
} stf_i960_memory_operand;

typedef struct stf_i960_operand {
    stf_i960_operand_kind kind;
    bool is_destination;
    union {
        uint8_t reg;
        int32_t literal;
        uint32_t address;
        stf_i960_memory_operand memory;
    } value;
} stf_i960_operand;

typedef struct stf_i960_instruction {
    uint32_t address;
    uint32_t words[2];
    uint8_t size;
    uint16_t opcode;
    stf_i960_format format;
    stf_i960_flow flow;
    const char *mnemonic;
    stf_i960_operand operands[3];
    uint8_t operand_count;
    bool valid;
    bool conditional;
    bool indirect;
    bool has_fallthrough;
    bool has_target;
    uint32_t target;
} stf_i960_instruction;

const char *stf_i960_register_name(uint8_t reg);
const char *stf_i960_fp_register_name(uint8_t reg);
const char *stf_i960_format_name(stf_i960_format format);
const char *stf_i960_flow_name(stf_i960_flow flow);

#endif
