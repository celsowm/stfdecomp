#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "set_obj_runtime.h"

#define CHECK(expr, id) do { if (!(expr)) { \
  fprintf(stderr, "set_obj STF scenario %d failed at %d\n", (id), __LINE__); \
  return (id); } } while (0)

typedef struct event {
    stf_set_obj_io_kind kind;
    uint32_t offset;
    uint32_t value;
} event;

typedef struct fixture {
    stf_set_obj_state state;
    stf_set_obj_record standard[4];
    stf_set_obj_record extended[4];
    stf_set_obj_model2 model;
    stf_set_obj_result result;
    event log[24];
    unsigned calls;
    unsigned fail_at; /* one-based; 0 means always work */
    unsigned reply_index;
} fixture;

static bool io(void *user, stf_set_obj_io_kind kind,
               uint32_t offset, uint32_t value, uint32_t *response)
{
    fixture *f = (fixture *)user;
    const uint32_t fifo_reads[] = {
        UINT32_C(0xDEADBEEF), UINT32_C(0x123), UINT32_C(0x456)
    };
    if (f->calls >= 24u || (f->fail_at != 0u &&
                             f->calls + 1u == f->fail_at)) return false;
    f->log[f->calls].kind = kind;
    f->log[f->calls].offset = offset;
    f->log[f->calls].value = value;
    ++f->calls;
    if (kind == STF_SET_OBJ_GEO_READ) {
        if (response == NULL || offset != 0x2008u || value != 0u)
            return false;
        *response = UINT32_C(0x7000);
    } else if (kind == STF_SET_OBJ_FIFO_READ) {
        if (response == NULL || f->reply_index >= 3u ||
            offset != UINT32_C(0x40) || value != 0u) return false;
        *response = fifo_reads[f->reply_index++];
    } else if (response != NULL) {
        return false;
    }
    return true;
}

static void init(fixture *f)
{
    memset(f, 0, sizeof(*f));
    f->state.polygon_limit = 100u;
    f->state.polygon = 17u;
    f->state.p2_polygon = 5u;
    f->state.polygon_counter = 33u;
    f->state.previous_polygon = 77u;
    f->standard[2].word[0] = UINT32_C(0x11111111);
    f->standard[2].word[1] = UINT32_C(0x22222222);
    f->standard[2].word[2] = UINT32_C(0x33333333);
    f->standard[2].word[3] = UINT32_C(0x44445555);
    f->extended[1].word[0] = UINT32_C(0xABCDEF01);
    f->extended[1].word[1] = UINT32_C(0x1);
    f->extended[1].word[2] = UINT32_C(0x2);
    f->extended[1].word[3] = UINT32_C(0x3);
    f->model.state = &f->state;
    f->model.object_selector_g0 = 2u;
    f->model.normal = f->standard;
    f->model.normal_count = 4u;
    f->model.max_poly = 3u;
    f->model.extended = f->extended;
    f->model.extended_count = 4u;
    f->model.extended_max_poly = 3u;
    f->model.fifo_offset_g12 = UINT32_C(0x40);
    f->model.io = io;
    f->model.io_user_data = f;
}

static bool events_match(fixture *f, bool extended)
{
    const uint32_t *record = extended ? f->extended[1].word :
                                       f->standard[2].word;
    const event expected[] = {
        {STF_SET_OBJ_FIFO_WRITE,0x40u,UINT32_C(0x09801313)},
        {STF_SET_OBJ_FIFO_WRITE,0x40u,UINT32_C(0x09801313)},
        {STF_SET_OBJ_FIFO_WRITE,0x40u,UINT32_C(0x09801313)},
        {STF_SET_OBJ_FIFO_READ,0x40u,0u},
        {STF_SET_OBJ_FIFO_WRITE,0x40u,UINT32_C(0x3C007878)},
        {STF_SET_OBJ_GEO_READ,0x2008u,0u},
        {STF_SET_OBJ_GEO_WRITE,0x1008u,UINT32_C(0x7048)},
        {STF_SET_OBJ_FIFO_WRITE,0x40u,UINT32_C(0x7000)},
        {STF_SET_OBJ_FIFO_WRITE,0x40u,0u},
        {STF_SET_OBJ_FIFO_WRITE,0x40u,record[0]},
        {STF_SET_OBJ_FIFO_WRITE,0x40u,record[1]},
        {STF_SET_OBJ_FIFO_WRITE,0x40u,record[2]},
        {STF_SET_OBJ_FIFO_WRITE,0x40u,record[3]},
        {STF_SET_OBJ_FIFO_WRITE,0x40u,5u},
        {STF_SET_OBJ_FIFO_WRITE,0x40u,17u},
        {STF_SET_OBJ_FIFO_READ,0x40u,0u},
        {STF_SET_OBJ_FIFO_READ,0x40u,0u}
    };
    unsigned i;
    if (f->calls != sizeof(expected)/sizeof(expected[0])) return false;
    for (i = 0u; i < f->calls; ++i) {
        if (f->log[i].kind != expected[i].kind ||
            f->log[i].offset != expected[i].offset ||
            f->log[i].value != expected[i].value) return false;
    }
    return true;
}

static int test_gate(void)
{
    fixture f;
    init(&f);
    f.state.polygon_limit = 15u;
    f.model.io = NULL; /* even absent callbacks are OK on closed gate */
    f.model.normal = NULL;
    CHECK(stf_set_obj_apply_model2(&f.model, &f.result),1);
    CHECK(f.result.gate_closed && !f.result.submitted &&
          !f.result.validated_record &&
          f.result.completed_operations == 0u && f.calls == 0u &&
          f.state.polygon == 17u && f.state.polygon_counter == 33u &&
          f.state.previous_polygon == 77u,2);
    return 0;
}

static int test_normal_and_extended(void)
{
    fixture f;
    init(&f);
    CHECK(stf_set_obj_apply_model2(&f.model,&f.result),3);
    CHECK(f.result.submitted && !f.result.gate_closed &&
          !f.result.used_extended && f.result.validated_record &&
          f.result.selected_index == 2u &&
          f.result.selected_table_address == UINT32_C(0x020E0024) &&
          f.result.completed_operations == 17u &&
          f.result.fifo_writes == 12u && f.result.fifo_reads == 3u &&
          f.result.geo_reads == 1u && f.result.geo_writes == 1u &&
          f.state.previous_polygon == 2u &&
          f.state.polygon_counter == 34u &&
          f.state.p2_polygon == UINT32_C(0x123) &&
          f.state.polygon == UINT32_C(0x456),4);
    CHECK(events_match(&f,false),5);

    init(&f);
    f.model.object_selector_g0 = UINT32_C(0x8001);
    CHECK(stf_set_obj_apply_model2(&f.model,&f.result),6);
    CHECK(f.result.submitted && f.result.used_extended &&
          f.result.selected_index == 1u &&
          f.result.selected_table_address == UINT32_C(0x0647F014) &&
          f.state.previous_polygon == UINT32_C(0x8001) &&
          f.state.polygon_counter == 34u,7);
    CHECK(events_match(&f,true),8);
    return 0;
}

static int test_invalid_selector_is_fail_closed(void)
{
    fixture f;
    stf_set_obj_state saved;
    unsigned i;
    for (i=0u;i<6u;++i) {
        init(&f);
        if (i==0u) f.model.object_selector_g0 = 4u;
        if (i==1u) f.model.max_poly = 1u;
        if (i==2u) f.standard[2].word[0] = 0u;
        if (i==3u) f.model.normal = NULL;
        if (i==4u) f.model.io = NULL;
        if (i==5u) {
            f.model.object_selector_g0 = UINT32_C(0x8004);
            f.model.extended_max_poly = 3u;
        }
        saved=f.state;
        CHECK(!stf_set_obj_apply_model2(&f.model,&f.result),9);
        CHECK(f.calls==0u && memcmp(&f.state,&saved,sizeof(saved))==0,10);
    }
    init(&f);
    CHECK(!stf_set_obj_apply_model2(NULL,&f.result),11);
    CHECK(!stf_set_obj_apply_model2(&f.model,NULL),12);
    f.model.state = NULL;
    CHECK(!stf_set_obj_apply_model2(&f.model,&f.result),13);
    return 0;
}

static int test_partial_failure_boundaries(void)
{
    fixture f;
    unsigned stop;
    for (stop=1u;stop<=17u;++stop) {
        init(&f);
        f.fail_at=stop;
        CHECK(!stf_set_obj_apply_model2(&f.model,&f.result),14);
        CHECK(f.calls==stop-1u &&
              f.result.completed_operations==stop-1u &&
              !f.result.submitted,15);
        CHECK(f.state.polygon_counter==(stop<=4u?33u:34u) &&
              f.state.previous_polygon==(stop<=4u?77u:2u),16);
        CHECK(f.state.p2_polygon==(stop<=16u?5u:UINT32_C(0x123)) &&
              f.state.polygon==17u,17);
    }
    return 0;
}

int main(void)
{
    int rc = test_gate();
    if (rc) return rc;
    rc = test_normal_and_extended();
    if (rc) return rc;
    rc = test_invalid_selector_is_fail_closed();
    if (rc) return rc;
    return test_partial_failure_boundaries();
}
