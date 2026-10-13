#ifndef STF_RECOVERED_SET_OBJ_EXTENDED_RUNTIME_H
#define STF_RECOVERED_SET_OBJ_EXTENDED_RUNTIME_H

#include "set_obj_state_runtime.h"

/*
 * Original Model 2B set_obj_mir (rom_code1.s:5229..5284).
 * Unlike set_obj, the polygon table is validated BEFORE the
 * polygon-limit gate. Not interchangeable with VF2's set_obj.
 */
bool stf_set_obj_mirror_apply_model2(
    const stf_set_obj_model2 *model, stf_set_obj_result *result
);

/*
 * Original set_obj_fifo (rom_code1.s:5158..5223).
 * Reads three quads from the coprocessor FIFO before its early gates.
 * g7 members are caller-provided snapshots of RAM reads at
 * 0x1A4, 0x1AA (unsigned halfword), and 0x70C.
 * Caller owns the 12-word g3 scratch area; saved quads are committed
 * only after the corresponding GEO stq completes.
 */
typedef struct stf_set_obj_fifo_context {
    uint32_t input_g2;
    uint32_t g7_flags_1a4;
    uint16_t g7_short_1aa;
    uint32_t g7_flags_70c;
    uint32_t *g3_words;
    size_t g3_word_capacity;
} stf_set_obj_fifo_context;

typedef enum stf_set_obj_fifo_tail {
    STF_SET_OBJ_FIFO_NO_TAIL = 0,
    STF_SET_OBJ_FIFO_COMMON_TAIL = 1,
    STF_SET_OBJ_FIFO_THD_FLAG26_TAIL = 2,
    STF_SET_OBJ_FIFO_THD_FLAG2_TAIL = 3
} stf_set_obj_fifo_tail;

typedef struct stf_set_obj_fifo_result {
    stf_set_obj_result object;
    bool returned_early;
    bool invalid_record;
    bool error_handler_unimplemented;
    stf_set_obj_fifo_tail tail;
    unsigned quads_read;
    unsigned quads_saved;
    unsigned words_saved;
    unsigned g3_bytes_advanced;
    unsigned g3_bytes_rewound;
    uint32_t selected_g2;
} stf_set_obj_fifo_result;

/*
 * Both paths preserve successful host bus effects if a later operation
 * fails. The invalid-selector branch in original set_obj_fifo executes
 * poly_data_err_fifo: g3-=0x30 then enters the debug/interrupt-wait
 * handler. This portable implementation reports that pending error
 * (g3_bytes_rewound=48) and returns false, NEVER pretends to run it.
 *
 * Word-level FIFO/GEO callbacks are portable approximations of i960
 * ldq/stq; they are not hardware bus timing or atomically equivalent.
 */
bool stf_set_obj_fifo_apply_model2(
    const stf_set_obj_model2 *model,
    const stf_set_obj_fifo_context *context,
    stf_set_obj_fifo_result *result
);

#endif
