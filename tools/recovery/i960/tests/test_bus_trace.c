#include <stdint.h>
#include <string.h>

#include "stf/recovery/i960/bus.h"
#include "stf/recovery/model2b_bus.h"

typedef struct trace_capture {
    stf_i960_bus_trace_event events[4];
    size_t count;
} trace_capture;

static void capture_trace(
    const stf_i960_bus_trace_event *event,
    void *user_data
)
{
    trace_capture *capture = (trace_capture *)user_data;
    if (capture == NULL || event == NULL || capture->count >= 4u) {
        return;
    }
    capture->events[capture->count++] = *event;
}

int main(void)
{
    uint8_t image[32] = {0u};
    uint32_t value = 0u;
    stf_model2b_bus model2b;
    trace_capture capture;
    const stf_model2b_fault *fault = NULL;
    stf_i960_bus *bus = NULL;

    memset(&capture, 0, sizeof(capture));
    memset(&model2b, 0, sizeof(model2b));

    if (stf_model2b_bus_init(&model2b) != STF_OK) {
        return 1;
    }
    if (stf_model2b_bus_attach_program(&model2b, image, sizeof(image)) != STF_OK) {
        stf_model2b_bus_destroy(&model2b);
        return 2;
    }

    bus = stf_model2b_bus_i960(&model2b);
    stf_i960_bus_set_trace(bus, capture_trace, &capture);
    stf_i960_bus_set_trace_step(bus, 7u);

    if (stf_i960_bus_write_u32(
            bus,
            STF_MODEL2B_WORK_RAM_BASE + 4u,
            0x12345678u
        ) != STF_OK) {
        stf_model2b_bus_destroy(&model2b);
        return 3;
    }

    if (capture.count != 1u ||
        capture.events[0].step != 7u ||
        capture.events[0].kind != STF_I960_BUS_ACCESS_WRITE ||
        capture.events[0].address != STF_MODEL2B_WORK_RAM_BASE + 4u ||
        capture.events[0].byte_count != 4u ||
        capture.events[0].bytes[0] != 0x78u ||
        capture.events[0].bytes[3] != 0x12u ||
        capture.events[0].status != STF_OK) {
        stf_model2b_bus_destroy(&model2b);
        return 4;
    }

    stf_i960_bus_set_trace_step(bus, 8u);
    if (stf_i960_bus_read_u32(bus, 0x00884000u, &value) !=
        STF_ERROR_UNSUPPORTED) {
        stf_model2b_bus_destroy(&model2b);
        return 5;
    }

    fault = stf_model2b_bus_last_fault(&model2b);
    if (capture.count != 2u ||
        capture.events[1].step != 8u ||
        capture.events[1].kind != STF_I960_BUS_ACCESS_READ ||
        capture.events[1].address != 0x00884000u ||
        capture.events[1].byte_count != 0u ||
        capture.events[1].status != STF_ERROR_UNSUPPORTED ||
        fault == NULL || !fault->valid || fault->write ||
        fault->address != 0x00884000u ||
        fault->size != 4u ||
        fault->status != STF_ERROR_UNSUPPORTED) {
        stf_model2b_bus_destroy(&model2b);
        return 6;
    }

    stf_model2b_bus_destroy(&model2b);
    return 0;
}
