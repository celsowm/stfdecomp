#ifndef STF_RECOVERY_I960_EXECUTOR_H
#define STF_RECOVERY_I960_EXECUTOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "stf/recovery/i960/bus.h"
#include "stf/recovery/i960/instruction.h"
#include "stf/recovery/status.h"

enum {
    STF_I960_REGISTER_COUNT = 32u,
    STF_I960_G0_REGISTER = 16u,
    STF_I960_G14_REGISTER = 30u,
    STF_I960_FP_REGISTER = 31u,
    STF_I960_LOCAL_REGISTER_COUNT = 16u,
    STF_I960_MAX_LOCAL_FRAMES = 128u
};

typedef enum stf_i960_compare_result {
    STF_I960_COMPARE_NONE = 0,
    STF_I960_COMPARE_LESS,
    STF_I960_COMPARE_EQUAL,
    STF_I960_COMPARE_GREATER,
    STF_I960_COMPARE_OVERFLOW
} stf_i960_compare_result;

typedef enum stf_i960_halt_reason {
    STF_I960_HALT_NONE = 0,
    STF_I960_HALT_STOP_ADDRESS,
    STF_I960_HALT_MAX_STEPS,
    STF_I960_HALT_SELF_BRANCH,
    STF_I960_HALT_INVALID_INSTRUCTION,
    STF_I960_HALT_UNSUPPORTED_INSTRUCTION,
    STF_I960_HALT_MEMORY_FAULT
} stf_i960_halt_reason;

typedef struct stf_i960_local_frame {
    uint32_t registers[STF_I960_LOCAL_REGISTER_COUNT];
} stf_i960_local_frame;

typedef struct stf_i960_cpu {
    uint32_t registers[STF_I960_REGISTER_COUNT];
    uint32_t sat;
    uint32_t prcb;
    uint32_t ip;
    uint32_t process_control;
    uint32_t arithmetic_control;
    uint32_t interrupt_control;
    stf_i960_compare_result compare_result;
    uint64_t executed_instructions;
    uint64_t procedure_calls;
    uint64_t procedure_returns;
    uint64_t interrupt_entries;
    uint64_t interrupt_returns;
    stf_i960_local_frame local_frames[STF_I960_MAX_LOCAL_FRAMES];
    uint32_t local_frame_depth;
    uint32_t maximum_local_frame_depth;
    bool reinitialized;
} stf_i960_cpu;

typedef struct stf_i960_trace_event {
    uint64_t step;
    uint32_t ip_before;
    uint32_t ip_after;
    stf_i960_instruction instruction;
} stf_i960_trace_event;

typedef void (*stf_i960_trace_callback)(
    const stf_i960_trace_event *event,
    const stf_i960_cpu *cpu,
    void *user_data
);

typedef struct stf_i960_run_options {
    uint32_t stop_address;
    uint64_t max_steps;
    bool stop_on_self_branch;
    stf_i960_trace_callback trace_callback;
    void *trace_user_data;
} stf_i960_run_options;

typedef struct stf_i960_run_result {
    stf_i960_halt_reason halt_reason;
    stf_status status;
    uint32_t halt_address;
    uint64_t executed_instructions;
} stf_i960_run_result;

void stf_i960_cpu_reset(
    stf_i960_cpu *cpu,
    uint32_t sat,
    uint32_t prcb,
    uint32_t start_ip
);

stf_status stf_i960_cpu_reset_from_bus(
    stf_i960_cpu *cpu,
    stf_i960_bus *bus,
    uint32_t sat,
    uint32_t prcb,
    uint32_t start_ip
);

stf_status stf_i960_cpu_enter_procedure(
    stf_i960_cpu *cpu,
    uint32_t target,
    uint32_t return_address
);

stf_status stf_i960_cpu_return_procedure(
    stf_i960_cpu *cpu,
    stf_i960_bus *bus
);

stf_status stf_i960_cpu_enter_interrupt(
    stf_i960_cpu *cpu,
    stf_i960_bus *bus,
    uint32_t vector,
    uint32_t level
);

stf_status stf_i960_step(
    stf_i960_cpu *cpu,
    stf_i960_bus *bus,
    stf_i960_trace_event *event
);

stf_status stf_i960_run(
    stf_i960_cpu *cpu,
    stf_i960_bus *bus,
    const stf_i960_run_options *options,
    stf_i960_run_result *result
);

const char *stf_i960_halt_reason_name(stf_i960_halt_reason reason);

#endif
