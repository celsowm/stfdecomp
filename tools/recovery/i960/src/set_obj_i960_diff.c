#include "stf/recovery/set_obj_i960_diff.h"

#include <string.h>

enum {
    STF_GEO_MMIO_LO = 0x00800000u,
    STF_GEO_MMIO_HI = 0x00808000u,
    STF_FIFO_MMIO_LO = 0x00880000u,
    STF_FIFO_MMIO_HI = 0x00888000u
};

static bool is_mmio(uint32_t address, size_t size)
{
    const uint64_t end = (uint64_t)address + (uint64_t)size;
    return (address >= STF_GEO_MMIO_LO &&
            end <= STF_GEO_MMIO_HI) ||
           (address >= STF_FIFO_MMIO_LO &&
            end <= STF_FIFO_MMIO_HI);
}

bool stf_set_obj_i960_mmio_capture_init(
    stf_set_obj_i960_mmio_capture *capture,
    stf_i960_bus_trace_event *storage,
    size_t capacity
)
{
    if (capture == NULL || storage == NULL || capacity == 0u)
        return false;
    capture->events = storage;
    capture->capacity = capacity;
    capture->count = 0u;
    capture->overflow = false;
    return true;
}

void stf_set_obj_i960_mmio_capture_callback(
    const stf_i960_bus_trace_event *event,
    void *user_data
)
{
    stf_set_obj_i960_mmio_capture *capture =
        (stf_set_obj_i960_mmio_capture *)user_data;
    if (capture == NULL || event == NULL || capture->events == NULL)
        return;
    if (!is_mmio(event->address, event->size))
        return;
    if (capture->count >= capture->capacity) {
        capture->overflow = true;
        return;
    }
    capture->events[capture->count++] = *event;
}

static bool same_event(const stf_i960_bus_trace_event *lhs,
                       const stf_i960_bus_trace_event *rhs)
{
    if (lhs->kind != rhs->kind ||
        lhs->address != rhs->address ||
        lhs->size != rhs->size ||
        lhs->status != rhs->status ||
        lhs->byte_count != rhs->byte_count)
        return false;
    /* step and IP are intentionally ignored because the recovered C
     * adapter is invoked at an explicit executor boundary. */
    if (lhs->byte_count > STF_I960_BUS_TRACE_MAX_BYTES)
        return false;
    if (lhs->byte_count != 0u &&
        memcmp(lhs->bytes, rhs->bytes, lhs->byte_count) != 0)
        return false;
    return true;
}

bool stf_set_obj_i960_mmio_compare(
    const stf_set_obj_i960_mmio_capture *original,
    const stf_set_obj_i960_mmio_capture *recovered,
    stf_set_obj_i960_mmio_difference *out
)
{
    size_t index;
    size_t minimum;
    if (original == NULL || recovered == NULL || out == NULL)
        return false;
    memset(out, 0, sizeof(*out));
    out->original_count = original->count;
    out->recovered_count = recovered->count;
    out->capture_complete = !original->overflow && !recovered->overflow &&
        original->events != NULL && recovered->events != NULL &&
        original->count <= original->capacity &&
        recovered->count <= recovered->capacity;
    if (!out->capture_complete)
        return false;
    minimum = original->count < recovered->count ?
        original->count : recovered->count;
    for (index = 0u; index < minimum; ++index) {
        if (!same_event(&original->events[index], &recovered->events[index])) {
            out->first_difference = index;
            return true;
        }
    }
    out->first_difference = minimum;
    out->equal = original->count == recovered->count;
    return true;
}
