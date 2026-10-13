#ifndef STF_RECOVERED_SET_OBJ_STATE_RUNTIME_H
#define STF_RECOVERED_SET_OBJ_STATE_RUNTIME_H

#include "set_obj_runtime.h"

enum {
    STF_SET_OBJ_STATE_QUAD_WORDS = 4u,
    STF_SET_OBJ_STATE_QUAD_COUNT = 3u,
    STF_SET_OBJ_STATE_WORDS = 12u
};

typedef enum stf_set_obj_state_entry {
    STF_SET_OBJ_COMMON = 1,
    STF_SET_OBJ_SAME = 2
} stf_set_obj_state_entry;

typedef enum stf_set_obj_transfer_kind {
    STF_SET_OBJ_SAVE = 1,
    STF_SET_OBJ_LOAD = 2
} stf_set_obj_transfer_kind;

typedef struct stf_set_obj_transfer_result {
    stf_set_obj_result object;
    unsigned completed_quads;
    unsigned saved_words_committed;
    unsigned g2_bytes_advanced;
} stf_set_obj_transfer_result;

/*
 * Original STF set_obj_common -> set_obj_go (rom_code1.s:4967..5009)
 * and set_obj_same (line 5152).
 *
 * COMMON intentionally has NO polygon-limit gate. SAME enters COMMON
 * only if POLYGON is strictly below POLYGON_LIMIT (cmpobl); otherwise
 * it exits without touching RAM or the host GEO callback.
 *
 * These entrypoints do not use the FIFO. The host must bind the actual
 * geometry destination using model->io (STF_SET_OBJ_GEO_WRITE).
 */
/*
 * Shared tail from set_obj_thd_common (rom_code1.s:5092), entered
 * AFTER the caller has performed any FIFO/GEO setup and validation.
 * Selects original record, overwrites the second word with g2 and
 * executes set_obj_go. No extra 0x1A003434 or 0xB0B command is emitted.
 * This is used by set_obj_fifo and is distinct from the full
 * set_obj_thd entrypoint.
 */
bool stf_set_obj_thd_tail_apply_model2(
    const stf_set_obj_model2 *model,
    uint32_t object_g2,
    stf_set_obj_result *result
);

bool stf_set_obj_common_apply_model2(
    const stf_set_obj_model2 *model,
    stf_set_obj_state_entry entry,
    stf_set_obj_result *result
);

/*
 * Original STF set_obj_save and set_obj_load, 3x ldq/stq followed by
 * the same set_obj_common path. A 12-word buffer represents caller g2.
 *
 * SAVE: read three FIFO quads (4 words each), submit those quads to
 * GEO, copy each COMPLETED quad to the caller buffer.
 * LOAD: read each quad from the caller buffer and submit it to GEO.
 * After three quads, invoke the common polygon update and submission.
 *
 * A quad's bus operations are exposed as individual 32-bit callbacks
 * at FIFO/GEO g12 + {0,4,8,12}, not as one atomic Model 2B bus op.
 * g2 pointer progression is reported but the host owns actual storage.
 *
 * As in earlier recovery adapters, callbacks may fail. Prior device
 * operations and already committed caller-buffer quads are NOT undone.
 * Unrecovered CPU error-handler side effects are not fabricated.
 */
bool stf_set_obj_transfer_apply_model2(
    const stf_set_obj_model2 *model,
    stf_set_obj_transfer_kind kind,
    uint32_t *saved_words,
    size_t saved_word_count,
    stf_set_obj_transfer_result *result
);

#endif
