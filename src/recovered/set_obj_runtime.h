#ifndef STF_RECOVERED_SET_OBJ_RUNTIME_H
#define STF_RECOVERED_SET_OBJ_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * STF-original set_obj (rom_code1.s:4910..4965), not VF2's 0x7C60 set_obj.
 * Decoded from original assembly; the GEO/TGP FIFO is external. This is
 * semantic recovery of the normal entry, not a ROM byte-match or a
 * complete 3D renderer. set_obj_tpd/thd/common are separate entrypoints.
 */

typedef struct stf_set_obj_record {
    uint32_t word[4];
} stf_set_obj_record;

typedef struct stf_set_obj_state {
    uint32_t polygon_limit;     /* POLYGON_LIMIT, 0x501018 */
    uint32_t polygon;           /* POLYGON,       0x50101C */
    uint32_t p2_polygon;        /* P2_POLYGON,    0x5010D0 */
    uint32_t polygon_counter;   /* 0x501010 */
    uint32_t previous_polygon;  /* mae_poly: original g0 selector */
} stf_set_obj_state;

typedef enum stf_set_obj_io_kind {
    STF_SET_OBJ_FIFO_WRITE = 1,
    STF_SET_OBJ_FIFO_READ = 2,
    STF_SET_OBJ_GEO_READ = 3,
    STF_SET_OBJ_GEO_WRITE = 4
} stf_set_obj_io_kind;

/* offset = the caller's FIFO slot (g12), or GEO byte offset +0x2008
 * or +0x1008. value is the exact CPU word to write, zero for reads.
 * The response pointer is non-NULL only on reads. Each bus operation
 * is individually visible and may be rejected. No fabricated TGP reply. */
typedef bool (*stf_set_obj_io_fn)(
    void *user_data, stf_set_obj_io_kind kind, uint32_t offset,
    uint32_t value, uint32_t *response
);

typedef struct stf_set_obj_model2 {
    stf_set_obj_state *state;
    uint32_t object_selector_g0;
    uint32_t max_poly;
    uint32_t extended_max_poly; /* 0x0647F000 for g0 bit 15 */
    const stf_set_obj_record *normal; /* 0x020E0004, stride 16 */
    size_t normal_count;
    const stf_set_obj_record *extended; /* 0x0647F004, stride 16 */
    size_t extended_count;
    uint32_t fifo_offset_g12;
    stf_set_obj_io_fn io;
    void *io_user_data;
} stf_set_obj_model2;

typedef struct stf_set_obj_result {
    bool gate_closed;
    bool used_extended;
    bool validated_record;
    bool submitted;
    uint32_t selected_index;
    uint32_t selected_table_address;
    uint32_t polygon_counter_after;
    uint32_t p2_polygon_after;
    uint32_t polygon_after;
    unsigned completed_operations;
    unsigned fifo_writes;
    unsigned fifo_reads;
    unsigned geo_reads;
    unsigned geo_writes;
} stf_set_obj_result;

/* Does not touch state or hardware on invalid selector/table/gate.
 * The original poly_data_err path has other effects; this semantic
 * adapter fails closed instead of asserting those are recovered.
 * Once FIFO dispatch starts, callback errors preserve already
 * issued operations and any prior CPU stores (no rollback). */
bool stf_set_obj_apply_model2(
    const stf_set_obj_model2 *model, stf_set_obj_result *result
);


/* Original STF set_obj_tpd (rom_code1.s:5012) and set_obj_thd
 * (line 5059). This is deliberately separate from normal set_obj.
 * Both issue 0x1A003434, receive a GEO pointer and then submit a quad:
 * TPD overwrites r8/quad word 0 with g2; THD overwrites r9/word 1.
 * The original stq is one architectural i960 quad store; this
 * portable host callback exposes its four component writes. The
 * actual Model 2 bus atomicity is not claimed.
 */
typedef enum stf_set_obj_special_kind {
    STF_SET_OBJ_TPD = 1,
    STF_SET_OBJ_THD = 2
} stf_set_obj_special_kind;

bool stf_set_obj_special_apply_model2(
    const stf_set_obj_model2 *model,
    stf_set_obj_special_kind kind,
    uint32_t object_g2,
    stf_set_obj_result *result
);

#endif
