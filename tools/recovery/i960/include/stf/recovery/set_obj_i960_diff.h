#ifndef STF_RECOVERY_SET_OBJ_I960_DIFF_H
#define STF_RECOVERY_SET_OBJ_I960_DIFF_H

#include <stdbool.h>
#include <stddef.h>
#include "stf/recovery/i960/bus.h"

/*
 * Device-only subset of stf_i960_bus_trace_event. Can compare the
 * original i960 runner's bus trace with the call-boundary C adapter
 * using the SAME deterministic Model 2B callback replies.
 *
 * This does not prove equivalence of full CPU regs, RAM or scheduling.
 * Other work-RAM accesses and descriptor prefetch are deliberately
 * excluded, because the adapter stages them at the boundary.
 */
typedef struct stf_set_obj_i960_mmio_capture {
    stf_i960_bus_trace_event *events;
    size_t capacity;
    size_t count;
    bool overflow;
} stf_set_obj_i960_mmio_capture;

typedef struct stf_set_obj_i960_mmio_difference {
    size_t first_difference;
    size_t original_count;
    size_t recovered_count;
    bool equal;
    bool capture_complete;
} stf_set_obj_i960_mmio_difference;

bool stf_set_obj_i960_mmio_capture_init(
    stf_set_obj_i960_mmio_capture *capture,
    stf_i960_bus_trace_event *storage,
    size_t capacity
);

void stf_set_obj_i960_mmio_capture_callback(
    const stf_i960_bus_trace_event *event,
    void *user_data
);

bool stf_set_obj_i960_mmio_compare(
    const stf_set_obj_i960_mmio_capture *original,
    const stf_set_obj_i960_mmio_capture *recovered,
    stf_set_obj_i960_mmio_difference *out
);

#endif
