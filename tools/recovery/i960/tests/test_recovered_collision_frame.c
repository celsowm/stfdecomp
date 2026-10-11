#include <stdint.h>
#include <string.h>

#include "collision_frame.h"

static void wr16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8u);
}
static void wr32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8u);
    p[2] = (uint8_t)(v >> 16u);
    p[3] = (uint8_t)(v >> 24u);
}
static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8u));
}
static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8u) |
           ((uint32_t)p[2]<<16u) | ((uint32_t)p[3]<<24u);
}

typedef struct callbacks {
    uint8_t attackers[2];
    uint8_t defenders[2];
    uint8_t count;
    uint8_t fail_on;
} callbacks;

static bool dispatch(void *opaque, uint8_t attacker, uint8_t defender)
{
    callbacks *cb = (callbacks *)opaque;
    if (cb->count >= 2u) return false;
    cb->attackers[cb->count] = attacker;
    cb->defenders[cb->count] = defender;
    ++cb->count;
    return cb->count != cb->fail_on;
}

int main(void)
{
    uint8_t f0[STF_COLLISION_ATTACK_FIGHTER_MODEL2_MIN_SIZE] = {0u};
    uint8_t f1[STF_COLLISION_ATTACK_FIGHTER_MODEL2_MIN_SIZE] = {0u};
    uint8_t ram[STF_COLLISION_ATTACK_WORKSPACE_MODEL2_MIN_SIZE] = {0u};
    uint8_t effect[STF_COLLISION_ATTACK_EFFECT_MODEL2_SIZE];
    uint8_t hit_xyz[12u];
    uint8_t source0[12u], source1[12u];
    uint16_t mapping[STF_COLLISION_ATTACK_MAPPING_COUNT] = {0u};
    stf_collision_frame_model2 frame;
    stf_collision_frame_result result;
    callbacks cb;
    size_t completed = 99u;
    unsigned i;

    memset(&frame, 0, sizeof(frame));
    memset(effect, 0xCD, sizeof(effect));
    memset(hit_xyz, 0xCD, sizeof(hit_xyz));
    for (i=0u;i<12u;++i) {
        source0[i]=(uint8_t)(0x20u+i);
        source1[i]=(uint8_t)(0x40u+i);
        f0[0x1F4u+5u*12u+i]=(uint8_t)(0x70u+i);
        f1[0x1F4u+3u*12u+i]=(uint8_t)(0x60u+i);
    }
    f1[4u] = 1u;
    frame.fighters[0]=f0; frame.fighter_sizes[0]=sizeof(f0);
    frame.fighters[1]=f1; frame.fighter_sizes[1]=sizeof(f1);
    frame.workspace=ram; frame.workspace_size=sizeof(ram);
    frame.mapping=mapping;
    frame.attack_profile_bits[0]=UINT32_C(1)<<1u;
    frame.attack_profile_bits[1]=UINT32_C(1)<<2u;
    frame.r5_selected_xyz[0]=source0;frame.r5_selected_xyz_sizes[0]=sizeof(source0);
    frame.r5_selected_xyz[1]=source1;frame.r5_selected_xyz_sizes[1]=sizeof(source1);
    frame.effect=effect;frame.effect_size=sizeof(effect);
    frame.copro_hit_xyz=hit_xyz;frame.copro_hit_xyz_size=sizeof(hit_xyz);

    /* Incomplete buffers must fail before touching state or effect. */
    frame.effect_size=sizeof(effect)-1u;
    if (stf_collision_frame_check_model2(&frame,&result) ||
        rd16(ram+0x274u)!=0u || effect[0x78u]!=0xCDu) return 1;
    frame.effect_size=sizeof(effect);
    frame.r5_selected_xyz_sizes[1]=11u;
    if (stf_collision_frame_check_model2(&frame,&result) ||
        rd16(ram+0x274u)!=0u) return 2;
    frame.r5_selected_xyz_sizes[1]=12u;

    mapping[1]=UINT16_C(1)<<3u; /* fighter0: direct mapping */
    mapping[5]=UINT16_C(1)<<2u; /* fighter1: transposed mapping */
    wr32(f0+0x1A4u,UINT32_C(1)<<8u);
    wr32(f1+0x1A4u,UINT32_C(1)<<8u);
    wr32(f1+0x860u,UINT32_C(1)<<22u); /* allow second hit despite lockout */
    wr16(f0+0x1AAu,10u); wr16(f1+0x1AAu,10u);
    wr16(f0+0x808u,5u); wr16(f1+0x808u,5u);
    wr16(f0+0x1A8u,4u);wr16(f1+0x1A8u,9u);
    wr16(ram+0x8Eu,9u); /* fighter1 motion unchanged */

    if (!stf_collision_frame_check_model2(&frame,&result)) return 3;
    if (!result.checks[0].hit || !result.checks[1].hit ||
        result.checks[0].selected_unit!=3u ||
        result.checks[1].selected_unit!=5u ||
        rd16(ram+0x274u)!=1u || rd16(ram+0x276u)!=1u ||
        rd16(ram+0x90u)!=3u || rd32(ram+0x2ACu)!=8u ||
        rd32(f0+0x7E0u)!=5u || rd32(f1+0x7E0u)!=3u ||
        result.order.count!=2u ||
        result.order.attacker[0]!=1u || result.order.attacker[1]!=0u) return 4;
    if (effect[0x78u]!=10u || memcmp(hit_xyz,f0+0x1F4u+5u*12u,12u)!=0 ||
        memcmp(effect+0x7Cu,hit_xyz,12u)!=0 ||
        memcmp(effect+0x8ECu,hit_xyz,12u)!=0 ||
        memcmp(effect+0x8F8u,source1,12u)!=0) return 5;

    memset(&cb,0,sizeof(cb));
    if (!stf_collision_frame_dispatch(&result.order,dispatch,&cb,&completed) ||
        completed!=2u || cb.count!=2u ||
        cb.attackers[0]!=1u || cb.defenders[0]!=0u ||
        cb.attackers[1]!=0u || cb.defenders[1]!=1u) return 6;
    memset(&cb,0,sizeof(cb));
    cb.fail_on=1u;
    if (stf_collision_frame_dispatch(&result.order,dispatch,&cb,&completed) ||
        completed!=0u || cb.count!=1u) return 7;

    result.order.count=3u;
    if (stf_collision_frame_dispatch(&result.order,dispatch,&cb,&completed) ||
        completed!=0u) return 8;

    /* Lockout prevents a second hit unless +0x860 bit22 is enabled. */
    memset(ram,0,sizeof(ram));
    wr32(f1+0x860u,0u);
    if (!stf_collision_frame_check_model2(&frame,&result) ||
        !result.checks[0].hit || result.checks[1].hit ||
        result.order.count!=1u || result.order.attacker[0]!=0u) return 9;

    /* No hit means no events, and the previous effect is left intact. */
    memset(ram,0,sizeof(ram));
    wr32(f0+0x1A4u,0u);
    wr32(f1+0x1A4u,0u);
    effect[0x78u]=0xAAu;
    if (!stf_collision_frame_check_model2(&frame,&result) ||
        result.order.count!=0u || effect[0x78u]!=0xAAu) return 10;

    f1[4u]=0u;
    if (stf_collision_frame_check_model2(&frame,&result)) return 11;
    if (stf_collision_frame_check_model2(NULL,&result) ||
        stf_collision_frame_check_model2(&frame,NULL)) return 12;
    return 0;
}
