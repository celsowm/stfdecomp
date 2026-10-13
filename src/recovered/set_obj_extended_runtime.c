#include "set_obj_extended_runtime.h"

#include <limits.h>
#include <string.h>

enum {
    STF_EXT_SELECTOR_FLAG = 0x8000u,
    STF_EXT_TABLE_NORMAL = 0x020E0004u,
    STF_EXT_TABLE_ALT = 0x0647F004u
};

static bool io(
    const stf_set_obj_model2 *model, stf_set_obj_result *out,
    stf_set_obj_io_kind kind, uint32_t offset,
    uint32_t value, uint32_t *reply
)
{
    if (!model->io(model->io_user_data, kind, offset, value, reply))
        return false;

    ++out->completed_operations;
    switch (kind) {
        case STF_SET_OBJ_FIFO_WRITE: ++out->fifo_writes; break;
        case STF_SET_OBJ_FIFO_READ: ++out->fifo_reads; break;
        case STF_SET_OBJ_GEO_READ: ++out->geo_reads; break;
        case STF_SET_OBJ_GEO_WRITE: ++out->geo_writes; break;
        default: return false;
    }
    return true;
}

static bool select_record(
    const stf_set_obj_model2 *model, stf_set_obj_result *out,
    uint32_t quad[4]
)
{
    const bool ext = (model->object_selector_g0 & STF_EXT_SELECTOR_FLAG) != 0u;
    const uint32_t index = model->object_selector_g0 & ~((uint32_t)STF_EXT_SELECTOR_FLAG);
    const stf_set_obj_record *table = ext ? model->extended : model->normal;
    const size_t count = ext ? model->extended_count : model->normal_count;
    const uint32_t limit = ext ? model->extended_max_poly : model->max_poly;
    const uint32_t base = ext ? STF_EXT_TABLE_ALT : STF_EXT_TABLE_NORMAL;
    unsigned i;

    if (table == NULL || index > limit || (size_t)index >= count ||
        table[index].word[0] == 0u ||
        index > (UINT32_MAX - base) / UINT32_C(16))
        return false;

    out->used_extended = ext;
    out->selected_index = index;
    out->selected_table_address = base + index * UINT32_C(16);
    out->validated_record = true;
    if (quad != NULL)
        for (i = 0u; i < 4u; ++i)
            quad[i] = table[index].word[i];
    return true;
}

bool stf_set_obj_mirror_apply_model2(
    const stf_set_obj_model2 *model, stf_set_obj_result *result
)
{
    stf_set_obj_result out;
    uint32_t quad[4];
    uint32_t pointer = 0u, discarded = 0u;
    unsigned i;
    if (model == NULL || model->state == NULL || result == NULL)
        return false;
    memset(&out, 0, sizeof(out));

    /* set_obj_mir: index and first record word are checked BEFORE
     * POLYGON_LIMIT; the original invalid case calls poly_data_err. */
    if (!select_record(model, &out, quad))
        return false;
    if (model->state->polygon > model->state->polygon_limit) {
        out.gate_closed = true;
        *result = out;
        return true;
    }
    if (model->io == NULL ||
        model->fifo_offset_g12 > UINT32_MAX - UINT32_C(12))
        return false;

    /* The counter and polygon are updated BEFORE GEO/FIFO stores. */
    ++model->state->polygon_counter;
    out.polygon_counter_after = model->state->polygon_counter;
    model->state->polygon += quad[3] & UINT32_C(0xFFFF);
    out.polygon_after = model->state->polygon;
    out.p2_polygon_after = model->state->p2_polygon;
    quad[3] = UINT32_MAX;

    /* mirror: 0xB0B setup, 0x1A003434 FIFO command, pointer+0x30
     * and 0x1111 mirror-mode GEO+0x110 before quad submission. */
    if (!io(model, &out, STF_SET_OBJ_GEO_WRITE,
            UINT32_C(0xB0), UINT32_C(0xB0B), NULL) ||
        !io(model, &out, STF_SET_OBJ_FIFO_WRITE,
            model->fifo_offset_g12, UINT32_C(0x1A003434), NULL) ||
        !io(model, &out, STF_SET_OBJ_GEO_READ,
            UINT32_C(0x2008), 0u, &pointer) ||
        !io(model, &out, STF_SET_OBJ_GEO_WRITE,
            UINT32_C(0x1008), pointer + UINT32_C(0x30), NULL) ||
        !io(model, &out, STF_SET_OBJ_FIFO_WRITE,
            model->fifo_offset_g12, pointer, NULL) ||
        !io(model, &out, STF_SET_OBJ_FIFO_READ,
            model->fifo_offset_g12, 0u, &discarded) ||
        !io(model, &out, STF_SET_OBJ_GEO_WRITE,
            UINT32_C(0x110), UINT32_C(0x1111), NULL)) {
        *result = out;
        return false;
    }
    for (i = 0u; i < 4u; ++i) {
        if (!io(model, &out, STF_SET_OBJ_GEO_WRITE,
                model->fifo_offset_g12 + i * UINT32_C(4),
                quad[i], NULL)) {
            *result = out;
            return false;
        }
    }
    out.submitted = true;
    *result = out;
    return true;
}

static void merge_results(stf_set_obj_result *prior,
                          const stf_set_obj_result *tail)
{
    stf_set_obj_result merged = *tail;
    merged.completed_operations += prior->completed_operations;
    merged.fifo_reads += prior->fifo_reads;
    merged.fifo_writes += prior->fifo_writes;
    merged.geo_reads += prior->geo_reads;
    merged.geo_writes += prior->geo_writes;
    *prior = merged;
}

bool stf_set_obj_fifo_apply_model2(
    const stf_set_obj_model2 *model,
    const stf_set_obj_fifo_context *context,
    stf_set_obj_fifo_result *result
)
{
    stf_set_obj_fifo_result out;
    stf_set_obj_result tail_result;
    uint32_t fifo_quads[12];
    uint32_t read_word;
    unsigned block, word;

    if (model == NULL || model->state == NULL || model->io == NULL ||
        context == NULL || result == NULL ||
        context->g3_words == NULL || context->g3_word_capacity < 12u ||
        model->fifo_offset_g12 > UINT32_MAX - UINT32_C(12))
        return false;

    memset(&out, 0, sizeof(out));
    /* Three CPU ldq from FIFO precede every early exit. Each
     * 32-bit callback forms one component of the original ldq. */
    for (block = 0u; block < 3u; ++block) {
        for (word = 0u; word < 4u; ++word) {
            if (!io(model, &out.object, STF_SET_OBJ_FIFO_READ,
                    model->fifo_offset_g12 + word * UINT32_C(4),
                    0u, &read_word)) {
                *result = out;
                return false;
            }
            fifo_quads[block * 4u + word] = read_word;
        }
        ++out.quads_read;
    }

    /* cmpobe 1,g2; cmpobg POLYGON,POLYGON_LIMIT; then
     * unsigned positive float32 magnitudes (clrbit 0x1F).
     * Compare order is i960 src2 vs src1: reject abs(r4) < 1.0f
     * or abs(r15) > 0.2f. Equality passes both comparisons. */
    if (context->input_g2 == 1u ||
        model->state->polygon > model->state->polygon_limit ||
        (fifo_quads[0] & UINT32_C(0x7FFFFFFF)) < UINT32_C(0x3F800000) ||
        (fifo_quads[11] & UINT32_C(0x7FFFFFFF)) > UINT32_C(0x3E4CCCCD)) {
        out.returned_early = true;
        out.object.gate_closed = true;
        *result = out;
        return true;
    }

    if (!io(model, &out.object, STF_SET_OBJ_GEO_WRITE,
            UINT32_C(0xB0), UINT32_C(0xB0B), NULL)) {
        *result = out;
        return false;
    }
    for (block = 0u; block < 3u; ++block) {
        /* The original writes each quad to GEO then to g3 scratch;
         * on interrupted GEO submission, that scratch quad remains
         * uncommitted even though the host saw earlier word stores. */
        for (word = 0u; word < 4u; ++word) {
            if (!io(model, &out.object, STF_SET_OBJ_GEO_WRITE,
                    model->fifo_offset_g12 + word * UINT32_C(4),
                    fifo_quads[block * 4u + word], NULL)) {
                *result = out;
                return false;
            }
        }
        for (word = 0u; word < 4u; ++word)
            context->g3_words[block * 4u + word] =
                fifo_quads[block * 4u + word];
        out.words_saved += 4u;
        out.g3_bytes_advanced += 16u;
        ++out.quads_saved;
    }

    /* Descriptor validation in original set_obj_fifo happens AFTER
     * all 12 FIFO reads, GEO writes, and g3 snapshots.
     * poly_data_err_fifo rewinds g3 by 0x30 and tail-jumps into
     * the unrecovered debug/interrupt handler. Report, do not
     * invent this handler's effects. */
    memset(&tail_result, 0, sizeof(tail_result));
    if (!select_record(model, &tail_result, NULL)) {
        out.invalid_record = true;
        out.error_handler_unimplemented = true;
        out.g3_bytes_rewound = 48u;
        *result = out;
        return false;
    }

    /* g7+0x1A4 bit 26 and g7+0x70C bit 2 select the same
     * thd_common tail with two distinct generated g2 values. */
    if ((context->g7_flags_1a4 & (UINT32_C(1) << 26)) != 0u &&
        context->g7_short_1aa >= 4u) {
        out.tail = STF_SET_OBJ_FIFO_THD_FLAG26_TAIL;
        out.selected_g2 = UINT32_C(0x00800000);
    } else if ((context->g7_flags_70c & UINT32_C(4)) != 0u &&
               context->g7_short_1aa >= 4u &&
               (context->g7_flags_70c & UINT32_C(1)) != 0u) {
        out.tail = STF_SET_OBJ_FIFO_THD_FLAG2_TAIL;
        out.selected_g2 = UINT32_C(0x00802000);
    } else {
        out.tail = STF_SET_OBJ_FIFO_COMMON_TAIL;
    }

    memset(&tail_result, 0, sizeof(tail_result));
    if (out.tail == STF_SET_OBJ_FIFO_COMMON_TAIL) {
        if (!stf_set_obj_common_apply_model2(
                model, STF_SET_OBJ_COMMON, &tail_result)) {
            merge_results(&out.object, &tail_result);
            *result = out;
            return false;
        }
    } else {
        if (!stf_set_obj_thd_tail_apply_model2(
                model, out.selected_g2, &tail_result)) {
            merge_results(&out.object, &tail_result);
            *result = out;
            return false;
        }
    }
    merge_results(&out.object, &tail_result);
    *result = out;
    return true;
}
