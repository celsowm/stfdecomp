#include "set_obj_runtime.h"

#include <string.h>

enum {
    STF_SET_OBJ_GEO_PTR = 0x2008u,
    STF_SET_OBJ_GEO_END = 0x1008u,
    STF_SET_OBJ_INDEX_FLAG = 0x8000u,
    STF_SET_OBJ_NORMAL_BASE = 0x020E0004u,
    STF_SET_OBJ_EXT_BASE = 0x0647F004u
};

static bool issue(
    const stf_set_obj_model2 *model, stf_set_obj_result *local,
    stf_set_obj_io_kind kind, uint32_t offset,
    uint32_t value, uint32_t *response
)
{
    if (!model->io(model->io_user_data, kind, offset, value, response)) {
        return false;
    }
    ++local->completed_operations;
    switch (kind) {
        case STF_SET_OBJ_FIFO_WRITE: ++local->fifo_writes; break;
        case STF_SET_OBJ_FIFO_READ: ++local->fifo_reads; break;
        case STF_SET_OBJ_GEO_READ: ++local->geo_reads; break;
        case STF_SET_OBJ_GEO_WRITE: ++local->geo_writes; break;
        default: return false;
    }
    return true;
}

static bool fifo_write(
    const stf_set_obj_model2 *model, stf_set_obj_result *local,
    uint32_t word
)
{
    return issue(model, local, STF_SET_OBJ_FIFO_WRITE,
                 model->fifo_offset_g12, word, NULL);
}

static bool fifo_read(
    const stf_set_obj_model2 *model, stf_set_obj_result *local,
    uint32_t *word
)
{
    return issue(model, local, STF_SET_OBJ_FIFO_READ,
                 model->fifo_offset_g12, 0u, word);
}

bool stf_set_obj_apply_model2(
    const stf_set_obj_model2 *model, stf_set_obj_result *result
)
{
    stf_set_obj_result local;
    stf_set_obj_state *state;
    const stf_set_obj_record *record;
    uint32_t selector;
    uint32_t index;
    uint32_t max_index;
    uint32_t record_base;
    uint32_t reply = 0u;
    uint32_t pointer = 0u;
    bool extended;
    size_t count;
    unsigned i;

    if (model == NULL || model->state == NULL || result == NULL) {
        return false;
    }
    state = model->state;
    memset(&local, 0, sizeof(local));

    /* STF set_obj: ld POLYGON_LIMIT r4; ld POLYGON r5;
     * cmpobg r5,r4,loc_504C; ret. Entire gate has no bus access. */
    if (state->polygon > state->polygon_limit) {
        local.gate_closed = true;
        *result = local;
        return true;
    }

    /* mov 0,g1 and bbs 0xF,g0. Check both candidate source tables
     * before any FIFO command or global RAM mutation. The assembly
     * branches to poly_data_err on invalid index/zero first word;
     * that error path remains unknown and is not synthesized. */
    selector = model->object_selector_g0;
    extended = (selector & STF_SET_OBJ_INDEX_FLAG) != 0u;
    index = selector & ~((uint32_t)STF_SET_OBJ_INDEX_FLAG);
    record = extended ? model->extended : model->normal;
    count = extended ? model->extended_count : model->normal_count;
    max_index = extended ? model->extended_max_poly : model->max_poly;
    record_base = extended ? STF_SET_OBJ_EXT_BASE : STF_SET_OBJ_NORMAL_BASE;

    if (model->io == NULL || record == NULL ||
        index > max_index || (size_t)index >= count ||
        record[index].word[0] == 0u ||
        index > (UINT32_MAX - record_base) / UINT32_C(16)) {
        return false;
    }
    record += index;
    local.used_extended = extended;
    local.selected_index = index;
    local.selected_table_address = record_base + index * UINT32_C(16);
    local.validated_record = true;

    /* set_obj+0x48: emit three copies of 0x09801313 and read one
     * reply from FIFO. First read is synchronization, ignored by CPU. */
    for (i = 0u; i < 3u; ++i) {
        if (!fifo_write(model, &local, UINT32_C(0x09801313))) {
            *result = local;
            return false;
        }
    }
    if (!fifo_read(model, &local, &reply)) {
        *result = local;
        return false;
    }

    /* st g0,mae_poly; ld/addo/st 0x501010. These happen BEFORE
     * the next FIFO opcode and must survive later bus failures. */
    state->previous_polygon = selector;
    ++state->polygon_counter;
    local.polygon_counter_after = state->polygon_counter;

    if (!fifo_write(model, &local, UINT32_C(0x3C007878))) {
        *result = local;
        return false;
    }
    if (!issue(model, &local, STF_SET_OBJ_GEO_READ,
               STF_SET_OBJ_GEO_PTR, 0u, &pointer)) {
        *result = local;
        return false;
    }
    if (!issue(model, &local, STF_SET_OBJ_GEO_WRITE,
               STF_SET_OBJ_GEO_END, pointer + UINT32_C(0x48), NULL)) {
        *result = local;
        return false;
    }

    if (!fifo_write(model, &local, pointer) ||
        !fifo_write(model, &local, 0u)) {
        *result = local;
        return false;
    }
    for (i = 0u; i < 4u; ++i) {
        if (!fifo_write(model, &local, record->word[i])) {
            *result = local;
            return false;
        }
    }
    if (!fifo_write(model, &local, state->p2_polygon) ||
        !fifo_write(model, &local, state->polygon)) {
        *result = local;
        return false;
    }

    /* Two independent FIFO replies overwrite P2_POLYGON and POLYGON
     * in this exact order. The last read is a real device reply, NOT
     * a computed polygon count like in the VF2 set_obj routine. */
    if (!fifo_read(model, &local, &reply)) {
        *result = local;
        return false;
    }
    state->p2_polygon = reply;
    local.p2_polygon_after = reply;
    if (!fifo_read(model, &local, &reply)) {
        *result = local;
        return false;
    }
    state->polygon = reply;
    local.polygon_after = reply;
    local.submitted = true;
    *result = local;
    return true;
}


bool stf_set_obj_special_apply_model2(
    const stf_set_obj_model2 *model,
    stf_set_obj_special_kind kind,
    uint32_t object_g2,
    stf_set_obj_result *result
)
{
    stf_set_obj_result local;
    stf_set_obj_state *state;
    const stf_set_obj_record *table;
    uint32_t selector;
    uint32_t index;
    uint32_t max_index;
    uint32_t table_base;
    uint32_t quad[4];
    uint32_t pointer = 0u;
    uint32_t discarded_reply = 0u;
    size_t table_count;
    bool extended;
    unsigned i;

    if (model == NULL || model->state == NULL || result == NULL ||
        (kind != STF_SET_OBJ_TPD && kind != STF_SET_OBJ_THD)) {
        return false;
    }
    state = model->state;
    memset(&local, 0, sizeof(local));

    /* Both entries begin with the same exact POLYGON_LIMIT/POLYGON
     * unsigned gate. An already-full output path issues no I/O. */
    if (state->polygon > state->polygon_limit) {
        local.gate_closed = true;
        *result = local;
        return true;
    }
    if (model->io == NULL) {
        return false;
    }

    /* Unlike normal set_obj, the TPD/THD source performs these
     * hardware writes BEFORE the selector validation. Failed
     * poly_data_err is not synthesized; earlier device writes remain. */
    if (!issue(model, &local, STF_SET_OBJ_GEO_WRITE,
               UINT32_C(0xB0), UINT32_C(0xB0B), NULL) ||
        !fifo_write(model, &local, UINT32_C(0x1A003434)) ||
        !issue(model, &local, STF_SET_OBJ_GEO_READ,
               STF_SET_OBJ_GEO_PTR, 0u, &pointer) ||
        !issue(model, &local, STF_SET_OBJ_GEO_WRITE,
               STF_SET_OBJ_GEO_END, pointer + UINT32_C(0x30), NULL) ||
        !fifo_write(model, &local, pointer) ||
        !fifo_read(model, &local, &discarded_reply)) {
        *result = local;
        return false;
    }

    selector = model->object_selector_g0;
    extended = (selector & STF_SET_OBJ_INDEX_FLAG) != 0u;
    index = selector & ~((uint32_t)STF_SET_OBJ_INDEX_FLAG);
    max_index = extended ? model->extended_max_poly : model->max_poly;
    table = extended ? model->extended : model->normal;
    table_count = extended ? model->extended_count : model->normal_count;
    table_base = extended ? STF_SET_OBJ_EXT_BASE : STF_SET_OBJ_NORMAL_BASE;

    if (table == NULL || index > max_index ||
        (size_t)index >= table_count ||
        table[index].word[0] == 0u ||
        index > (UINT32_MAX - table_base) / UINT32_C(16)) {
        *result = local;
        return false;
    }
    local.validated_record = true;
    local.used_extended = extended;
    local.selected_index = index;
    local.selected_table_address = table_base + index * UINT32_C(16);

    /* 0x501010++, ldq (g0),r8, then the entry-specific mov:
     *   TPD: mov g2,r8 ; THD: mov g2,r9.
     * Shared set_obj_go splits the fourth word into high and low,
     * adds the low 16 bits to POLYGON, and makes r11 = 0-1. */
    ++state->polygon_counter;
    local.polygon_counter_after = state->polygon_counter;
    for (i = 0u; i < 4u; ++i) {
        quad[i] = table[index].word[i];
    }
    state->polygon += quad[3] & UINT32_C(0xFFFF);
    local.polygon_after = state->polygon;
    quad[3] = UINT32_MAX;
    if (kind == STF_SET_OBJ_TPD) {
        quad[0] = object_g2;
    } else {
        quad[1] = object_g2;
    }

    /* Save/restore g14 via mmode has no net CPU state effect in this
     * semantic adapter; the recovered device write does matter. */
    if (!issue(model, &local, STF_SET_OBJ_GEO_WRITE,
               UINT32_C(0x10), UINT32_C(0x101), NULL)) {
        *result = local;
        return false;
    }
    /* The original writes stq r8,(g10)[g12*1]. Exposing the four
     * words through the callback preserves their order while leaving
     * bus-wide atomic behavior outside this portable recovery. */
    for (i = 0u; i < 4u; ++i) {
        if (!issue(model, &local, STF_SET_OBJ_GEO_WRITE,
                   model->fifo_offset_g12 + (uint32_t)i * UINT32_C(4),
                   quad[i], NULL)) {
            *result = local;
            return false;
        }
    }

    local.submitted = true;
    local.p2_polygon_after = state->p2_polygon;
    *result = local;
    return true;
}
