#include "set_obj_state_runtime.h"

#include <limits.h>
#include <string.h>

enum {
    STF_SET_OBJ_EXT_FLAG = 0x8000u,
    STF_SET_OBJ_NORMAL_BASE = 0x020E0004u,
    STF_SET_OBJ_EXT_BASE = 0x0647F004u
};

static bool bus(
    const stf_set_obj_model2 *model, stf_set_obj_result *result,
    stf_set_obj_io_kind kind, uint32_t offset,
    uint32_t value, uint32_t *reply
)
{
    if (!model->io(model->io_user_data, kind, offset, value, reply)) {
        return false;
    }
    ++result->completed_operations;
    switch (kind) {
        case STF_SET_OBJ_FIFO_READ: ++result->fifo_reads; break;
        case STF_SET_OBJ_FIFO_WRITE: ++result->fifo_writes; break;
        case STF_SET_OBJ_GEO_READ: ++result->geo_reads; break;
        case STF_SET_OBJ_GEO_WRITE: ++result->geo_writes; break;
        default: return false;
    }
    return true;
}

static bool select_record(
    const stf_set_obj_model2 *model, stf_set_obj_result *result,
    uint32_t words[4]
)
{
    const stf_set_obj_record *table;
    size_t count;
    uint32_t index;
    uint32_t maximum;
    uint32_t base;
    unsigned i;
    bool extended;

    extended = (model->object_selector_g0 & STF_SET_OBJ_EXT_FLAG) != 0u;
    index = model->object_selector_g0 & ~((uint32_t)STF_SET_OBJ_EXT_FLAG);
    table = extended ? model->extended : model->normal;
    count = extended ? model->extended_count : model->normal_count;
    maximum = extended ? model->extended_max_poly : model->max_poly;
    base = extended ? STF_SET_OBJ_EXT_BASE : STF_SET_OBJ_NORMAL_BASE;
    if (table == NULL || index > maximum ||
        (size_t)index >= count || table[index].word[0] == 0u ||
        index > (UINT32_MAX - base) / UINT32_C(16)) {
        return false;
    }
    result->used_extended = extended;
    result->selected_index = index;
    result->selected_table_address = base + index * UINT32_C(16);
    result->validated_record = true;
    for (i = 0u; i < 4u; ++i) {
        words[i] = table[index].word[i];
    }
    return true;
}

bool stf_set_obj_common_apply_model2(
    const stf_set_obj_model2 *model,
    stf_set_obj_state_entry entry,
    stf_set_obj_result *result
)
{
    stf_set_obj_result out;
    uint32_t words[4];
    unsigned i;

    if (model == NULL || model->state == NULL || result == NULL ||
        (entry != STF_SET_OBJ_COMMON && entry != STF_SET_OBJ_SAME)) {
        return false;
    }
    memset(&out, 0, sizeof(out));

    /* set_obj_same: cmpobl POLYGON, POLYGON_LIMIT, common; ret. */
    if (entry == STF_SET_OBJ_SAME &&
        model->state->polygon >= model->state->polygon_limit) {
        out.gate_closed = true;
        *result = out;
        return true;
    }

    /* The original common path validates the object table before
     * incrementing POLYGON_COUNT or issuing GEO writes. */
    if (model->io == NULL || !select_record(model, &out, words) ||
        model->fifo_offset_g12 > UINT32_MAX - UINT32_C(12)) {
        return false;
    }
    ++model->state->polygon_counter;
    out.polygon_counter_after = model->state->polygon_counter;

    /* set_obj_go: shro 16,r11,r3 ; and 0xffff,r11;
     * POLYGON += low16(r11); subo 1,0,r11 => UINT32_MAX. */
    model->state->polygon += words[3] & UINT32_C(0xFFFF);
    out.polygon_after = model->state->polygon;
    out.p2_polygon_after = model->state->p2_polygon;
    words[3] = UINT32_MAX;

    /* mov 0x101,g14; st g14,0x10(g10); restore g14. */
    if (!bus(model, &out, STF_SET_OBJ_GEO_WRITE,
             UINT32_C(0x10), UINT32_C(0x101), NULL)) {
        *result = out;
        return false;
    }

    /* stq r8,(g10)[g12*1]. Expose the component words on the bus
     * without claiming atomicity of the actual quad store. */
    for (i = 0u; i < 4u; ++i) {
        if (!bus(model, &out, STF_SET_OBJ_GEO_WRITE,
                 model->fifo_offset_g12 + (uint32_t)i * UINT32_C(4),
                 words[i], NULL)) {
            *result = out;
            return false;
        }
    }
    out.submitted = true;
    *result = out;
    return true;
}

static void merge_object_result(stf_set_obj_result *out,
                                const stf_set_obj_result *following)
{
    const unsigned earlier_ops = out->completed_operations;
    const unsigned earlier_reads = out->fifo_reads;
    const unsigned earlier_writes = out->fifo_writes;
    const unsigned earlier_geo_reads = out->geo_reads;
    const unsigned earlier_geo_writes = out->geo_writes;
    *out = *following;
    out->completed_operations += earlier_ops;
    out->fifo_reads += earlier_reads;
    out->fifo_writes += earlier_writes;
    out->geo_reads += earlier_geo_reads;
    out->geo_writes += earlier_geo_writes;
}

bool stf_set_obj_transfer_apply_model2(
    const stf_set_obj_model2 *model,
    stf_set_obj_transfer_kind kind,
    uint32_t *saved_words,
    size_t saved_word_count,
    stf_set_obj_transfer_result *result
)
{
    stf_set_obj_transfer_result out;
    stf_set_obj_result common;
    uint32_t quad[4];
    unsigned block;
    unsigned i;

    if (model == NULL || model->state == NULL || result == NULL ||
        (kind != STF_SET_OBJ_SAVE && kind != STF_SET_OBJ_LOAD)) {
        return false;
    }
    memset(&out, 0, sizeof(out));

    /* Both original entrypoints start with the normal (strict >)
     * POLYGON_LIMIT gate. */
    if (model->state->polygon > model->state->polygon_limit) {
        out.object.gate_closed = true;
        *result = out;
        return true;
    }
    if (model->io == NULL || saved_words == NULL ||
        saved_word_count < STF_SET_OBJ_STATE_WORDS ||
        model->fifo_offset_g12 > UINT32_MAX - UINT32_C(12)) {
        return false;
    }

    if (!bus(model, &out.object, STF_SET_OBJ_GEO_WRITE,
             UINT32_C(0xB0), UINT32_C(0xB0B), NULL)) {
        *result = out;
        return false;
    }
    if (kind == STF_SET_OBJ_SAVE &&
        !bus(model, &out.object, STF_SET_OBJ_FIFO_WRITE,
             model->fifo_offset_g12, UINT32_C(0x02800505), NULL)) {
        *result = out;
        return false;
    }
    for (block = 0u; block < STF_SET_OBJ_STATE_QUAD_COUNT; ++block) {
        if (kind == STF_SET_OBJ_SAVE) {
            for (i = 0u; i < 4u; ++i) {
                if (!bus(model, &out.object, STF_SET_OBJ_FIFO_READ,
                         model->fifo_offset_g12 + (uint32_t)i * UINT32_C(4),
                         0u, &quad[i])) {
                    *result = out;
                    return false;
                }
            }
        } else {
            for (i = 0u; i < 4u; ++i) {
                quad[i] = saved_words[block * 4u + i];
            }
            /* Original load increments g2 before its GEO stq. */
            out.g2_bytes_advanced += 16u;
        }

        for (i = 0u; i < 4u; ++i) {
            if (!bus(model, &out.object, STF_SET_OBJ_GEO_WRITE,
                     model->fifo_offset_g12 + (uint32_t)i * UINT32_C(4),
                     quad[i], NULL)) {
                *result = out;
                return false;
            }
        }
        if (kind == STF_SET_OBJ_SAVE) {
            for (i = 0u; i < 4u; ++i) {
                saved_words[block * 4u + i] = quad[i];
            }
            out.saved_words_committed += 4u;
            out.g2_bytes_advanced += 16u;
        }
        ++out.completed_quads;
    }

    /* Both routines tail-branch to set_obj_common, which has no
     * second polygon gate. State writes from the save/load phase
     * remain visible if common validation later fails. */
    memset(&common, 0, sizeof(common));
    if (!stf_set_obj_common_apply_model2(model, STF_SET_OBJ_COMMON, &common)) {
        merge_object_result(&out.object, &common);
        *result = out;
        return false;
    }
    merge_object_result(&out.object, &common);
    *result = out;
    return true;
}
