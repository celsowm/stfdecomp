#include "collision_kind.h"
#include <math.h>
#include <string.h>

static uint16_t u16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8u));
}
static uint32_t u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) |
           ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}
static void w32(uint8_t *p, uint32_t v)
{
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8u);
    p[2]=(uint8_t)(v>>16u);p[3]=(uint8_t)(v>>24u);
}
static float as_float(uint32_t bits)
{
    float f;
    memcpy(&f,&bits,sizeof(f));
    return f;
}
static bool bit(uint32_t flags,unsigned index)
{
    return (flags & (UINT32_C(1)<<index))!=0u;
}

bool stf_collision_kind_select_model2(
    const uint8_t *f0,size_t n0,const uint8_t *f1,size_t n1,
    const uint8_t *workspace,size_t workspace_size,
    stf_collision_kind_result *result
)
{
    stf_collision_kind_result state;
    uint32_t a,b,flags_or,flags_and,flags_xor,motion_diff;
    uint32_t facing0,facing1;
    bool fallback=false;
    if(f0==NULL||f1==NULL||workspace==NULL||result==NULL||
       n0<STF_COLLISION_KIND_FIGHTER_MIN_SIZE||
       n1<STF_COLLISION_KIND_FIGHTER_MIN_SIZE||
       workspace_size<STF_COLLISION_KIND_WORKSPACE_MIN_SIZE||
       f0[4u]!=0u||f1[4u]!=1u) return false;
    memset(&state,0,sizeof(state));
    state.kind=STF_COLLISION_KIND_NORMAL;
    a=u32(f0+0x1A4u); b=u32(f1+0x1A4u);
    facing0=u32(f0+0x70Cu);facing1=u32(f1+0x70Cu);
    flags_or=a|b;flags_and=a&b;flags_xor=a^b;
    motion_diff=facing0^facing1;
    if(bit(flags_or,18u)) {
        state.flags_g6|=1u;
    } else if(bit(motion_diff,18u)) {
        if(!bit(flags_xor,3u)) {
            fallback=true;
        } else if(!bit(facing0,18u)) {
            const uint32_t speed=u32(f0+0x5F4u);
            if(!bit(a,3u) || (bit(u32(f1+0xC58u),18u)&&!bit(b,4u)) ||
               u16(f1+0x1AAu)<4u) {
                fallback=true;
            } else if(bit(b,1u)||bit(u32(f0+0x5B8u),9u)||
                      bit(u32(f0+0x5B8u),8u)) {
                state.kind=STF_COLLISION_KIND_TOBIKOE_0;
                goto selected;
            } else if(!isfinite(as_float(speed)) ||
                      as_float(speed)>0.4f) {
                fallback=true;
            } else {
                state.kind=STF_COLLISION_KIND_TOBIKOE_0;
                goto selected;
            }
        } else {
            const uint32_t speed=u32(f1+0x5F4u);
            if(!bit(b,3u) || (bit(u32(f0+0xC58u),18u)&&!bit(a,4u))||
               u16(f0+0x1AAu)<4u) {
                fallback=true;
            } else if(bit(a,1u)||bit(u32(f1+0x5B8u),9u)||
                      bit(u32(f1+0x5B8u),8u)) {
                state.kind=STF_COLLISION_KIND_TOBIKOE_1;
                goto selected;
            } else if(!isfinite(as_float(speed)) ||
                      as_float(speed)>0.4f) {
                fallback=true;
            } else {
                state.kind=STF_COLLISION_KIND_TOBIKOE_1;
                goto selected;
            }
        }
    } else if(bit(flags_xor,8u)) {
        fallback=true;
    } else if(bit(flags_and,8u)) {
        state.flags_g6|=4u;
    }
    if(fallback) {
        /* addo 0x1f,0xa = 41; either fighter kind 41 bypasses bit 1. */
        if(f0[0x820u]!=41u&&f1[0x820u]!=41u) state.flags_g6|=2u;
    }
    if(bit(flags_or,14u)) state.flags_g6|=8u;
    if(bit(state.flags_g6,0u)||bit(state.flags_g6,2u)) goto selected;

    if(bit(a,16u)) {
        if(bit(a,15u)||!bit(b,22u)) goto fixed;
        if(bit(u32(f1+0x804u),12u)||bit(u32(f1+0x804u),4u)) {
            state.flags_g6|=UINT32_C(1)<<6u;
            state.kind=STF_COLLISION_KIND_RIDE_1;
            goto selected;
        }
        state.flags_g6|=UINT32_C(1)<<7u;
        goto fixed;
    }
    if(bit(b,16u)) {
        if(bit(b,15u)||!bit(a,22u)) goto fixed;
        if(bit(u32(f0+0x804u),12u)||bit(u32(f0+0x804u),4u)) {
            state.flags_g6|=UINT32_C(1)<<4u;
            state.kind=STF_COLLISION_KIND_RIDE_0;
            goto selected;
        }
        state.flags_g6|=UINT32_C(1)<<5u;
        goto fixed;
    }
    if(bit(state.flags_g6,1u)) {
        const uint32_t angle=u32(workspace+0x1E0u);
        const uint32_t idx=(angle>>13u)&7u;
        const uint32_t offsets0[8]={12u,0u,0u,8u,8u,4u,4u,12u};
        const uint32_t offsets1[8]={8u,4u,4u,12u,12u,0u,0u,8u};
        const uint8_t *selected_fighter;
        const uint32_t *offsets;
        float plane;
        state.angle_index=(uint8_t)idx;
        if(bit(a,8u)) {
            state.kind=STF_COLLISION_KIND_BODY;
            selected_fighter=f1;offsets=offsets1;
        } else {
            state.kind=STF_COLLISION_KIND_BODY_0;
            selected_fighter=f0;offsets=offsets0;
        }
        state.selected_plane_bits=u32(selected_fighter+0x650u+offsets[idx]);
        plane=as_float(state.selected_plane_bits);
        if(!isfinite(plane)) return false;
        if(plane<=0.1f) {
            state.kind=bit(a,8u)?
                STF_COLLISION_KIND_KABE:STF_COLLISION_KIND_KABE_0;
        }
        goto selected;
    }
    if(bit(flags_xor,4u)) {
        state.kind=bit(a,4u)?
            STF_COLLISION_KIND_JUMP_0:STF_COLLISION_KIND_JUMP_1;
    }
    goto selected;

fixed:
    /* loc_2D20C checks fighter0 bit16 first, then fighter1. */
    if(bit(a,16u)) {
        if(!bit(b,16u)) state.kind=bit(a,15u)?
            STF_COLLISION_KIND_RISE_0:STF_COLLISION_KIND_FIX_0;
    } else if(bit(b,16u)) {
        state.kind=bit(b,15u)?
            STF_COLLISION_KIND_RISE_1:STF_COLLISION_KIND_FIX_1;
    }
selected:
    *result=state;
    return true;
}

bool stf_collision_unit_to_ball_model2(
    uint32_t mask,const uint8_t *rows,size_t rows_size,uint32_t *balls
)
{
    uint32_t result=0u;
    unsigned unit;
    if(balls==NULL || (mask!=0u && rows==NULL)) return false;
    for(unit=0u;unit<STF_UNIT_TO_BALL_UNIT_COUNT;++unit) {
        unsigned j;
        const uint64_t end=(uint64_t)unit*8u+8u;
        if(!bit(mask,unit)) continue;
        if(end>rows_size) return false;
        for(j=0u;j<8u;++j) {
            const unsigned ball=rows[unit*8u+j]>>2u;
            if(j!=0u && ball==0u) break;
            result |= UINT32_C(1)<<(ball&31u);
        }
        /* Require a sentinel after the first entry, within this row. */
        if(j==8u) return false;
    }
    *balls=result;
    return true;
}

bool stf_collision_kind_apply_model2(
    const stf_collision_kind_result *selection,
    const stf_collision_kind_profile *profiles,size_t profile_count,
    const uint8_t *unit_rows0,size_t rows0_size,
    const uint8_t *unit_rows1,size_t rows1_size,
    uint8_t *workspace,size_t workspace_size,
    stf_collision_kind_special_fn special,void *special_user_data
)
{
    stf_collision_kind_profile p;
    uint32_t m0,m1,aux0,aux1,cc,d0;
    if(selection==NULL||profiles==NULL||workspace==NULL||
       workspace_size<STF_COLLISION_KIND_APPLY_WORKSPACE_MIN_SIZE||
       (unsigned)selection->kind>=profile_count) return false;
    p=profiles[selection->kind];
    if(!stf_collision_unit_to_ball_model2(p.units0,unit_rows0,rows0_size,&m0)||
       !stf_collision_unit_to_ball_model2(p.units1,unit_rows1,rows1_size,&m1)||
       !stf_collision_unit_to_ball_model2(p.units264,unit_rows0,rows0_size,&aux0)||
       !stf_collision_unit_to_ball_model2(p.units268,unit_rows1,rows1_size,&aux1)) return false;
    cc=p.scalar_cc;d0=p.scalar_d0;
    if(p.scalar_cc==UINT32_C(0x411E6666)) {
        uint32_t a=u32(workspace+0x27Cu)&UINT32_C(0x7FFFFFFF);
        uint32_t b=u32(workspace+0x280u)&UINT32_C(0x7FFFFFFF);
        if((a|b)!=0u) {
            if(special==NULL||!special(special_user_data,a,b,&cc,&d0))
                return false;
        }
    }
    w32(workspace+0xC0u,~m0);
    w32(workspace+0xC4u,~m1);
    w32(workspace+0xCCu,cc);
    w32(workspace+0xD0u,d0);
    w32(workspace+0xC8u,p.scalar_c8);
    w32(workspace+0x88u,p.scalar_88);
    w32(workspace+0x264u,aux0);
    w32(workspace+0x268u,aux1);
    return true;
}
