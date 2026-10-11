#include <stdint.h>
#include <string.h>

#include "collision_kind.h"
#include "collision_area.h"

enum { F_SIZE=0x1D20u, W_SIZE=0x2B0u };

static void w16(uint8_t *p,uint16_t v)
{
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8u);
}
static void w32(uint8_t *p,uint32_t v)
{
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8u);
    p[2]=(uint8_t)(v>>16u); p[3]=(uint8_t)(v>>24u);
}
static uint32_t r32(const uint8_t *p)
{
    return (uint32_t)p[0]|((uint32_t)p[1]<<8u)|
           ((uint32_t)p[2]<<16u)|((uint32_t)p[3]<<24u);
}
static void setup(uint8_t *a,uint8_t *b,uint8_t *task)
{
    memset(a,0,F_SIZE);memset(b,0,F_SIZE);memset(task,0,W_SIZE);
    b[4]=1u;
}
static int check_kind(const uint8_t *a,const uint8_t *b,
                      const uint8_t *task,stf_collision_kind kind,
                      uint32_t g6mask)
{
    stf_collision_kind_result result;
    return stf_collision_kind_select_model2(
        a,F_SIZE,b,F_SIZE,task,W_SIZE,&result) &&
        result.kind==kind && (result.flags_g6&g6mask)==g6mask;
}
static int select_checks(void)
{
    uint8_t f0[F_SIZE],f1[F_SIZE],task[W_SIZE];
    stf_collision_kind_result result;
    setup(f0,f1,task);
    if(!check_kind(f0,f1,task,STF_COLLISION_KIND_NORMAL,0u)) return 1;

    w32(f0+0x1A4u,UINT32_C(1)<<18u);
    if(!check_kind(f0,f1,task,STF_COLLISION_KIND_NORMAL,1u)) return 2;
    setup(f0,f1,task);
    w32(f0+0x1A4u,UINT32_C(1)<<8u);
    w32(f1+0x1A4u,UINT32_C(1)<<8u);
    if(!check_kind(f0,f1,task,STF_COLLISION_KIND_NORMAL,4u)) return 3;

    /* XOR active bit 8 falls into BODY/KABE; 0.1 threshold and direction. */
    setup(f0,f1,task);
    w32(f0+0x1A4u,UINT32_C(1)<<8u);
    w32(f1+0x658u,UINT32_C(0x3E4CCCCD)); /* 0.2f */
    if(!check_kind(f0,f1,task,STF_COLLISION_KIND_BODY,2u)) return 4;
    w32(f1+0x658u,UINT32_C(0x3DCCCCCD)); /* 0.1f */
    if(!check_kind(f0,f1,task,STF_COLLISION_KIND_KABE,2u)) return 5;
    f0[0x820u]=41u;
    if(!check_kind(f0,f1,task,STF_COLLISION_KIND_NORMAL,0u)) return 6;

    setup(f0,f1,task);
    w32(f1+0x1A4u,UINT32_C(1)<<8u);
    w32(f0+0x65Cu,UINT32_C(0x3E4CCCCD));
    if(!check_kind(f0,f1,task,STF_COLLISION_KIND_BODY_0,2u)) return 7;
    w32(f0+0x65Cu,UINT32_C(0x3DCCCCCD));
    if(!check_kind(f0,f1,task,STF_COLLISION_KIND_KABE_0,2u)) return 8;

    setup(f0,f1,task);
    w32(f0+0x1A4u,UINT32_C(1)<<4u);
    if(!check_kind(f0,f1,task,STF_COLLISION_KIND_JUMP_0,0u)) return 9;
    setup(f0,f1,task);
    w32(f1+0x1A4u,UINT32_C(1)<<4u);
    if(!check_kind(f0,f1,task,STF_COLLISION_KIND_JUMP_1,0u)) return 10;

    setup(f0,f1,task);
    w32(f0+0x1A4u,UINT32_C(1)<<16u);
    if(!check_kind(f0,f1,task,STF_COLLISION_KIND_FIX_0,0u)) return 11;
    w32(f0+0x1A4u,(UINT32_C(1)<<16u)|(UINT32_C(1)<<15u));
    if(!check_kind(f0,f1,task,STF_COLLISION_KIND_RISE_0,0u)) return 12;
    setup(f0,f1,task);
    w32(f1+0x1A4u,UINT32_C(1)<<16u);
    if(!check_kind(f0,f1,task,STF_COLLISION_KIND_FIX_1,0u)) return 13;
    w32(f1+0x1A4u,(UINT32_C(1)<<16u)|(UINT32_C(1)<<15u));
    if(!check_kind(f0,f1,task,STF_COLLISION_KIND_RISE_1,0u)) return 14;

    setup(f0,f1,task);
    w32(f0+0x1A4u,UINT32_C(1)<<16u);
    w32(f1+0x1A4u,UINT32_C(1)<<22u);
    w32(f1+0x804u,UINT32_C(1)<<12u);
    if(!check_kind(f0,f1,task,STF_COLLISION_KIND_RIDE_1,UINT32_C(1)<<6u)) return 15;
    setup(f0,f1,task);
    w32(f1+0x1A4u,UINT32_C(1)<<16u);
    w32(f0+0x1A4u,UINT32_C(1)<<22u);
    w32(f0+0x804u,UINT32_C(1)<<4u);
    if(!check_kind(f0,f1,task,STF_COLLISION_KIND_RIDE_0,UINT32_C(1)<<4u)) return 16;

    setup(f0,f1,task);
    w32(f0+0x70Cu,UINT32_C(1)<<18u);
    w32(f1+0x1A4u,UINT32_C(1)<<3u);
    w16(f0+0x1AAu,4u);
    w32(f1+0x1A4u,(UINT32_C(1)<<3u)|(UINT32_C(1)<<1u));
    if(!check_kind(f0,f1,task,STF_COLLISION_KIND_TOBIKOE_1,0u)) return 17;
    setup(f0,f1,task);
    w32(f1+0x70Cu,UINT32_C(1)<<18u);
    w32(f0+0x1A4u,(UINT32_C(1)<<3u)|(UINT32_C(1)<<1u));
    w16(f1+0x1AAu,4u);
    if(!check_kind(f0,f1,task,STF_COLLISION_KIND_TOBIKOE_0,0u)) return 18;

    setup(f0,f1,task);
    f1[4]=0u;
    if(stf_collision_kind_select_model2(
            f0,F_SIZE,f1,F_SIZE,task,W_SIZE,&result)) return 19;
    f1[4]=1u;
    if(stf_collision_kind_select_model2(
            f0,STF_COLLISION_KIND_FIGHTER_MIN_SIZE-1u,
            f1,F_SIZE,task,W_SIZE,&result) ||
       stf_collision_kind_select_model2(
            f0,F_SIZE,f1,F_SIZE,task,STF_COLLISION_KIND_WORKSPACE_MIN_SIZE-1u,
            &result)) return 20;
    return 0;
}
static void rows_init(uint8_t *a,uint8_t *b)
{
    memset(a,0,256u);memset(b,0,256u);
    a[8u]=12u;a[9u]=20u; /* unit 1 -> balls 3 and 5 */
    a[16u]=16u;          /* unit 2 -> ball 4 */
    a[24u]=0u;           /* unit 3 -> ball 0 from first entry */
    b[8u]=8u;            /* unit 1 -> ball 2 */
    b[16u]=28u;          /* unit 2 -> ball 7 */
}
static bool special(void *context,uint32_t a,uint32_t b,uint32_t *x,uint32_t *y)
{
    unsigned *called=(unsigned*)context;
    ++*called;
    if(a!=1u||b!=2u) return false;
    *x=UINT32_C(0xABCD0001);*y=UINT32_C(0xABCD0002);
    return true;
}
static int map_and_apply_checks(void)
{
    uint8_t a[256u],b[256u],ram[W_SIZE],nonzero[8u];
    stf_collision_kind_profile profiles[STF_COLLISION_KIND_COUNT];
    stf_collision_kind_result result;
    uint32_t value;
    unsigned called=0u;
    memset(ram,0xAB,sizeof(ram));
    memset(profiles,0,sizeof(profiles));
    rows_init(a,b);
    if(!stf_collision_unit_to_ball_model2(
            (UINT32_C(1)<<1u)|(UINT32_C(1)<<3u),a,sizeof(a),&value) ||
       value!=((UINT32_C(1)<<3u)|(UINT32_C(1)<<5u)|1u) ||
       !stf_collision_unit_to_ball_model2(0u,NULL,0u,&value) ||
       value!=0u) return 1;
    if(stf_collision_unit_to_ball_model2(
            UINT32_C(1)<<31u,a,16u,&value)) return 2;
    memset(nonzero,0xFC,sizeof(nonzero));
    if(stf_collision_unit_to_ball_model2(
            1u,nonzero,sizeof(nonzero),&value)) return 3;

    result.kind=STF_COLLISION_KIND_NORMAL;result.flags_g6=0u;
    profiles[STF_COLLISION_KIND_NORMAL].units0=(1u<<1u)|(1u<<3u);
    profiles[STF_COLLISION_KIND_NORMAL].units1=1u<<2u;
    profiles[STF_COLLISION_KIND_NORMAL].units264=1u<<2u;
    profiles[STF_COLLISION_KIND_NORMAL].units268=1u<<1u;
    profiles[STF_COLLISION_KIND_NORMAL].scalar_cc=UINT32_C(0x40000000);
    profiles[STF_COLLISION_KIND_NORMAL].scalar_d0=UINT32_C(0x40800000);
    profiles[STF_COLLISION_KIND_NORMAL].scalar_c8=UINT32_C(0x40A00000);
    profiles[STF_COLLISION_KIND_NORMAL].scalar_88=UINT32_C(0x40C00000);
    if(!stf_collision_kind_apply_model2(
            &result,profiles,STF_COLLISION_KIND_COUNT,
            a,sizeof(a),b,sizeof(b),ram,sizeof(ram),NULL,NULL) ||
       r32(ram+0xC0u)!=~((1u<<3u)|(1u<<5u)|1u) ||
       r32(ram+0xC4u)!=~(1u<<7u) ||
       r32(ram+0x264u)!=(1u<<4u) ||
       r32(ram+0x268u)!=(1u<<2u) ||
       r32(ram+0xCCu)!=UINT32_C(0x40000000) ||
       r32(ram+0xD0u)!=UINT32_C(0x40800000) ||
       r32(ram+0xC8u)!=UINT32_C(0x40A00000) ||
       r32(ram+0x88u)!=UINT32_C(0x40C00000)) return 4;

    profiles[STF_COLLISION_KIND_NORMAL].scalar_cc=UINT32_C(0x411E6666);
    w32(ram+0x27Cu,UINT32_C(0x80000001));
    w32(ram+0x280u,2u);
    w32(ram+0xC0u,UINT32_C(0x12345678));
    if(stf_collision_kind_apply_model2(
            &result,profiles,STF_COLLISION_KIND_COUNT,
            a,sizeof(a),b,sizeof(b),ram,sizeof(ram),NULL,NULL) ||
       r32(ram+0xC0u)!=UINT32_C(0x12345678)) return 5;
    if(!stf_collision_kind_apply_model2(
            &result,profiles,STF_COLLISION_KIND_COUNT,
            a,sizeof(a),b,sizeof(b),ram,sizeof(ram),special,&called) ||
       called!=1u || r32(ram+0xCCu)!=UINT32_C(0xABCD0001) ||
       r32(ram+0xD0u)!=UINT32_C(0xABCD0002)) return 6;
    result.kind=STF_COLLISION_KIND_COUNT;
    if(stf_collision_kind_apply_model2(
            &result,profiles,STF_COLLISION_KIND_COUNT,
            a,sizeof(a),b,sizeof(b),ram,sizeof(ram),special,&called)) return 7;
    return 0;
}
typedef struct fixture {uint32_t calls;} fixture;
static bool geo(void *context,uint8_t index,const stf_collision_area_query *query,
                uint32_t response[STF_COLLISION_AREA_RESPONSE_WORDS])
{
    fixture *f=(fixture*)context;
    unsigned i;
    if(query==NULL||query->words[0]!=UINT32_C(0x38007070)||index>1u)
       return false;
    ++f->calls;
    for(i=0u;i<STF_COLLISION_AREA_RESPONSE_WORDS;++i) response[i]=0u;
    return true;
}
static int integration_checks(void)
{
    uint8_t a[F_SIZE],b[F_SIZE],ram[W_SIZE],row0[256u],row1[256u];
    stf_collision_kind_profile profiles[STF_COLLISION_KIND_COUNT];
    stf_copro_attack_rom_view rom={0};
    stf_collision_copro_area_model2 model;
    stf_collision_copro_area_result out;
    fixture f={0};
    setup(a,b,ram);
    rows_init(row0,row1);memset(profiles,0,sizeof(profiles));
    memset(&model,0,sizeof(model));
    model.fighter[0]=a;model.fighter[1]=b;
    model.fighter_size[0]=sizeof(a);model.fighter_size[1]=sizeof(b);
    model.workspace=ram;model.workspace_size=sizeof(ram);
    model.rom_view=&rom;model.geometry=geo;model.geometry_user_data=&f;
    model.kind_profiles=profiles;model.kind_profile_count=STF_COLLISION_KIND_COUNT;
    model.unit_rows[0]=row0;model.unit_rows_sizes[0]=sizeof(row0);
    model.unit_rows[1]=row1;model.unit_rows_sizes[1]=sizeof(row1);
    profiles[STF_COLLISION_KIND_JUMP_0].units0=1u<<1u;
    profiles[STF_COLLISION_KIND_JUMP_0].units1=1u<<2u;
    w32(a+0x1A4u,1u<<4u);
    if(!stf_collision_copro_area_stage_model2(&model,&out) ||
       f.calls!=2u||!out.completed_kind||
       out.kind.kind!=STF_COLLISION_KIND_JUMP_0||
       r32(ram+0xC0u)!=~((1u<<3u)|(1u<<5u))||
       r32(ram+0xC4u)!=~(1u<<7u)) return 1;

    /* Invalid stage length prevents the initial RAM reset and queries. */
    f.calls=0u;
    model.kind_profile_count=2u;
    ram[0x170u]=0xBDu;
    if(stf_collision_copro_area_stage_model2(&model,&out)||
       f.calls!=0u||ram[0x170u]!=0xBDu) return 2;
    model.kind_profile_count=STF_COLLISION_KIND_COUNT;
    model.kind_profiles=NULL;
    if(!stf_collision_copro_area_stage_model2(&model,&out)||
       out.completed_kind||f.calls!=2u) return 3;
    return 0;
}
int main(void)
{
    int rc=select_checks();
    if(rc!=0) return rc;
    rc=map_and_apply_checks();
    if(rc!=0) return 100+rc;
    rc=integration_checks();
    if(rc!=0) return 200+rc;
    return 0;
}
