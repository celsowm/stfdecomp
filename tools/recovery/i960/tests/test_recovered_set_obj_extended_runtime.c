#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "set_obj_extended_runtime.h"

#define CHECK(expr,id) do { if (!(expr)) { \
    fprintf(stderr,"STF object FIFO/mirror assertion %d line %d\n",(id),__LINE__); \
    return (id); } } while (0)

typedef struct event {
    stf_set_obj_io_kind kind;
    uint32_t offset;
    uint32_t value;
} event;

typedef struct fixture {
    stf_set_obj_state state;
    stf_set_obj_record standard[4], extended[4];
    stf_set_obj_model2 model;
    stf_set_obj_result mirror_result;
    stf_set_obj_fifo_context context;
    stf_set_obj_fifo_result fifo_result;
    uint32_t g3[14];
    uint32_t replies[12];
    unsigned replies_used;
    uint32_t reply_other;
    uint32_t geo_pointer;
    unsigned fail_at;
    unsigned count;
    event log[48];
} fixture;

static bool mock_io(void *opaque, stf_set_obj_io_kind kind,
                    uint32_t offset, uint32_t value, uint32_t *reply)
{
    fixture *f=(fixture *)opaque;
    if (f->count >= 48u ||
        (f->fail_at && f->count+1u==f->fail_at))
        return false;
    if ((kind==STF_SET_OBJ_FIFO_READ || kind==STF_SET_OBJ_GEO_READ) !=
        (reply!=NULL))
        return false;
    f->log[f->count].kind=kind;
    f->log[f->count].offset=offset;
    f->log[f->count].value=value;
    ++f->count;
    if (kind==STF_SET_OBJ_GEO_READ) {
        if (offset!=UINT32_C(0x2008)) return false;
        *reply=f->geo_pointer;
    } else if (kind==STF_SET_OBJ_FIFO_READ) {
        if (f->replies_used<12u)
            *reply=f->replies[f->replies_used++];
        else
            *reply=f->reply_other;
    }
    return true;
}

static void init(fixture *f)
{
    unsigned i;
    memset(f,0,sizeof(*f));
    f->state.polygon_limit=100u;
    f->state.polygon=17u;
    f->state.p2_polygon=7u;
    f->state.polygon_counter=33u;
    f->state.previous_polygon=44u;
    f->standard[2].word[0]=UINT32_C(0xAABBCCDD);
    f->standard[2].word[1]=UINT32_C(0x10111213);
    f->standard[2].word[2]=UINT32_C(0x20212223);
    f->standard[2].word[3]=UINT32_C(0x00040005);
    f->extended[1].word[0]=UINT32_C(0xEEFF0011);
    f->extended[1].word[1]=UINT32_C(0x30313233);
    f->extended[1].word[2]=UINT32_C(0x40414243);
    f->extended[1].word[3]=UINT32_C(0x12340006);
    f->model.state=&f->state;
    f->model.object_selector_g0=2u;
    f->model.normal=f->standard;
    f->model.normal_count=4u;
    f->model.max_poly=3u;
    f->model.extended=f->extended;
    f->model.extended_count=4u;
    f->model.extended_max_poly=3u;
    f->model.fifo_offset_g12=0x40u;
    f->model.io=mock_io;
    f->model.io_user_data=f;
    f->geo_pointer=UINT32_C(0x7000);
    f->reply_other=UINT32_C(0x99887766);
    f->replies[0]=UINT32_C(0x3F800000); /* 1.0 exactly */
    for(i=1u;i<11u;++i) f->replies[i]=UINT32_C(0x10000000)+i;
    f->replies[11]=UINT32_C(0x3E4CCCCD); /* 0.2 exactly */
    for(i=0u;i<14u;++i) f->g3[i]=UINT32_C(0xDEADBEEF);
    f->context.input_g2=0u;
    f->context.g3_words=f->g3;
    f->context.g3_word_capacity=12u;
}

static bool event_is(const fixture *f,unsigned index,stf_set_obj_io_kind kind,
                     uint32_t offset,uint32_t value)
{
    return index<f->count && f->log[index].kind==kind &&
           f->log[index].offset==offset &&
           f->log[index].value==value;
}

static int mirror_paths(void)
{
    fixture f;
    unsigned variant;
    for(variant=0u;variant<2u;++variant) {
        const uint32_t *expected;
        unsigned i;
        init(&f);
        if(variant) f.model.object_selector_g0=UINT32_C(0x8001);
        expected=variant?f.extended[1].word:f.standard[2].word;
        CHECK(stf_set_obj_mirror_apply_model2(&f.model,&f.mirror_result),1);
        CHECK(f.mirror_result.submitted &&
              f.mirror_result.validated_record &&
              f.mirror_result.used_extended==(variant!=0u) &&
              f.mirror_result.selected_index==(variant?1u:2u) &&
              f.mirror_result.selected_table_address==
                (variant?UINT32_C(0x0647F014):UINT32_C(0x020E0024)) &&
              f.mirror_result.completed_operations==11u &&
              f.mirror_result.geo_writes==6u &&
              f.mirror_result.geo_reads==1u &&
              f.mirror_result.fifo_writes==2u &&
              f.mirror_result.fifo_reads==1u &&
              f.state.polygon==(variant?23u:22u) &&
              f.state.polygon_counter==34u &&
              f.state.p2_polygon==7u && f.state.previous_polygon==44u,2);
        CHECK(event_is(&f,0u,STF_SET_OBJ_GEO_WRITE,0xB0u,0xB0Bu) &&
              event_is(&f,1u,STF_SET_OBJ_FIFO_WRITE,0x40u,UINT32_C(0x1A003434)) &&
              event_is(&f,2u,STF_SET_OBJ_GEO_READ,0x2008u,0u) &&
              event_is(&f,3u,STF_SET_OBJ_GEO_WRITE,0x1008u,0x7030u) &&
              event_is(&f,4u,STF_SET_OBJ_FIFO_WRITE,0x40u,0x7000u) &&
              event_is(&f,5u,STF_SET_OBJ_FIFO_READ,0x40u,0u) &&
              event_is(&f,6u,STF_SET_OBJ_GEO_WRITE,0x110u,0x1111u),3);
        for(i=0u;i<4u;++i)
            CHECK(event_is(&f,7u+i,STF_SET_OBJ_GEO_WRITE,
                           0x40u+i*4u,i==3u?UINT32_MAX:expected[i]),4);
    }
    return 0;
}

static int mirror_guards_and_faults(void)
{
    fixture f;
    unsigned stop;
    stf_set_obj_state before;
    init(&f);
    f.standard[2].word[0]=0u;
    f.state.polygon=101u;
    before=f.state;
    CHECK(!stf_set_obj_mirror_apply_model2(&f.model,&f.mirror_result) &&
          f.count==0u && memcmp(&before,&f.state,sizeof(before))==0,5);
    init(&f);
    f.state.polygon=101u;
    f.model.io=NULL;
    CHECK(stf_set_obj_mirror_apply_model2(&f.model,&f.mirror_result) &&
          f.mirror_result.gate_closed && f.mirror_result.validated_record &&
          !f.mirror_result.submitted && f.count==0u &&
          f.state.polygon_counter==33u,6);
    init(&f);
    f.model.fifo_offset_g12=UINT32_MAX;
    CHECK(!stf_set_obj_mirror_apply_model2(&f.model,&f.mirror_result) &&
          f.count==0u && f.state.polygon_counter==33u,7);
    for(stop=1u;stop<=11u;++stop) {
        init(&f);
        f.fail_at=stop;
        CHECK(!stf_set_obj_mirror_apply_model2(&f.model,&f.mirror_result),8);
        CHECK(f.count==stop-1u &&
              f.mirror_result.completed_operations==stop-1u &&
              !f.mirror_result.submitted &&
              f.state.polygon_counter==34u && f.state.polygon==22u,9);
    }
    return 0;
}

static int fifo_tail_paths(void)
{
    fixture f;
    unsigned variant, block, word;
    for(variant=0u;variant<4u;++variant) {
        uint32_t *rec;
        stf_set_obj_fifo_tail expected_tail;
        uint32_t expected_g2=0u;
        init(&f);
        if(variant==1u) {
            f.context.g7_flags_1a4=UINT32_C(1)<<26;
            f.context.g7_short_1aa=4u;
            expected_tail=STF_SET_OBJ_FIFO_THD_FLAG26_TAIL;
            expected_g2=UINT32_C(0x800000);
        } else if(variant==2u) {
            f.context.g7_flags_70c=5u;
            f.context.g7_short_1aa=4u;
            expected_tail=STF_SET_OBJ_FIFO_THD_FLAG2_TAIL;
            expected_g2=UINT32_C(0x802000);
        } else if(variant==3u) {
            f.model.object_selector_g0=UINT32_C(0x8001);
            f.context.g7_flags_1a4=UINT32_C(1)<<26;
            f.context.g7_flags_70c=5u;
            f.context.g7_short_1aa=4u;
            expected_tail=STF_SET_OBJ_FIFO_THD_FLAG26_TAIL;
            expected_g2=UINT32_C(0x800000);
        } else {
            expected_tail=STF_SET_OBJ_FIFO_COMMON_TAIL;
        }
        rec=(f.model.object_selector_g0 & 0x8000u) ?
              f.extended[1].word : f.standard[2].word;
        CHECK(stf_set_obj_fifo_apply_model2(&f.model,&f.context,&f.fifo_result),10);
        CHECK(f.fifo_result.object.submitted && !f.fifo_result.returned_early &&
              !f.fifo_result.invalid_record && f.fifo_result.tail==expected_tail &&
              f.fifo_result.selected_g2==expected_g2 &&
              f.fifo_result.quads_read==3u && f.fifo_result.quads_saved==3u &&
              f.fifo_result.words_saved==12u &&
              f.fifo_result.g3_bytes_advanced==48u &&
              f.fifo_result.g3_bytes_rewound==0u &&
              f.fifo_result.object.completed_operations==30u &&
              f.fifo_result.object.fifo_reads==12u &&
              f.fifo_result.object.geo_writes==18u &&
              f.state.polygon_counter==34u &&
              f.state.polygon==((variant==3u)?23u:22u) &&
              f.state.previous_polygon==44u &&
              f.state.p2_polygon==7u,11);
        for(block=0u;block<3u;++block) {
            for(word=0u;word<4u;++word) {
                const unsigned i=block*4u+word;
                CHECK(event_is(&f,i,STF_SET_OBJ_FIFO_READ,
                       0x40u+word*4u,0u),12);
                CHECK(event_is(&f,13u+i,STF_SET_OBJ_GEO_WRITE,
                       0x40u+word*4u,f.replies[i]) &&
                      f.g3[i]==f.replies[i],13);
            }
        }
        CHECK(event_is(&f,12u,STF_SET_OBJ_GEO_WRITE,0xB0u,0xB0Bu) &&
              event_is(&f,25u,STF_SET_OBJ_GEO_WRITE,0x10u,0x101u),14);
        for(word=0u;word<4u;++word)
            CHECK(event_is(&f,26u+word,STF_SET_OBJ_GEO_WRITE,
                  0x40u+word*4u,
                  word==3u?UINT32_MAX:
                  (word==1u && expected_tail!=STF_SET_OBJ_FIFO_COMMON_TAIL?
                   expected_g2:rec[word])),15);
    }
    return 0;
}

static int fifo_gates(void)
{
    fixture f;
    unsigned gate;
    for(gate=0u;gate<4u;++gate) {
        unsigned i;
        init(&f);
        if(gate==0u) f.context.input_g2=1u;
        if(gate==1u) f.state.polygon=101u;
        if(gate==2u) f.replies[0]=UINT32_C(0x3F7FFFFF);
        if(gate==3u) f.replies[11]=UINT32_C(0x3E4CCCCE);
        CHECK(stf_set_obj_fifo_apply_model2(&f.model,&f.context,&f.fifo_result),16);
        CHECK(f.fifo_result.returned_early &&
              f.fifo_result.object.gate_closed &&
              !f.fifo_result.object.submitted &&
              f.fifo_result.quads_read==3u &&
              f.fifo_result.quads_saved==0u &&
              f.fifo_result.object.fifo_reads==12u &&
              f.fifo_result.object.geo_writes==0u &&
              f.count==12u && f.state.polygon_counter==33u,17);
        for(i=0u;i<14u;++i)
            CHECK(f.g3[i]==UINT32_C(0xDEADBEEF),18);
    }
    init(&f);
    f.replies[0]=UINT32_C(0xBF800000); /* sign bit is cleared */
    f.replies[11]=UINT32_C(0xBE4CCCCD);
    CHECK(stf_set_obj_fifo_apply_model2(&f.model,&f.context,&f.fifo_result) &&
          f.fifo_result.object.submitted,19);
    return 0;
}

static int fifo_invalid_after_bus_writes(void)
{
    fixture f;
    unsigned i;
    init(&f);
    f.standard[2].word[0]=0u; /* invalid descriptor after 25 events */
    CHECK(!stf_set_obj_fifo_apply_model2(&f.model,&f.context,&f.fifo_result),20);
    CHECK(f.fifo_result.invalid_record &&
          f.fifo_result.error_handler_unimplemented &&
          !f.fifo_result.object.submitted &&
          f.count==25u &&
          f.fifo_result.object.completed_operations==25u &&
          f.fifo_result.quads_saved==3u &&
          f.fifo_result.g3_bytes_advanced==48u &&
          f.fifo_result.g3_bytes_rewound==48u &&
          f.state.polygon_counter==33u && f.state.polygon==17u,21);
    for(i=0u;i<12u;++i) CHECK(f.g3[i]==f.replies[i],22);
    return 0;
}

static int fifo_fault_boundaries(void)
{
    fixture f;
    unsigned stop;
    for(stop=1u;stop<=30u;++stop) {
        unsigned completed=0u;
        unsigned i;
        init(&f);
        f.fail_at=stop;
        CHECK(!stf_set_obj_fifo_apply_model2(
                  &f.model,&f.context,&f.fifo_result),23);
        CHECK(f.count==stop-1u &&
              f.fifo_result.object.completed_operations==stop-1u &&
              !f.fifo_result.object.submitted,24);
        if(stop>=18u) ++completed;
        if(stop>=22u) ++completed;
        if(stop>=26u) ++completed;
        CHECK(f.fifo_result.quads_saved==completed &&
              f.fifo_result.words_saved==4u*completed &&
              f.fifo_result.g3_bytes_advanced==16u*completed,25);
        CHECK(f.state.polygon_counter==(stop>=26u?34u:33u) &&
              f.state.polygon==(stop>=26u?22u:17u),26);
        for(i=0u;i<14u;++i)
            CHECK(f.g3[i]==(i<4u*completed?f.replies[i]:
                          UINT32_C(0xDEADBEEF)),27);
    }
    return 0;
}

int main(void)
{
    int status=mirror_paths();
    if(status) return status;
    status=mirror_guards_and_faults();
    if(status) return status;
    status=fifo_tail_paths();
    if(status) return status;
    status=fifo_gates();
    if(status) return status;
    status=fifo_invalid_after_bus_writes();
    if(status) return status;
    return fifo_fault_boundaries();
}
