#include "tgp_command_trace.h"

#include <math.h>
#include <string.h>

static bool valid_query(const stf_attack_hit_guard_vector_query *query)
{
    /* Currently supported command layouts are scalar 1-word and trig
     * 2-word. Commands with other arities require a separate contract. */
    return query != NULL &&
        (query->argument_count == 1u || query->argument_count == 2u);
}

static bool valid_reply(uint32_t bits)
{
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return isfinite(value);
}

static bool matching_query(
    const stf_attack_hit_guard_vector_query *left,
    const stf_attack_hit_guard_vector_query *right
)
{
    return valid_query(left) && valid_query(right) &&
        left->opcode == right->opcode &&
        left->argument_count == right->argument_count &&
        left->words[0] == right->words[0] &&
        (left->argument_count == 1u ||
         left->words[1] == right->words[1]);
}

bool stf_tgp_command_trace_begin_capture(
    stf_tgp_command_trace *trace,
    stf_tgp_command_record *storage,
    size_t capacity,
    stf_attack_hit_guard_vector_tgp_fn backend,
    void *backend_user_data
)
{
    if (trace == NULL || storage == NULL || capacity == 0u ||
        backend == NULL || backend == stf_tgp_command_trace_callback) {
        return false;
    }
    memset(trace, 0, sizeof(*trace));
    trace->capture = storage;
    trace->capacity = capacity;
    trace->backend = backend;
    trace->backend_user_data = backend_user_data;
    return true;
}

bool stf_tgp_command_trace_begin_replay(
    stf_tgp_command_trace *trace,
    const stf_tgp_command_record *records,
    size_t count
)
{
    if (trace == NULL || records == NULL || count == 0u) {
        return false;
    }
    memset(trace, 0, sizeof(*trace));
    trace->expected = records;
    trace->count = count;
    trace->capacity = count;
    trace->replay = true;
    return true;
}

bool stf_tgp_command_trace_callback(
    void *user_data,
    const stf_attack_hit_guard_vector_query *query,
    uint32_t *response_bits
)
{
    stf_tgp_command_trace *trace = (stf_tgp_command_trace *)user_data;
    uint32_t response = 0u;

    if (trace == NULL || response_bits == NULL) {
        return false;
    }
    if (trace->error != STF_TGP_TRACE_OK) {
        return false;
    }
    if (!valid_query(query)) {
        trace->error = STF_TGP_TRACE_BAD_ARGUMENT;
        trace->error_position = trace->position;
        return false;
    }

    if (trace->replay) {
        const stf_tgp_command_record *item;
        if (trace->expected == NULL || trace->position >= trace->count) {
            trace->error = STF_TGP_TRACE_EXHAUSTED;
            trace->error_position = trace->position;
            trace->actual_on_failure = *query;
            return false;
        }
        item = &trace->expected[trace->position];
        if (!matching_query(&item->request, query)) {
            trace->error = STF_TGP_TRACE_MISMATCH;
            trace->error_position = trace->position;
            trace->actual_on_failure = *query;
            return false;
        }
        if (!valid_reply(item->response_bits)) {
            trace->error = STF_TGP_TRACE_NONFINITE_RESPONSE;
            trace->error_position = trace->position;
            return false;
        }
        *response_bits = item->response_bits;
        ++trace->position;
        return true;
    }

    if (trace->capture == NULL || trace->backend == NULL) {
        trace->error = STF_TGP_TRACE_BAD_ARGUMENT;
        trace->error_position = trace->position;
        return false;
    }
    if (trace->count >= trace->capacity) {
        trace->error = STF_TGP_TRACE_CAPACITY;
        trace->error_position = trace->position;
        trace->actual_on_failure = *query;
        return false;
    }
    if (!trace->backend(trace->backend_user_data, query, &response)) {
        trace->error = STF_TGP_TRACE_BACKEND_FAILED;
        trace->error_position = trace->position;
        trace->actual_on_failure = *query;
        return false;
    }
    if (!valid_reply(response)) {
        trace->error = STF_TGP_TRACE_NONFINITE_RESPONSE;
        trace->error_position = trace->position;
        trace->actual_on_failure = *query;
        return false;
    }

    trace->capture[trace->count].request = *query;
    trace->capture[trace->count].response_bits = response;
    *response_bits = response;
    ++trace->count;
    ++trace->position;
    return true;
}

bool stf_tgp_command_trace_fully_consumed(
    const stf_tgp_command_trace *trace
)
{
    return trace != NULL && trace->replay &&
        trace->error == STF_TGP_TRACE_OK &&
        trace->position == trace->count;
}

stf_tgp_trace_difference stf_tgp_command_trace_compare(
    const stf_tgp_command_record *reference, size_t reference_count,
    const stf_tgp_command_record *candidate, size_t candidate_count,
    size_t *first_difference
)
{
    size_t i;
    const size_t common = reference_count < candidate_count
        ? reference_count : candidate_count;
    if (first_difference != NULL) {
        *first_difference = 0u;
    }
    if ((reference == NULL && reference_count != 0u) ||
        (candidate == NULL && candidate_count != 0u)) {
        return STF_TGP_TRACE_INVALID_INPUT;
    }
    for (i = 0u; i < common; ++i) {
        if (!matching_query(&reference[i].request, &candidate[i].request)) {
            if (first_difference != NULL) *first_difference = i;
            return STF_TGP_TRACE_INPUT_DIFFERENCE;
        }
        if (reference[i].response_bits != candidate[i].response_bits) {
            if (first_difference != NULL) *first_difference = i;
            return STF_TGP_TRACE_REPLY_DIFFERENCE;
        }
    }
    if (reference_count != candidate_count) {
        if (first_difference != NULL) *first_difference = common;
        return STF_TGP_TRACE_LENGTH_DIFFERENCE;
    }
    if (first_difference != NULL) *first_difference = common;
    return STF_TGP_TRACE_IDENTICAL;
}
