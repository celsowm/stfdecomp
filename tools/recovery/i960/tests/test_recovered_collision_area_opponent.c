#include <stdint.h>
#include <string.h>

#include "collision_area_opponent.h"
#include "collision_area.h"

enum { F_SIZE=0x1D20u, W_SIZE=0x2B0u };
static const uint16_t kind8[8] = {0u,8u,8u,8u,8u,0u,0u,0u};
static const uint16_t kind16[16] = {
    0u,1u,2u,3u,2u,3u,1u,0u,1u,0u,3u,2u,3u,2u,0u,1u
};
static const uint8_t angle16[16] = {
    7u,5u,3u,1u,0u,2u,4u,6u,
    6u,4u,2u,0u,1u,3u,5u,7u
};
static void w32(uint8_t *p,uint32_t v)
{
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8u);
    p[2]=(uint8_t)(v>>16u);p[3]=(uint8_t)(v>>24u);
}
static uint32_t r32(const uint8_t *p)
{
    return (uint32_t)p[0]|((uint32_t)p[1]<<8u)|
        ((uint32_t)p[2]<<16u)|((uint32_t)p[3]<<24u);
}
static uint16_t r16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8u));
}
static uint32_t fb(float value)
{
    uint32_t bits;
    memcpy(&bits,&value,sizeof(bits));
    return bits;
}
static void planes(uint8_t *fighter,float a,float b,float c,float d)
{
    w32(fighter+0x650u,fb(a));
    w32(fighter+0x654u,fb(b));
    w32(fighter+0x658u,fb(c));
    w32(fighter+0x65Cu,fb(d));
}
static bool process(
    const uint8_t *f, size_t fn,
    const uint8_t *opp, size_t on,
    uint8_t *ram,size_t rn,
    const uint16_t *a,size_t an,const uint16_t *b,size_t bn,
    stf_collision_area_opponent_result *result
)
{
    return stf_collision_area_opponent_apply_model2(
        f,fn,opp,on,ram,rn,a,an,b,bn,result);
}
static int direct_tests(void)
{
    uint8_t f0[F_SIZE]={0u},f1[F_SIZE]={0u},ram[W_SIZE];
    stf_collision_area_opponent_result out;
    memset(ram,0xEE,sizeof(ram));f1[4u]=1u;
    planes(f1,4.0f,1.0f,3.0f,2.0f);
    w32(ram+0x1E0u,UINT32_C(0x2000));
    if(!process(f0,sizeof(f0),f1,sizeof(f1),ram,sizeof(ram),
                kind8,8u,kind16,16u,&out) ||
        out.fighter_index!=0u || out.opponent_index!=1u ||
        out.minimum_plane_index!=1u ||
        out.minimum_plane_bits!=fb(1.0f) ||
        out.angle_table_index!=1u ||
        out.raw_direction_index!=1u || out.table_2e230_index!=13u ||
        out.kind_200!=2u ||
        r32(ram+0x204u)!=fb(1.0f) || r16(ram+0x1FCu)!=13u ||
        r16(ram+0x200u)!=2u ||
        r32(ram+0x208u)!=UINT32_C(0xEEEEEEEE)) return 1;

    planes(f0,2.0f,3.0f,1.0f,4.0f);
    w32(ram+0x1E0u,0u);
    if(!process(f1,sizeof(f1),f0,sizeof(f0),ram,sizeof(ram),
                kind8,8u,kind16,16u,&out) ||
        out.minimum_plane_index!=2u || out.raw_direction_index!=3u ||
        out.table_2e230_index!=3u || out.kind_200!=3u ||
        out.minimum_plane_bits!=fb(1.0f) ||
        r32(ram+0x208u)!=fb(1.0f) ||
        r16(ram+0x1FEu)!=3u || r16(ram+0x202u)!=3u ||
        r16(ram+0x1FCu)!=13u || r16(ram+0x200u)!=2u) return 2;

    /* Tie selects the latest candidate, and fighter1 flips its low bit. */
    planes(f0,1.0f,1.0f,1.0f,1.0f);
    if(!process(f1,sizeof(f1),f0,sizeof(f0),ram,sizeof(ram),
                kind8,8u,kind16,16u,&out) ||
        out.minimum_plane_index!=3u ||
        out.raw_direction_index!=2u ||
        out.table_2e230_index!=2u || out.kind_200!=2u) return 3;

    /* Non-finite planes fail without modifying result buffers. */
    planes(f0,1.0f,1.0f,1.0f,1.0f);
    w32(f0+0x658u,UINT32_C(0x7FC00000));
    w32(ram+0x208u,UINT32_C(0x12345678));
    if(process(f1,sizeof(f1),f0,sizeof(f0),ram,sizeof(ram),
               kind8,8u,kind16,16u,&out) ||
       r32(ram+0x208u)!=UINT32_C(0x12345678)) return 4;
    planes(f0,1.0f,2.0f,3.0f,4.0f);

    /* Bounds checks all table sources and RAM before stores. */
    if(process(f1,sizeof(f1),f0,sizeof(f0),ram,0x20Bu,
               kind8,8u,kind16,16u,&out) ||
       process(f1,sizeof(f1),f0,0x65Fu,ram,sizeof(ram),
               kind8,8u,kind16,16u,&out) ||
       process(f1,sizeof(f1),f0,sizeof(f0),ram,sizeof(ram),
               kind8,7u,kind16,16u,&out) ||
       process(f1,sizeof(f1),f0,sizeof(f0),ram,sizeof(ram),
               kind8,8u,kind16,15u,&out) ||
       process(f0,sizeof(f0),f0,sizeof(f0),ram,sizeof(ram),
               kind8,8u,kind16,16u,&out)) return 5;

    w32(ram+0x1E0u,UINT32_C(0x10000));
    if(process(f1,sizeof(f1),f0,sizeof(f0),ram,sizeof(ram),
               kind8,8u,kind16,16u,&out) ||
       r32(ram+0x208u)!=UINT32_C(0x12345678)) return 6;
    w32(ram+0x1E0u,0u);
    {
        uint16_t malformed[8]={0u};
        malformed[0]=UINT16_C(32);
        if(process(f1,sizeof(f1),f0,sizeof(f0),ram,sizeof(ram),
                   malformed,8u,kind16,16u,&out) ||
           r32(ram+0x208u)!=UINT32_C(0x12345678)) return 7;
    }
    return 0;
}

typedef struct events {uint8_t count,sequence[4];} events;
static bool geometry(
    void *user,uint8_t slot,const stf_collision_area_query *query,
    uint32_t response[STF_COLLISION_AREA_RESPONSE_WORDS]
)
{
    events *ev=(events*)user;
    unsigned i;
    if(ev->count>=4u||slot>1u||query==NULL||
       query->words[0]!=UINT32_C(0x38007070)) return false;
    ev->sequence[ev->count++]=(uint8_t)(10u+slot);
    for(i=0u;i<STF_COLLISION_AREA_RESPONSE_WORDS;++i) response[i]=0u;
    response[14u]=fb(slot==0u?5.0f:7.0f);
    response[15u]=fb(slot==0u?2.0f:6.0f);
    response[16u]=fb(slot==0u?3.0f:5.0f);
    response[17u]=fb(4.0f);
    return true;
}
static bool distance(
    void *user,uint8_t slot,const stf_collision_area_distance_query *query,
    uint32_t *response_bits
)
{
    events *ev=(events*)user;
    if(ev->count>=4u||slot>1u||query==NULL||
       query->words[0]!=UINT32_C(0x15802B2B)) return false;
    ev->sequence[ev->count++]=(uint8_t)(20u+slot);
    return stf_collision_area_distance_semantic_bits(query,response_bits);
}

static int integration_tests(void)
{
    uint8_t f0[F_SIZE]={0u},f1[F_SIZE]={0u},ram[W_SIZE];
    stf_copro_attack_rom_view view={0};
    stf_collision_copro_area_model2 model;
    stf_collision_copro_area_result result;
    events ev={0};

    memset(&model,0,sizeof(model));memset(ram,0xCD,sizeof(ram));
    f1[4u]=1u;
    f0[0x1A18u]=2u;f1[0x1A18u]=2u;
    planes(f0,90.0f,90.0f,90.0f,90.0f);
    planes(f1,4.0f,1.0f,3.0f,2.0f);
    w32(ram+0x1E0u,UINT32_C(0x2000));
    model.fighter[0]=f0;model.fighter_size[0]=sizeof(f0);
    model.fighter[1]=f1;model.fighter_size[1]=sizeof(f1);
    model.workspace=ram;model.workspace_size=sizeof(ram);
    model.rom_view=&view;model.cage_height_bits=fb(12.0f);
    model.geometry=geometry;model.geometry_user_data=&ev;
    model.distance=distance;model.distance_user_data=&ev;
    model.angle_lut=angle16;model.angle_lut_size=sizeof(angle16);
    model.table_2e250=kind8;model.table_2e250_count=8u;
    model.table_2e230=kind16;model.table_2e230_count=16u;

    /* Static bad tables fail before any mutation/geometry callbacks. */
    model.table_2e250_count=7u;
    if(stf_collision_copro_area_stage_model2(&model,&result) ||
       ev.count!=0u || ram[0x170u]!=0xCDu) return 1;
    model.table_2e250_count=8u;

    if(!stf_collision_copro_area_stage_model2(&model,&result) ||
       result.completed_areas!=2u || result.completed_distances!=2u ||
       result.completed_sides!=2u || result.completed_opponents!=2u ||
       ev.count!=4u ||
       ev.sequence[0]!=10u || ev.sequence[1]!=20u ||
       ev.sequence[2]!=11u || ev.sequence[3]!=21u ||
       result.side[0].applied || result.side[1].applied ||
       result.opponent[0].minimum_plane_index!=1u ||
       result.opponent[1].minimum_plane_index!=1u ||
       result.opponent[0].minimum_plane_bits!=fb(1.0f) ||
       result.opponent[1].minimum_plane_bits!=fb(2.0f) ||
       result.opponent[0].table_2e230_index!=13u ||
       result.opponent[1].table_2e230_index!=12u ||
       r32(ram+0x204u)!=fb(1.0f) ||
       r32(ram+0x208u)!=fb(2.0f) ||
       r16(ram+0x1FCu)!=13u || r16(ram+0x1FEu)!=12u ||
       r16(ram+0x200u)!=2u || r16(ram+0x202u)!=3u)
        return 2;

    /* Without tables the original optional prefix is unchanged. */
    ev.count=0u;
    model.table_2e250=NULL;model.table_2e230=NULL;
    if(!stf_collision_copro_area_stage_model2(&model,&result) ||
       ev.count!=4u || result.completed_opponents!=0u) return 3;

    return 0;
}
int main(void)
{
    int rc=direct_tests();
    if(rc!=0) return rc;
    rc=integration_tests();
    if(rc!=0) return 100+rc;
    return 0;
}
