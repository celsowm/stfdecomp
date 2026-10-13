#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "collision_particle_frame_runtime.h"

enum { N = STF_COLLISION_PARTICLE_EXTENDED_SLOTS * STF_COLLISION_PARTICLE_SLOT_SIZE };
#define OK(x,id) do { if (!(x)) { fprintf(stderr,"particle frame scenario %d line %d\\n",(id),__LINE__);return (id); } } while(0)

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
typedef struct fixture {
    uint8_t slots[N];
    stf_collision_particle_descriptor desc[STF_COLLISION_PARTICLE_KIND_COUNT];
    stf_collision_particle_stage stage;
    stf_collision_particle_draw draws[16];
    stf_collision_particle_frame_result out;
} fixture;
static const uint16_t frames[]={101u,202u};
static const uint16_t scale_frames[]={1u,2u,3u,4u,5u,6u,7u,8u};
static void init(fixture *f)
{
    memset(f,0,sizeof(*f));
    f->desc[1].effect_address=UINT32_C(0x2000);
    f->desc[1].initial_value=UINT32_C(0x3F800000);
    f->desc[1].duration=3u;
    f->desc[1].frame_divisor=2u;
    f->desc[1].frames=frames;
    f->desc[1].frame_count=2u;
    f->stage.draw_particle=1u;
    f->stage.flags=UINT8_C(5);
    f->stage.position_a[0]=UINT32_C(0x11223344);
    f->stage.position_b[2]=UINT32_C(0x55667788);
}
static bool step(fixture *f,size_t cap)
{
    return stf_collision_particle_frame_step_model2(
        f->slots,sizeof(f->slots),f->desc,f->draws,cap,&f->out);
}

static int lifecycle(void)
{
    fixture f;
    stf_collision_particle_result alloc;
    unsigned i;
    init(&f);
    OK(stf_collision_particle_put_model2(f.slots,sizeof(f.slots),0u,
         f.desc,&f.stage,&alloc) && alloc.allocated &&
         alloc.slot_index==0u,1);
    for(i=0u;i<3u;++i) {
        OK(step(&f,16u),2);
        OK(f.out.updated && f.out.draws_built &&
           f.out.update.active_before==1u &&
           f.out.update.advanced==1u &&
           f.out.render.draw_count==1u &&
           f.slots[0x18u]==(uint8_t)(i+1u) &&
           f.draws[0].slot_index==0u &&
           f.draws[0].frame_id==(i<2u?101u:202u) &&
           f.draws[0].position[0]==UINT32_C(0x11223344) &&
           f.draws[0].scale_bits==UINT32_C(0x3F800000) &&
           f.draws[0].use_saved_graphics_state &&
           f.draws[0].suppress_orientation &&
           r32(f.slots+0x14u)==UINT32_C(0x55667788),3);
    }
    OK(step(&f,16u) && f.out.update.released==1u &&
       f.out.render.draw_count==0u &&
       r32(f.slots+0x1Cu)==0u && f.slots[0x18u]==0u,4);
    f.stage.draw_particle=1u;
    f.stage.position_a[0]=1234u;
    OK(stf_collision_particle_put_model2(f.slots,sizeof(f.slots),0u,
        f.desc,&f.stage,&alloc) && alloc.allocated &&
        alloc.slot_index==0u && r32(f.slots)==1234u,5);
    return 0;
}
static int extended_and_scale(void)
{
    fixture f;
    uint8_t saved[STF_COLLISION_PARTICLE_SLOT_SIZE];
    uint8_t *slot16;
    init(&f);
    f.desc[1].duration=8u;
    f.desc[1].frame_divisor=1u;
    f.desc[1].frames=scale_frames;
    f.desc[1].frame_count=8u;
    w32(f.slots+0x1Cu,UINT32_C(0x2000));
    f.slots[0x18u]=6u;
    f.slots[0x19u]=8u;
    slot16=f.slots+16u*STF_COLLISION_PARTICLE_SLOT_SIZE;
    w32(slot16+0x1Cu,UINT32_C(0xDEADBEEF));
    slot16[0x18u]=200u;
    memcpy(saved,slot16,sizeof(saved));
    OK(step(&f,16u) && f.out.update.active_before==1u &&
       f.out.render.draw_count==1u && f.draws[0].frame_id==7u &&
       f.draws[0].scale_bits==UINT32_C(0x3ECCCCCD) &&
       memcmp(saved,slot16,sizeof(saved))==0,6);
    OK(step(&f,16u) && f.draws[0].frame_id==8u &&
       f.draws[0].scale_bits==UINT32_C(0x40900000) &&
       memcmp(saved,slot16,sizeof(saved))==0,7);
    OK(step(&f,16u) && f.out.update.released==1u &&
       f.out.render.draw_count==0u &&
       memcmp(saved,slot16,sizeof(saved))==0,8);
    return 0;
}
static int atomic_failure(void)
{
    fixture f;
    uint8_t snap[N];
    stf_collision_particle_draw draw_before;
    stf_collision_particle_frame_result out_before;
    init(&f);
    w32(f.slots+0x1Cu,UINT32_C(0x2000));
    w32(f.slots+STF_COLLISION_PARTICLE_SLOT_SIZE+0x1Cu,UINT32_C(0x2000));
    memset(f.draws,0xA5,sizeof(f.draws));
    memset(&f.out,0x5A,sizeof(f.out));
    draw_before=f.draws[0];
    out_before=f.out;
    memcpy(snap,f.slots,sizeof(snap));
    OK(!step(&f,1u) &&
       memcmp(snap,f.slots,sizeof(snap))==0 &&
       memcmp(&draw_before,&f.draws[0],sizeof(draw_before))==0 &&
       memcmp(&out_before,&f.out,sizeof(out_before))==0,9);
    w32(f.slots+STF_COLLISION_PARTICLE_SLOT_SIZE+0x1Cu,
        UINT32_C(0xBAD00000));
    memcpy(snap,f.slots,sizeof(snap));
    OK(!step(&f,16u) &&
       memcmp(snap,f.slots,sizeof(snap))==0 &&
       memcmp(&draw_before,&f.draws[0],sizeof(draw_before))==0,10);
    w32(f.slots+STF_COLLISION_PARTICLE_SLOT_SIZE+0x1Cu,0u);
    f.desc[1].frames=NULL;
    memcpy(snap,f.slots,sizeof(snap));
    OK(!step(&f,16u) && memcmp(snap,f.slots,sizeof(snap))==0,11);
    f.desc[1].frames=frames;
    f.desc[1].frame_count=1u;
    f.slots[0x18u]=2u;
    memcpy(snap,f.slots,sizeof(snap));
    OK(!step(&f,16u) && memcmp(snap,f.slots,sizeof(snap))==0,12);
    OK(!stf_collision_particle_frame_step_model2(
        f.slots,575u,f.desc,f.draws,16u,&f.out),13);
    OK(!stf_collision_particle_frame_step_model2(
        f.slots,sizeof(f.slots),f.desc,NULL,16u,&f.out),14);
    OK(!stf_collision_particle_frame_step_model2(
        f.slots,sizeof(f.slots),f.desc,f.draws,16u,NULL),15);
    return 0;
}
static int model_binding(void)
{
    fixture f;
    stf_collision_combat_model2 combat;
    init(&f);
    OK(stf_collision_particle_frame_step_model2(
       f.slots,sizeof(f.slots),f.desc,NULL,0u,&f.out) &&
       f.out.render.draw_count==0u,16);
    memset(&combat,0,sizeof(combat));
    combat.damage_particle_slots=f.slots;
    combat.damage_particle_slots_size=sizeof(f.slots);
    combat.damage_particle_descriptors=f.desc;
    OK(!stf_collision_particle_frame_step_combat_model2(
       &combat,f.draws,16u,&f.out),17);
    combat.recover_damage_particles=true;
    OK(stf_collision_particle_frame_step_combat_model2(
       &combat,f.draws,16u,&f.out) &&
       f.out.update.advanced==0u &&
       f.out.render.draw_count==0u,18);
    OK(!stf_collision_particle_frame_step_combat_model2(
       NULL,f.draws,16u,&f.out),19);
    return 0;
}
int main(void)
{
    int n=lifecycle();
    if(n!=0)return n;
    n=extended_and_scale();
    if(n!=0)return n;
    n=atomic_failure();
    if(n!=0)return n;
    return model_binding();
}
