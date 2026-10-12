#ifndef STF_RECOVERED_TGP_COMMAND_TRACE_H
#define STF_RECOVERED_TGP_COMMAND_TRACE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "attack_hit_guard_vector.h"

/* A reply is a raw IEEE-754 bit pattern from an actual Model 2 TGP trace.
 * The tuple identifies the exact submitted opcode and FIFO argument words.
 * Synthetic test fixtures MUST NOT be mistaken for hardware captures. */
typedef struct stf_tgp_command_record {
    stf_attack_hit_guard_vector_query request;
    uint32_t response_bits;
} stf_tgp_command_record;

typedef enum stf_tgp_trace_error {
    STF_TGP_TRACE_OK = 0,
    STF_TGP_TRACE_BAD_ARGUMENT,
    STF_TGP_TRACE_EXHAUSTED,
    STF_TGP_TRACE_MISMATCH,
    STF_TGP_TRACE_BACKEND_FAILED,
    STF_TGP_TRACE_NONFINITE_RESPONSE,
    STF_TGP_TRACE_CAPACITY
} stf_tgp_trace_error;

typedef struct stf_tgp_command_trace {
    stf_tgp_command_record *capture;
    const stf_tgp_command_record *expected;
    size_t capacity;
    size_t count;
    size_t position;
    stf_attack_hit_guard_vector_tgp_fn backend;
    void *backend_user_data;
    bool replay;
    stf_tgp_trace_error error;
    size_t error_position;
    stf_attack_hit_guard_vector_query actual_on_failure;
} stf_tgp_command_trace;

/* Capture only through a concrete external TGP provider. The caller owns
 * the output array and can serialize it after the game frame finishes. */
bool stf_tgp_command_trace_begin_capture(
    stf_tgp_command_trace *trace,
    stf_tgp_command_record *storage,
    size_t capacity,
    stf_attack_hit_guard_vector_tgp_fn backend,
    void *backend_user_data
);

/* Replay an immutable previously captured transcript, checking opcode and
 * EVERY input word before returning the bit-exact reply. No host fallback. */
bool stf_tgp_command_trace_begin_replay(
    stf_tgp_command_trace *trace,
    const stf_tgp_command_record *records,
    size_t count
);

/* Pass this as guard_vector_tgp or normal_knockback_tgp. On mismatch or
 * failure, the reply word is unchanged and the trace is latched failed. */
bool stf_tgp_command_trace_callback(
    void *user_data,
    const stf_attack_hit_guard_vector_query *query,
    uint32_t *response_bits
);

/* Only a full, error-free replay proves its input/output transcript matched.
 * It does not prove the transcript came from actual hardware. */
bool stf_tgp_command_trace_fully_consumed(
    const stf_tgp_command_trace *trace
);

/* Compare capture or emulator runs with an independent reference; report
 * first divergent index and distinguish input vs response differences. */
typedef enum stf_tgp_trace_difference {
    STF_TGP_TRACE_IDENTICAL = 0,
    STF_TGP_TRACE_INPUT_DIFFERENCE,
    STF_TGP_TRACE_REPLY_DIFFERENCE,
    STF_TGP_TRACE_LENGTH_DIFFERENCE,
    STF_TGP_TRACE_INVALID_INPUT
} stf_tgp_trace_difference;

stf_tgp_trace_difference stf_tgp_command_trace_compare(
    const stf_tgp_command_record *reference, size_t reference_count,
    const stf_tgp_command_record *candidate, size_t candidate_count,
    size_t *first_difference
);

/* Compare the FINAL fighter impact vectors, after any CPU multipliers.
 * Unlike trace_compare this checks the visible X/Y/Z bit patterns.
 * mismatch_mask bits 0/1/2 indicate X/Y/Z bitwise differences; the
 * corresponding ordered-float ULP distances quantify the divergence.
 * Finite inputs only; the function never modifies source arrays. */
typedef struct stf_tgp_vector_difference {
    uint8_t mismatch_mask;
    uint32_t ulp_distance[3];
} stf_tgp_vector_difference;

bool stf_tgp_command_trace_compare_vectors(
    const uint32_t reference_xyz_bits[3],
    const uint32_t candidate_xyz_bits[3],
    stf_tgp_vector_difference *difference
);

#endif
