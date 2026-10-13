#ifndef STF_RECOVERY_SET_OBJ_I960_BRIDGE_H
#define STF_RECOVERY_SET_OBJ_I960_BRIDGE_H

#include <stdbool.h>
#include <stdint.h>

#include "set_obj_extended_runtime.h"
#include "stf/recovery/i960/executor.h"
#include "stf/recovery/model2b_bus.h"

/*
 * Explicit call-boundary adapter for recovered STF set_obj operations.
 *
 * This is NOT an i960 opcode interceptor, an automatic replacement for
 * original ROM instructions, or a byte-exact CPU implementation. The
 * caller must stop the real i960 executor at the intended call entry,
 * pass the captured CPU registers and bus, and separately compare the
 * result against a reference execution.
 */
typedef enum stf_set_obj_i960_entry {
    STF_SET_OBJ_I960_NORMAL = 1,
    STF_SET_OBJ_I960_TPD = 2,
    STF_SET_OBJ_I960_THD = 3,
    STF_SET_OBJ_I960_COMMON = 4,
    STF_SET_OBJ_I960_SAME = 5,
    STF_SET_OBJ_I960_MIRROR = 6
} stf_set_obj_i960_entry;

typedef struct stf_set_obj_i960_bridge_result {
    stf_set_obj_result recovery;
    stf_set_obj_state before;
    stf_set_obj_state after;
    stf_status last_bus_status;
    bool invoked;
    bool completed;
    bool committed;
    uint32_t descriptor_address;
    uint32_t fifo_bus_address;
    uint32_t geo_bus_address;
    unsigned descriptor_reads;
    unsigned work_ram_reads;
    unsigned work_ram_writes;
} stf_set_obj_i960_bridge_result;

/*
 * The routine reads the original STF globals at 0x501018, 0x50101C,
 * 0x5010D0, 0x501010 and 0x50FFFC from the connected i960 bus.
 * The polygon table header and selected four-word row are read via
 * stf_i960_bus_read_u32(), including any device fault status. The
 * recovered routine's callback uses g11+g12 for FIFO and g10 for GEO.
 *
 * CPU registers are an input snapshot; they are not silently mutated.
 * Changes to the five work-RAM globals are committed to the bus after
 * the semantic routine returns, including partial changes on faults.
 * The commit phase is not interleaved with original i960 instructions.
 *
 * Unknown GEO/FIFO behavior is never supplied implicitly: without
 * explicitly configured device callbacks, access fails closed.
 */
bool stf_set_obj_i960_bridge_execute(
    const stf_i960_cpu *cpu,
    stf_model2b_bus *hardware,
    stf_set_obj_i960_entry entry,
    stf_set_obj_i960_bridge_result *result
);

#endif
