#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "set_obj_state_runtime.h"

#define CHECK(expr,n) do { if (!(expr)) { \
    fprintf(stderr,"set_obj state case %d at line %d\n",(n),__LINE__); \
    return (n); } } while (0)

typedef struct event {
    stf_set_obj_io_kind kind;
    uint32_t offset;
    uint32_t value;
} event;

typedef struct fixture {
    stf_set_obj_state state;
    stf_set_obj_record standard[4], extended[4];
    stf_set_obj_model2 model;
    stf_set_obj_result result;
    stf_set_obj_transfer_result transfer;
    uint32_t saved[STF_SET_OBJ_STATE_WORDS + 2u];
    event ops[48];
    unsigned count;
    unsigned fail_at;
    unsigned read_count;
} fixture;

static bool io(void *opaque, stf_set_obj_io_kind kind,
               uint32_t offset, uint32_t value, uint32_t *reply)
{
    fixture *f = (fixture *)opaque;
    if (f->count >= 48u || (f->fail_at != 0u &&
                             f->count + 1u == f->fail_at))
        return false;
    f->ops[f->count].kind = kind;
    f->ops[f->count].offset = offset;
    f->ops[f->count].value = value;
    ++f->count;
    if (kind == STF_SET_OBJ_FIFO_READ) {
        if (reply == NULL || value != 0u ||
            offset < 0x40u || offset > 0x4Cu) return false;
        *reply = UINT32_C(0xD0000000) + ++f->read_count;
    } else if (reply != NULL) {
        return false;
    }
    return true;
}

static void init(fixture *f)
{
    unsigned i;
    memset(f, 0, sizeof(*f));
    f->state.polygon_limit = 100u;
    f->state.polygon = 17u;
    f->state.p2_polygon = 9u;
    f->state.polygon_counter = 33u;
    f->state.previous_polygon = 8u;
    f->standard[2].word[0] = 0xAAu;
    f->standard[2].word[1] = 0xBBu;
    f->standard[2].word[2] = 0xCCu;
    f->standard[2].word[3] = UINT32_C(0x00040005);
    f->extended[1].word[0] = 0xDDu;
    f->extended[1].word[1] = 0xEEu;
    f->extended[1].word[2] = 0xFFu;
    f->extended[1].word[3] = UINT32_C(0x1234FFFF);
    f->model.state = &f->state;
    f->model.object_selector_g0 = 2u;
    f->model.max_poly = 3u;
    f->model.normal = f->standard;
    f->model.normal_count = 4u;
    f->model.extended_max_poly = 3u;
    f->model.extended = f->extended;
    f->model.extended_count = 4u;
    f->model.fifo_offset_g12 = 0x40u;
    f->model.io = io;
    f->model.io_user_data = f;
    for (i = 0; i < STF_SET_OBJ_STATE_WORDS + 2u; ++i)
        f->saved[i] = UINT32_C(0xEEEEEEEE);
}

static bool geowrite(const fixture *f, unsigned i, uint32_t off, uint32_t value)
{
    return i < f->count &&
        f->ops[i].kind == STF_SET_OBJ_GEO_WRITE &&
        f->ops[i].offset == off && f->ops[i].value == value;
}

static int common_and_same(void)
{
    fixture f;
    stf_set_obj_state saved;
    unsigned variant;
    for (variant=0u; variant<2u; ++variant) {
        init(&f);
        if (variant) f.model.object_selector_g0 = UINT32_C(0x8001);
        CHECK(stf_set_obj_common_apply_model2(
                  &f.model, STF_SET_OBJ_COMMON, &f.result), 1);
        CHECK(f.result.submitted && f.result.validated_record &&
              f.result.used_extended == (variant!=0u) &&
              f.result.completed_operations==5u &&
              f.result.geo_writes==5u && f.result.fifo_reads==0u &&
              f.state.polygon_counter==34u &&
              f.state.polygon==(variant?17u+UINT32_C(0xFFFF):22u) &&
              f.state.previous_polygon==8u, 2);
        CHECK(geowrite(&f,0u,0x10u,0x101u) &&
              geowrite(&f,1u,0x40u,variant?0xDDu:0xAAu) &&
              geowrite(&f,2u,0x44u,variant?0xEEu:0xBBu) &&
              geowrite(&f,3u,0x48u,variant?0xFFu:0xCCu) &&
              geowrite(&f,4u,0x4Cu,UINT32_MAX), 3);
    }
    init(&f);
    f.state.polygon = 100u; /* SAME strict below check */
    saved = f.state;
    f.model.io=NULL;
    CHECK(stf_set_obj_common_apply_model2(
              &f.model,STF_SET_OBJ_SAME,&f.result) &&
          f.result.gate_closed && f.count==0u &&
          memcmp(&saved,&f.state,sizeof(saved))==0,4);
    init(&f);
    f.state.polygon = 101u;
    CHECK(stf_set_obj_common_apply_model2(
              &f.model,STF_SET_OBJ_SAME,&f.result) &&
          f.result.gate_closed && f.count==0u,5);
    CHECK(stf_set_obj_common_apply_model2(
              &f.model,STF_SET_OBJ_COMMON,&f.result) &&
          f.state.polygon==106u && f.count==5u,6);
    init(&f);
    CHECK(stf_set_obj_common_apply_model2(
              &f.model,STF_SET_OBJ_SAME,&f.result) &&
          f.result.submitted && f.state.polygon==22u,7);
    return 0;
}

static int common_failures(void)
{
    fixture f;
    stf_set_obj_state previous;
    unsigned stop;
    init(&f);
    f.standard[2].word[0]=0u;
    previous=f.state;
    CHECK(!stf_set_obj_common_apply_model2(
               &f.model,STF_SET_OBJ_COMMON,&f.result) &&
          f.count==0u && memcmp(&previous,&f.state,sizeof(previous))==0,8);
    init(&f);
    f.model.fifo_offset_g12=UINT32_MAX;
    CHECK(!stf_set_obj_common_apply_model2(
              &f.model,STF_SET_OBJ_COMMON,&f.result) && f.count==0u,9);
    init(&f);
    CHECK(!stf_set_obj_common_apply_model2(
              &f.model,(stf_set_obj_state_entry)99,&f.result),10);
    for(stop=1u; stop<=5u; ++stop) {
        init(&f);
        f.fail_at=stop;
        CHECK(!stf_set_obj_common_apply_model2(
                  &f.model,STF_SET_OBJ_COMMON,&f.result),11);
        CHECK(f.result.completed_operations==stop-1u &&
              f.count==stop-1u && !f.result.submitted &&
              f.state.polygon_counter==34u && f.state.polygon==22u,12);
    }
    return 0;
}

static int save_load(void)
{
    fixture f;
    unsigned block, i;
    init(&f);
    CHECK(stf_set_obj_transfer_apply_model2(
              &f.model,STF_SET_OBJ_SAVE,f.saved,12u,&f.transfer),13);
    CHECK(f.transfer.object.submitted &&
          f.transfer.completed_quads==3u &&
          f.transfer.saved_words_committed==12u &&
          f.transfer.g2_bytes_advanced==48u &&
          f.transfer.object.completed_operations==31u &&
          f.transfer.object.fifo_writes==1u &&
          f.transfer.object.fifo_reads==12u &&
          f.transfer.object.geo_writes==18u &&
          f.state.polygon==22u && f.state.polygon_counter==34u &&
          f.saved[12]==UINT32_C(0xEEEEEEEE) &&
          f.saved[13]==UINT32_C(0xEEEEEEEE),14);
    CHECK(geowrite(&f,0u,0xB0u,UINT32_C(0xB0B)) &&
          f.ops[1].kind==STF_SET_OBJ_FIFO_WRITE &&
          f.ops[1].value==UINT32_C(0x02800505) &&
          f.ops[1].offset==0x40u,15);
    for(block=0u;block<3u;++block) {
        for(i=0u;i<4u;++i) {
            const unsigned read_event=2u+block*8u+i;
            const unsigned write_event=read_event+4u;
            const uint32_t word=UINT32_C(0xD0000000)+block*4u+i+1u;
            CHECK(f.ops[read_event].kind==STF_SET_OBJ_FIFO_READ &&
                  f.ops[read_event].offset==0x40u+i*4u &&
                  geowrite(&f,write_event,0x40u+i*4u,word) &&
                  f.saved[block*4u+i]==word,16);
        }
    }
    CHECK(geowrite(&f,26u,0x10u,0x101u) &&
          geowrite(&f,27u,0x40u,0xAAu) &&
          geowrite(&f,30u,0x4Cu,UINT32_MAX),17);

    /* A real save buffer can feed the load path, without device reads. */
    {
        uint32_t snapshot[12];
        memcpy(snapshot,f.saved,sizeof(snapshot));
        init(&f);
        memcpy(f.saved,snapshot,sizeof(snapshot));
    }
    CHECK(stf_set_obj_transfer_apply_model2(
              &f.model,STF_SET_OBJ_LOAD,f.saved,12u,&f.transfer),18);
    CHECK(f.transfer.object.submitted &&
          f.transfer.completed_quads==3u &&
          f.transfer.saved_words_committed==0u &&
          f.transfer.g2_bytes_advanced==48u &&
          f.transfer.object.completed_operations==18u &&
          f.transfer.object.fifo_reads==0u &&
          f.transfer.object.fifo_writes==0u &&
          f.transfer.object.geo_writes==18u &&
          f.state.polygon==22u && f.state.polygon_counter==34u &&
          geowrite(&f,0u,0xB0u,UINT32_C(0xB0B)),19);
    for(block=0u;block<3u;++block)
        for(i=0u;i<4u;++i)
            CHECK(geowrite(&f,1u+block*4u+i,0x40u+i*4u,
                           f.saved[block*4u+i]),20);
    CHECK(geowrite(&f,13u,0x10u,0x101u) &&
          geowrite(&f,14u,0x40u,0xAAu) &&
          geowrite(&f,17u,0x4Cu,UINT32_MAX),21);
    return 0;
}

static int transfer_gates_and_failures(void)
{
    fixture f;
    unsigned stop, i;
    init(&f);
    f.state.polygon=101u;
    f.model.io=NULL;
    CHECK(stf_set_obj_transfer_apply_model2(
              &f.model,STF_SET_OBJ_SAVE,NULL,0u,&f.transfer) &&
          f.transfer.object.gate_closed && f.count==0u,22);

    init(&f);
    CHECK(!stf_set_obj_transfer_apply_model2(
              &f.model,STF_SET_OBJ_SAVE,NULL,12u,&f.transfer) &&
          f.count==0u,23);
    CHECK(!stf_set_obj_transfer_apply_model2(
              &f.model,STF_SET_OBJ_LOAD,f.saved,11u,&f.transfer) &&
          f.count==0u,24);
    CHECK(!stf_set_obj_transfer_apply_model2(
              &f.model,(stf_set_obj_transfer_kind)77u,
              f.saved,12u,&f.transfer) && f.count==0u,25);

    /* Exhaust every SAVE I/O failure boundary. Already submitted FIFO
     * events and fully copied quads stay committed; incomplete quads do not. */
    for(stop=1u;stop<=31u;++stop) {
        unsigned completed=0u;
        init(&f);
        f.fail_at=stop;
        CHECK(!stf_set_obj_transfer_apply_model2(
                  &f.model,STF_SET_OBJ_SAVE,f.saved,12u,&f.transfer),26);
        CHECK(f.count==stop-1u &&
              f.transfer.object.completed_operations==stop-1u &&
              !f.transfer.object.submitted,27);
        if(stop>2u) {
            completed=(stop-3u)/8u;
            if(completed>3u) completed=3u;
        }
        CHECK(f.transfer.completed_quads==completed &&
              f.transfer.saved_words_committed==completed*4u &&
              f.transfer.g2_bytes_advanced==completed*16u,28);
        for(i=completed*4u;i<14u;++i) {
            CHECK(f.saved[i]==UINT32_C(0xEEEEEEEE),29);
        }
    }
    /* LOAD advances g2 after the quad read, before each GEO stq. */
    for(stop=1u;stop<=18u;++stop) {
        init(&f);
        f.fail_at=stop;
        CHECK(!stf_set_obj_transfer_apply_model2(
                  &f.model,STF_SET_OBJ_LOAD,f.saved,12u,&f.transfer),30);
        CHECK(f.count==stop-1u &&
              f.transfer.object.completed_operations==stop-1u &&
              !f.transfer.object.submitted &&
              f.transfer.saved_words_committed==0u,31);
    }
    return 0;
}

int main(void)
{
    int rc=common_and_same();
    if(rc) return rc;
    rc=common_failures();
    if(rc) return rc;
    rc=save_load();
    if(rc) return rc;
    return transfer_gates_and_failures();
}
