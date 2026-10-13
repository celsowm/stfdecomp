#include "stf/recovery/set_obj_i960_bridge.h"

#include <stdlib.h>
#include <string.h>

enum {
    STF_BRIDGE_POLYGON_LIMIT = 0x00501018u,
    STF_BRIDGE_POLYGON = 0x0050101Cu,
    STF_BRIDGE_P2_POLYGON = 0x005010D0u,
    STF_BRIDGE_POLYGON_COUNTER = 0x00501010u,
    STF_BRIDGE_MAE_POLY = 0x0050FFFCu,
    STF_BRIDGE_MAX_POLY = 0x020E0000u,
    STF_BRIDGE_NORMAL_TABLE = 0x020E0004u,
    STF_BRIDGE_EXT_MAX_POLY = 0x0647F000u,
    STF_BRIDGE_EXT_TABLE = 0x0647F004u,
    STF_BRIDGE_SELECTOR_EXTENDED = 0x8000u,
    STF_BRIDGE_GEO_BASE = 0x00800000u,
    STF_BRIDGE_FIFO_FUNCTION_BASE = 0x00880000u,
    STF_BRIDGE_FIFO_OFFSET = 0x00004000u
};

typedef struct bridge_io {
    stf_i960_bus *bus;
    uint32_t geo_base;
    uint32_t fifo_base;
    stf_status last_status;
} bridge_io;

static bool busio(void *user, stf_set_obj_io_kind kind, uint32_t offset,
                  uint32_t value, uint32_t *reply)
{
    bridge_io *ctx = (bridge_io *)user;
    uint32_t addr;
    if (ctx == NULL || ctx->bus == NULL) return false;
    if (kind == STF_SET_OBJ_FIFO_READ || kind == STF_SET_OBJ_FIFO_WRITE) {
        if (offset > UINT32_MAX - ctx->fifo_base) {
            ctx->last_status = STF_ERROR_OUT_OF_BOUNDS;
            return false;
        }
        addr = ctx->fifo_base + offset;
    } else if (kind == STF_SET_OBJ_GEO_READ ||
               kind == STF_SET_OBJ_GEO_WRITE) {
        if (offset > UINT32_MAX - ctx->geo_base) {
            ctx->last_status = STF_ERROR_OUT_OF_BOUNDS;
            return false;
        }
        addr = ctx->geo_base + offset;
    } else {
        ctx->last_status = STF_ERROR_INVALID_ARGUMENT;
        return false;
    }
    if (kind == STF_SET_OBJ_FIFO_READ || kind == STF_SET_OBJ_GEO_READ) {
        if (reply == NULL) {
            ctx->last_status = STF_ERROR_INVALID_ARGUMENT;
            return false;
        }
        ctx->last_status = stf_i960_bus_read_u32(ctx->bus, addr, reply);
    } else {
        ctx->last_status = stf_i960_bus_write_u32(ctx->bus, addr, value);
    }
    return ctx->last_status == STF_OK;
}

static bool read32(stf_i960_bus *bus, uint32_t address, uint32_t *out,
                   stf_set_obj_i960_bridge_result *result, bool descriptor)
{
    result->last_bus_status = stf_i960_bus_read_u32(bus, address, out);
    if (result->last_bus_status != STF_OK) return false;
    if (descriptor) ++result->descriptor_reads;
    else ++result->work_ram_reads;
    return true;
}

static bool write32(stf_i960_bus *bus, uint32_t address, uint32_t value,
                    stf_set_obj_i960_bridge_result *result)
{
    result->last_bus_status = stf_i960_bus_write_u32(bus, address, value);
    if (result->last_bus_status != STF_OK) return false;
    ++result->work_ram_writes;
    return true;
}

static bool sync_globals(stf_i960_bus *bus,
                         const stf_set_obj_state *original,
                         const stf_set_obj_state *updated,
                         stf_set_obj_i960_bridge_result *result)
{
    /* These writes deliberately occur only if the model changed a
     * global.  An unchanged gate/error path is side-effect-free. */
    if (original->polygon_counter != updated->polygon_counter &&
        !write32(bus, STF_BRIDGE_POLYGON_COUNTER,
                 updated->polygon_counter, result)) return false;
    if (original->previous_polygon != updated->previous_polygon &&
        !write32(bus, STF_BRIDGE_MAE_POLY,
                 updated->previous_polygon, result)) return false;
    if (original->p2_polygon != updated->p2_polygon &&
        !write32(bus, STF_BRIDGE_P2_POLYGON,
                 updated->p2_polygon, result)) return false;
    if (original->polygon != updated->polygon &&
        !write32(bus, STF_BRIDGE_POLYGON, updated->polygon, result))
        return false;
    return true;
}

bool stf_set_obj_i960_bridge_execute(
    const stf_i960_cpu *cpu,
    stf_model2b_bus *hardware,
    stf_set_obj_i960_entry entry,
    stf_set_obj_i960_bridge_result *result
)
{
    stf_set_obj_i960_bridge_result out;
    stf_set_obj_state state;
    stf_set_obj_record *table = NULL;
    stf_set_obj_model2 model;
    bridge_io bridge;
    stf_i960_bus *bus;
    const uint32_t selector = cpu != NULL ?
        cpu->registers[STF_I960_G0_REGISTER] : 0u;
    const uint32_t index = selector & ~((uint32_t)STF_BRIDGE_SELECTOR_EXTENDED);
    const bool extended = (selector & STF_BRIDGE_SELECTOR_EXTENDED) != 0u;
    const uint32_t base = extended ? STF_BRIDGE_EXT_TABLE :
                                      STF_BRIDGE_NORMAL_TABLE;
    const uint32_t header = extended ? STF_BRIDGE_EXT_MAX_POLY :
                                        STF_BRIDGE_MAX_POLY;
    uint32_t maximum = 0u;
    uint32_t record_address;
    uint32_t value[4] = {0u, 0u, 0u, 0u};
    uint32_t caller_words[STF_SET_OBJ_STATE_WORDS] = {0u};
    uint32_t caller_base = 0u;
    stf_set_obj_fifo_context fifo_context;
    stf_set_obj_fifo_result fifo;
    stf_set_obj_transfer_result transfer;
    bool ran;
    bool committed;
    unsigned i;

    if (cpu == NULL || hardware == NULL || result == NULL ||
        entry < STF_SET_OBJ_I960_NORMAL ||
        entry > STF_SET_OBJ_I960_LOAD) return false;
    memset(&out, 0, sizeof(out));
    memset(&state, 0, sizeof(state));
    memset(&model, 0, sizeof(model));
    memset(&bridge, 0, sizeof(bridge));

    bus = stf_model2b_bus_i960(hardware);
    if (bus == NULL) return false;
    bridge.bus = bus;
    bridge.last_status = STF_OK;
    bridge.geo_base = cpu->registers[26u];  /* original g10 */
    bridge.fifo_base = cpu->registers[27u]; /* original g11 */
    out.geo_bus_address = bridge.geo_base;
    /* Address materialized only after register validation below. */

    /* Refuse speculative aliases or addresses. These are the known
     * STF startup register mappings, not generalized Model 2 devices. */
    if (bridge.geo_base != STF_BRIDGE_GEO_BASE ||
        bridge.fifo_base != STF_BRIDGE_FIFO_FUNCTION_BASE ||
        cpu->registers[28u] != STF_BRIDGE_FIFO_OFFSET) {
        out.last_bus_status = STF_ERROR_INVALID_ARGUMENT;
        *result = out;
        return false;
    }
    out.fifo_bus_address = bridge.fifo_base + cpu->registers[28u];

    if (!read32(bus, STF_BRIDGE_POLYGON_LIMIT, &state.polygon_limit,&out,false) ||
        !read32(bus, STF_BRIDGE_POLYGON,&state.polygon,&out,false) ||
        !read32(bus, STF_BRIDGE_P2_POLYGON,&state.p2_polygon,&out,false) ||
        !read32(bus, STF_BRIDGE_POLYGON_COUNTER,&state.polygon_counter,&out,false) ||
        !read32(bus, STF_BRIDGE_MAE_POLY,&state.previous_polygon,&out,false)) {
        *result = out;
        return false;
    }
    out.before = state;
    out.after = state;
    /* Source-order guard: normal/special return before touching either
     * descriptor table; SAME also returns at equality. MIRROR deliberately
     * validates the descriptor before its own polygon gate. */
    if ((entry == STF_SET_OBJ_I960_SAME &&
         state.polygon >= state.polygon_limit) ||
        ((entry == STF_SET_OBJ_I960_NORMAL ||
          entry == STF_SET_OBJ_I960_TPD ||
          entry == STF_SET_OBJ_I960_THD) &&
         state.polygon > state.polygon_limit)) {
        out.recovery.gate_closed = true;
        out.invoked = true;
        out.completed = true;
        out.committed = true;
        *result = out;
        return true;
    }
    if (!read32(bus, header, &maximum, &out, true)) {
        *result = out;
        return false;
    }

    /* Index selection is bounds-checked before allocation or ROM
     * access. An invalid selector feeds the existing fail-closed C
     * entrypoint with a zero first word; we do NOT run poly_data_err. */
    if (index <= maximum) {
        if (index > (UINT32_MAX - base) / UINT32_C(16)) {
            out.last_bus_status = STF_ERROR_OUT_OF_BOUNDS;
            *result = out;
            return false;
        }
        record_address = base + index * UINT32_C(16);
        for (i = 0u; i < 4u; ++i) {
            if (!read32(bus, record_address + i * UINT32_C(4),
                        &value[i], &out, true)) {
                *result = out;
                return false;
            }
        }
    } else {
        record_address = base;
    }
    out.descriptor_address = record_address;

    /* Model's unmodified selectors still index by original value.
     * 32K entries occupy at most 512 KiB, but only the target row
     * holds data. No ROM bytes are copied beyond that row. */
    table = (stf_set_obj_record *)calloc((size_t)index + 1u, sizeof(*table));
    if (table == NULL) {
        out.last_bus_status = STF_ERROR_OUT_OF_MEMORY;
        *result = out;
        return false;
    }
    if (index <= maximum) {
        for (i = 0u; i < 4u; ++i) table[index].word[i] = value[i];
    }
    model.state = &state;
    model.object_selector_g0 = selector;
    model.max_poly = extended ? 0u : maximum;
    model.extended_max_poly = extended ? maximum : 0u;
    model.normal = extended ? NULL : table;
    model.normal_count = extended ? 0u : (size_t)index + 1u;
    model.extended = extended ? table : NULL;
    model.extended_count = extended ? (size_t)index + 1u : 0u;
    model.fifo_offset_g12 = cpu->registers[28u];
    model.io = busio;
    model.io_user_data = &bridge;

    /*
     * Original g3 (FIFO scratch) and g2 (save/load target/source)
     * point to caller-owned memory. Restrict this adapter to mapped
     * and word-aligned 48-byte work-RAM windows until other i960
     * memory ownership evidence is available. A rejected window
     * never triggers any device effects.
     */
    if (entry == STF_SET_OBJ_I960_FIFO ||
        entry == STF_SET_OBJ_I960_SAVE ||
        entry == STF_SET_OBJ_I960_LOAD) {
        caller_base = cpu->registers[
            entry == STF_SET_OBJ_I960_FIFO ? 19u : 18u];
        if (caller_base < UINT32_C(0x00500000) ||
            (uint64_t)caller_base + UINT64_C(48) > UINT64_C(0x00600000) ||
            (caller_base & UINT32_C(3)) != 0u) {
            out.last_bus_status = STF_ERROR_OUT_OF_BOUNDS;
            free(table);
            *result = out;
            return false;
        }
        if (entry == STF_SET_OBJ_I960_LOAD) {
            for (i = 0u; i < STF_SET_OBJ_STATE_WORDS; ++i) {
                out.last_bus_status = stf_i960_bus_read_u32(
                    bus, caller_base + i * UINT32_C(4), &caller_words[i]);
                if (out.last_bus_status != STF_OK) {
                    free(table);
                    *result = out;
                    return false;
                }
                ++out.caller_buffer_reads;
            }
        }
    }
    memset(&fifo, 0, sizeof(fifo));
    memset(&transfer, 0, sizeof(transfer));
    memset(&fifo_context, 0, sizeof(fifo_context));
    if (entry == STF_SET_OBJ_I960_FIFO) {
        const uint32_t g7 = cpu->registers[23u];
        uint8_t half[2];
        if (g7 > UINT32_MAX - UINT32_C(0x70C) ||
            !read32(bus, g7 + UINT32_C(0x1A4),
                    &fifo_context.g7_flags_1a4, &out, false) ||
            !read32(bus, g7 + UINT32_C(0x70C),
                    &fifo_context.g7_flags_70c, &out, false)) {
            if (out.last_bus_status == STF_OK)
                out.last_bus_status = STF_ERROR_OUT_OF_BOUNDS;
            free(table);
            *result = out;
            return false;
        }
        out.last_bus_status = stf_i960_bus_read(
            bus, g7 + UINT32_C(0x1AA), half, sizeof(half));
        if (out.last_bus_status != STF_OK) {
            free(table);
            *result = out;
            return false;
        }
        ++out.work_ram_reads;
        fifo_context.g7_short_1aa =
            (uint16_t)((uint16_t)half[0] | ((uint16_t)half[1] << 8u));
        fifo_context.input_g2 = cpu->registers[18u];
        fifo_context.g3_words = caller_words;
        fifo_context.g3_word_capacity = STF_SET_OBJ_STATE_WORDS;
    }

    /* There is no opcode substitution here. This is the recovered
     * C routine applied at an explicit CPU call-boundary snapshot. */
    out.invoked = true;
    switch (entry) {
        case STF_SET_OBJ_I960_NORMAL:
            ran = stf_set_obj_apply_model2(&model, &out.recovery);
            break;
        case STF_SET_OBJ_I960_TPD:
            ran = stf_set_obj_special_apply_model2(
                &model, STF_SET_OBJ_TPD, cpu->registers[18u], &out.recovery);
            break;
        case STF_SET_OBJ_I960_THD:
            ran = stf_set_obj_special_apply_model2(
                &model, STF_SET_OBJ_THD, cpu->registers[18u], &out.recovery);
            break;
        case STF_SET_OBJ_I960_COMMON:
            ran = stf_set_obj_common_apply_model2(
                &model, STF_SET_OBJ_COMMON, &out.recovery);
            break;
        case STF_SET_OBJ_I960_SAME:
            ran = stf_set_obj_common_apply_model2(
                &model, STF_SET_OBJ_SAME, &out.recovery);
            break;
        case STF_SET_OBJ_I960_MIRROR:
            ran = stf_set_obj_mirror_apply_model2(&model, &out.recovery);
            break;
        case STF_SET_OBJ_I960_FIFO:
            ran = stf_set_obj_fifo_apply_model2(&model, &fifo_context, &fifo);
            out.recovery = fifo.object;
            out.fifo_quads_consumed = fifo.quads_read;
            out.fifo_quads_saved = fifo.quads_saved;
            out.fifo_tail = fifo.tail;
            out.g3_rewind_bytes = fifo.g3_bytes_rewound;
            break;
        case STF_SET_OBJ_I960_SAVE:
        case STF_SET_OBJ_I960_LOAD:
            ran = stf_set_obj_transfer_apply_model2(
                &model, entry == STF_SET_OBJ_I960_SAVE ?
                STF_SET_OBJ_SAVE : STF_SET_OBJ_LOAD,
                caller_words, STF_SET_OBJ_STATE_WORDS, &transfer);
            out.recovery = transfer.object;
            out.fifo_quads_saved = transfer.completed_quads;
            out.g2_advanced_bytes = transfer.g2_bytes_advanced;
            break;
        default:
            ran = false;
            break;
    }

    /*
     * Save only quads that the recovered routine fully committed.
     * This stage intentionally follows the semantic adapter rather
     * than claiming instruction-level bus interleaving. A later RAM
     * fault preserves any earlier successful device and memory I/O.
     */
    if (entry == STF_SET_OBJ_I960_FIFO ||
        entry == STF_SET_OBJ_I960_SAVE) {
        const unsigned words = entry == STF_SET_OBJ_I960_FIFO ?
            fifo.words_saved : transfer.saved_words_committed;
        for (i = 0u; i < words; ++i) {
            out.last_bus_status = stf_i960_bus_write_u32(
                bus, caller_base + i * UINT32_C(4), caller_words[i]);
            if (out.last_bus_status != STF_OK) {
                ran = false;
                break;
            }
            ++out.caller_buffer_writes;
        }
    }
    out.after = state;
    out.completed = ran;
    /* Preserve a failed caller-buffer write; otherwise the last
     * device access status remains available until global RAM commits. */
    if (out.last_bus_status == STF_OK)
        out.last_bus_status = bridge.last_status;
    /* If recovered C rejected invalid original record, device
     * status is still OK; caller can examine invoked/completed
     * and recovery.validated_record separately. */
    committed = sync_globals(bus, &out.before, &out.after, &out);
    out.committed = committed;
    if (!committed) out.completed = false;
    free(table);
    *result = out;
    return ran && committed;
}
