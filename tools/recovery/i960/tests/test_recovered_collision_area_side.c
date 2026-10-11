#include <stdint.h>
#include <string.h>

#include "collision_area_side.h"
#include "collision_area.h"

enum { F_SIZE=0x1D20u, W_SIZE=0x2B0u };
static const uint8_t angles[16] = {
    7u,5u,3u,1u,0u,2u,4u,6u,
    6u,4u,2u,0u,1u,3u,5u,7u
};
static void w16(uint8_t *p,uint16_t v)
{
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8u);
}
static uint16_t r16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8u));
}
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
static uint32_t fb(float value)
{
    uint32_t bits;
    memcpy(&bits,&value,sizeof(bits));
    return bits;
}
static void four_planes(uint8_t *fighter)
{
    w32(fighter+0x650u,fb(4.0f));
    w32(fighter+0x654u,fb(1.0f));
    w32(fighter+0x658u,fb(3.0f));
    w32(fighter+0x65Cu,fb(2.0f));
}

static int direct_side_tests(void)
{
    uint8_t fighter[F_SIZE]={0u},workspace[W_SIZE];
    stf_collision_area_side_result result;
    memset(workspace,0xCD,sizeof(workspace));
    four_planes(fighter);
    w32(fighter+0x644u,fb(2.0f));
    w32(fighter+0x64Cu,fb(1.0f));
    w16(fighter+0x26u,UINT16_C(0x1000));

    /* Positive X: LUT[1]=5, minimum is second record (offset -8). */
    if(!stf_collision_area_side_apply_model2(
            fighter,sizeof(fighter),workspace,sizeof(workspace),
            angles,sizeof(angles),&result) ||
        !result.applied || result.plane!=STF_COLLISION_AREA_PLANE_X ||
        result.lut_index!=1u || result.orientation_code!=5u ||
        result.minimum_record_offset!=1 ||
        result.min_plane_index!=1u || result.minimum_bits!=fb(1.0f) ||
        result.selected_1f8!=UINT16_C(0x0015) ||
        r16(workspace+0x1F0u)!=UINT16_C(0x1000) ||
        r16(workspace+0x1F4u)!=5u ||
        r16(workspace+0x1F8u)!=UINT16_C(0x0015) ||
        r16(workspace+0x23Cu)!=2u ||
        r16(fighter+0xAE2u)!=5u) return 1;

    /* Negative X toggles bit3; LUT[9]=4; high flag bit 11 stays set. */
    w32(fighter+0x644u,fb(-3.0f));
    if(!stf_collision_area_side_apply_model2(
            fighter,sizeof(fighter),workspace,sizeof(workspace),
            angles,sizeof(angles),&result) ||
        result.plane!=STF_COLLISION_AREA_PLANE_X ||
        result.lut_index!=9u || result.orientation_code!=12u ||
        r16(fighter+0xAE2u)!=4u ||
        (r32(fighter+0xA08u)&UINT32_C(0x800))==0u ||
        r16(workspace+0x1F8u)!=UINT16_C(0x0014)) return 2;

    /* Negative Z with quarter-turn subtraction: signed index 10. */
    w32(fighter+0x644u,fb(1.0f));
    w32(fighter+0x64Cu,fb(-3.0f));
    w16(fighter+0x26u,UINT16_C(0x2000));
    if(!stf_collision_area_side_apply_model2(
            fighter,sizeof(fighter),workspace,sizeof(workspace),
            angles,sizeof(angles),&result) ||
        result.plane!=STF_COLLISION_AREA_PLANE_Z ||
        result.lut_index!=10u || result.orientation_code!=10u ||
        r16(fighter+0xAE2u)!=2u ||
        r16(workspace+0x1F4u)!=10u ||
        r16(workspace+0x1F8u)!=UINT16_C(0x0010)) return 3;

    /* Ties in minimum candidate float select the *last* one. */
    four_planes(fighter);
    w32(fighter+0x650u,fb(1.0f));
    w32(fighter+0x658u,fb(1.0f));
    w32(fighter+0x65Cu,fb(1.0f));
    if(!stf_collision_area_side_apply_model2(
            fighter,sizeof(fighter),workspace,sizeof(workspace),
            angles,sizeof(angles),&result) ||
        result.min_plane_index!=3u ||
        result.minimum_record_offset!=3 ||
        result.selected_1f8!=UINT16_C(0x0030)) return 4;

    /* Two-zero branch cannot invent an angle, and must not modify state. */
    w32(fighter+0x644u,0u);w32(fighter+0x64Cu,0u);
    w32(fighter+0xA08u,UINT32_C(0x11223344));
    w16(workspace+0x23Cu,UINT16_C(0x5678));
    if(!stf_collision_area_side_apply_model2(
            fighter,sizeof(fighter),workspace,sizeof(workspace),
            angles,sizeof(angles),&result) ||
        result.applied || result.plane!=STF_COLLISION_AREA_PLANE_NONE ||
        r32(fighter+0xA08u)!=UINT32_C(0x11223344) ||
        r16(workspace+0x23Cu)!=UINT16_C(0x5678)) return 5;

    /* Input failures and nonfinite candidate must be nonmutating. */
    w32(fighter+0x644u,fb(2.0f));
    w32(fighter+0x64Cu,fb(1.0f));
    w32(fighter+0x658u,UINT32_C(0x7FC00000));
    w32(fighter+0xA08u,UINT32_C(0x12345678));
    if(stf_collision_area_side_apply_model2(
            fighter,sizeof(fighter),workspace,sizeof(workspace),
            angles,sizeof(angles),&result) ||
        r32(fighter+0xA08u)!=UINT32_C(0x12345678)) return 6;
    four_planes(fighter);
    if(stf_collision_area_side_apply_model2(
            fighter,sizeof(fighter),workspace,sizeof(workspace),
            angles,15u,&result) ||
        stf_collision_area_side_apply_model2(
            fighter,sizeof(fighter),workspace,0x23Fu,
            angles,sizeof(angles),&result) ||
        stf_collision_area_side_apply_model2(
            fighter,0xAE3u,workspace,sizeof(workspace),
            angles,sizeof(angles),&result) ||
        stf_collision_area_side_apply_model2(
            fighter,sizeof(fighter),workspace,sizeof(workspace),
            angles,sizeof(angles),NULL)) return 7;

    /* Slot 1 writes the second halfwords, not the slot-0 state. */
    memset(workspace,0xDE,sizeof(workspace));
    fighter[4u]=1u;
    w32(fighter+0x644u,fb(2.0f));
    w32(fighter+0x64Cu,fb(1.0f));
    if(!stf_collision_area_side_apply_model2(
            fighter,sizeof(fighter),workspace,sizeof(workspace),
            angles,sizeof(angles),&result) ||
        r16(workspace+0x1F0u)!=UINT16_C(0xDEDE) ||
        r16(workspace+0x1F2u)!=UINT16_C(0x2000) ||
        r16(workspace+0x1F4u)!=UINT16_C(0xDEDE) ||
        r16(workspace+0x1F6u)!=3u ||
        r16(workspace+0x23Cu)!=UINT16_C(0xDEDE) ||
        r16(workspace+0x23Eu)!=2u) return 8;
    return 0;
}

typedef struct callbacks {
    uint8_t order[6];
    uint8_t count;
} callbacks;

static bool geo(void *user_data,uint8_t slot,
                const stf_collision_area_query *query,uint32_t response[18])
{
    callbacks *cb=(callbacks*)user_data;
    unsigned i;
    if(cb->count>=6u || query==NULL || slot>1u ||
       query->words[0]!=UINT32_C(0x38007070)) return false;
    cb->order[cb->count++]=(uint8_t)(10u+slot);
    for(i=0u;i<18u;++i) response[i]=0u;
    response[6]=fb(slot==0u?1.0f:3.0f);
    response[8]=fb(slot==0u?2.0f:1.0f);
    response[14]=fb(4.0f);
    response[15]=fb(3.0f);
    response[16]=fb(2.0f);
    response[17]=fb(1.0f);
    return true;
}
static bool distance(void *user_data,uint8_t slot,
                     const stf_collision_area_distance_query *query,
                     uint32_t *response)
{
    callbacks *cb=(callbacks*)user_data;
    if(cb->count>=6u || query==NULL || slot>1u) return false;
    cb->order[cb->count++]=(uint8_t)(20u+slot);
    return stf_collision_area_distance_semantic_bits(query,response);
}

static int integrated_side_tests(void)
{
    uint8_t f0[F_SIZE]={0u},f1[F_SIZE]={0u},ram[W_SIZE];
    stf_copro_attack_rom_view view={0};
    stf_collision_copro_area_model2 model;
    stf_collision_copro_area_result result;
    callbacks cb={0};

    memset(&model,0,sizeof(model));
    memset(ram,0xCA,sizeof(ram));
    f1[4u]=1u;
    f0[0x1A18u]=2u;f1[0x1A18u]=2u;
    w16(f0+0x26u,0u);
    w16(f1+0x26u,UINT16_C(0x1000));
    model.fighter[0]=f0;model.fighter_size[0]=sizeof(f0);
    model.fighter[1]=f1;model.fighter_size[1]=sizeof(f1);
    model.workspace=ram;model.workspace_size=sizeof(ram);
    model.rom_view=&view;
    model.geometry=geo;model.geometry_user_data=&cb;
    model.distance=distance;model.distance_user_data=&cb;
    model.angle_lut=angles;model.angle_lut_size=sizeof(angles);
    model.cage_height_bits=fb(10.0f);

    model.angle_lut_size=15u;
    if(stf_collision_copro_area_stage_model2(&model,&result) ||
       cb.count!=0u || ram[0x170u]!=0xCAu) return 1;
    model.angle_lut_size=sizeof(angles);
    model.workspace_size=0x23Fu;
    if(stf_collision_copro_area_stage_model2(&model,&result) ||
       cb.count!=0u || ram[0x170u]!=0xCAu) return 2;
    model.workspace_size=sizeof(ram);
    if(!stf_collision_copro_area_stage_model2(&model,&result) ||
       result.completed_areas!=2u || result.completed_distances!=2u ||
       result.completed_sides!=2u || cb.count!=4u ||
       cb.order[0]!=10u || cb.order[1]!=20u ||
       cb.order[2]!=11u || cb.order[3]!=21u ||
       result.side[0].plane!=STF_COLLISION_AREA_PLANE_Z ||
       result.side[1].plane!=STF_COLLISION_AREA_PLANE_X ||
       result.side[0].lut_index!=4u ||
       result.side[1].lut_index!=1u ||
       result.side[0].orientation_code!=0u ||
       result.side[1].orientation_code!=5u ||
       r16(ram+0x1F4u)!=0u || r16(ram+0x1F6u)!=5u ||
       r16(ram+0x23Cu)!=2u || r16(ram+0x23Eu)!=2u ||
       r16(ram+0x1F8u)!=UINT16_C(0x0020) ||
       r16(ram+0x1FAu)!=UINT16_C(0x0025)) return 3;

    cb.count=0u;
    model.distance=NULL;
    if(stf_collision_copro_area_stage_model2(&model,&result) ||
       cb.count!=0u) return 4;
    return 0;
}
int main(void)
{
    int code=direct_side_tests();
    if(code!=0) return code;
    code=integrated_side_tests();
    if(code!=0) return 100+code;
    return 0;
}
