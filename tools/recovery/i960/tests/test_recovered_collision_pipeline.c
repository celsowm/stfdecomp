#include "collision_pipeline.h"
#include <stdint.h>
#include <string.h>

enum { F_SIZE=0x2200u, TASK_SIZE=0x2B0u };
static void w16(uint8_t *p, uint16_t v)
{
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8u);
}
static void w32(uint8_t *p, uint32_t v)
{
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8u);
    p[2]=(uint8_t)(v>>16u);p[3]=(uint8_t)(v>>24u);
}
static uint16_t r16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8u));
}
static uint32_t r32(const uint8_t *p)
{
    return (uint32_t)p[0]|((uint32_t)p[1]<<8u)|
        ((uint32_t)p[2]<<16u)|((uint32_t)p[3]<<24u);
}
typedef struct trace {
    uint8_t geometry_calls;
    uint8_t hit_calls;
    uint8_t attacker[2];
    uint8_t fail_hit_at;
    uint8_t invalid_geometry_second;
} trace;
static bool fake_geometry(
    void *user, uint8_t fighter_index,
    const stf_collision_area_query *query,
    uint32_t output[STF_COLLISION_AREA_RESPONSE_WORDS])
{
    trace *t=(trace *)user;
    if (query == NULL || fighter_index != t->geometry_calls ||
        query->words[0]!=UINT32_C(0x38007070)) return false;
    ++t->geometry_calls;
    if(t->invalid_geometry_second&&fighter_index==1u) return false;
    memset(output,0,STF_COLLISION_AREA_RESPONSE_WORDS*sizeof(output[0]));
    return true;
}
static bool fake_body(
    void *user,uint8_t attacker_index,uint8_t defender_index,
    uint8_t *attacker,size_t attacker_size,
    uint8_t *defender,size_t defender_size,
    uint8_t *workspace,size_t workspace_size,
    const stf_attack_hit_prefix_result *prefix)
{
    trace *t=(trace *)user;
    if(prefix==NULL||attacker==NULL||defender==NULL||workspace==NULL||
       attacker_size<F_SIZE||defender_size<F_SIZE||
       workspace_size<TASK_SIZE||attacker_index>1u||
       defender_index!=1u-attacker_index||t->hit_calls>=2u) return false;
    t->attacker[t->hit_calls]=attacker_index;
    ++t->hit_calls;
    return t->hit_calls!=t->fail_hit_at;
}
typedef struct scene {
    uint8_t fighters[2][F_SIZE];
    uint8_t workspace[TASK_SIZE];
    uint8_t image[0x400u];
    uint8_t hit_effect[STF_COLLISION_ATTACK_EFFECT_MODEL2_SIZE];
    uint8_t hit_xyz[12u];
    uint8_t hit_source[2][12u];
    uint8_t enemies[2][STF_ATTACK_HIT_ENEMY_MODEL2_MIN_SIZE];
    uint16_t mapping[STF_COLLISION_ATTACK_MAPPING_COUNT];
    stf_collision_kind_profile profiles[STF_COLLISION_KIND_COUNT];
    stf_damage_unit_effect_state damage_effect;
    stf_copro_attack_rom_view rom;
    stf_collision_copro_area_model2 area;
    stf_collision_frame_model2 frame;
    stf_collision_combat_model2 combat;
    stf_collision_pipeline_model2 pipeline;
    trace events;
} scene;
static void init_scene(scene *s)
{
    size_t i;
    memset(s,0,sizeof(*s));
    s->fighters[1][4u]=1u;
    s->rom.image=s->image;
    s->rom.image_size=sizeof(s->image);
    s->rom.image_base=UINT32_C(0xCE000);
    s->rom.profile_table_address=UINT32_C(0xCE020);
    s->rom.profile_count=42u;
    w32(s->image+0x24u,UINT32_C(1)<<3u);
    w32(s->image+0x28u,UINT32_C(1)<<4u);
    for(i=0u;i<32u;++i) {
        w32(s->image+0x200u+i*4u,15u);
        w32(s->image+0x300u+i*4u,15u);
    }
    w32(s->image+0x200u+2u*4u,3u);
    w32(s->image+0x300u+4u*4u,4u);
    s->fighters[0][0x820u]=1u;
    s->fighters[1][0x820u]=2u;
    w32(s->fighters[0]+0x1D18u,UINT32_C(0xCE200));
    w32(s->fighters[1]+0x1D18u,UINT32_C(0xCE300));
    for(i=0u;i<2u;++i) {
        uint8_t *f=s->fighters[i];
        s->area.fighter[i]=f;
        s->area.fighter_size[i]=F_SIZE;
        s->frame.fighters[i]=f;
        s->frame.fighter_sizes[i]=F_SIZE;
        s->frame.r5_selected_xyz[i]=s->hit_source[i];
        s->frame.r5_selected_xyz_sizes[i]=sizeof(s->hit_source[i]);
        s->combat.fighters[i]=f;
        s->combat.fighter_sizes[i]=F_SIZE;
        s->combat.enemy_slots[i]=s->enemies[i];
        s->combat.enemy_slot_sizes[i]=sizeof(s->enemies[i]);
        w32(f+0xAF0u,UINT32_MAX);
        w32(f+0xAF4u,UINT32_MAX);
    }
    s->area.rom_view=&s->rom;
    s->area.workspace=s->workspace;
    s->area.workspace_size=TASK_SIZE;
    s->area.geometry=fake_geometry;
    s->area.geometry_user_data=&s->events;
    s->frame.workspace=s->workspace;
    s->frame.workspace_size=TASK_SIZE;
    s->frame.mapping=s->mapping;
    s->frame.effect=s->hit_effect;
    s->frame.effect_size=sizeof(s->hit_effect);
    s->frame.copro_hit_xyz=s->hit_xyz;
    s->frame.copro_hit_xyz_size=sizeof(s->hit_xyz);
    s->combat.workspace=s->workspace;
    s->combat.workspace_size=TASK_SIZE;
    s->combat.effect=&s->damage_effect;
    s->combat.num_rounds_to_win=2u;
    s->combat.attack_body=fake_body;
    s->combat.attack_body_user_data=&s->events;
    s->pipeline.area=&s->area;
    s->pipeline.frame=&s->frame;
    s->pipeline.combat=&s->combat;
}
static void enable_hits(scene *s)
{
    uint8_t *f0=s->fighters[0],*f1=s->fighters[1];
    s->area.kind_profiles=s->profiles;
    s->area.kind_profile_count=STF_COLLISION_KIND_COUNT;
    w32(f0+0x1A4u,UINT32_C(1)<<8u);
    w32(f1+0x1A4u,UINT32_C(1)<<8u);
    w32(f1+0x860u,UINT32_C(1)<<22u);
    w16(f0+0x1AAu,10u);w16(f1+0x1AAu,10u);
    w16(f0+0x808u,5u);w16(f1+0x808u,5u);
    w16(f0+0x1A8u,4u);w16(f1+0x1A8u,9u);
    w16(s->workspace+0x8Eu,9u);
    s->mapping[3u]=UINT16_C(1)<<1u;
    s->mapping[5u]=UINT16_C(1)<<4u;
    w32(f0+0x1234u,20u);
    w32(f1+0x1234u,30u);
}
int main(void)
{
    scene s;
    stf_collision_pipeline_result out;
    init_scene(&s);

    /* Shared-state preflight rejects miswired composite consumers
     * without issuing a geometry query or resetting the workspace. */
    s.frame.workspace_size=TASK_SIZE-1u;
    s.workspace[0x170u]=0xA5u;
    if(stf_collision_pipeline_apply_model2(&s.pipeline,&out) ||
       s.events.geometry_calls!=0u ||
       s.workspace[0x170u]!=0xA5u) return 1;
    s.frame.workspace_size=TASK_SIZE;
    s.combat.attack_body=NULL;
    if(stf_collision_pipeline_apply_model2(&s.pipeline,&out) ||
       s.events.geometry_calls!=0u) return 2;
    s.combat.attack_body=fake_body;

    /* Without attack flags, full area + suppression + frame + combat
       succeeds with no fabricated hits or damage. */
    if(!stf_collision_pipeline_apply_model2(&s.pipeline,&out) ||
       !out.completed_area || out.completed_suppressions!=2u ||
       !out.completed_checks || !out.completed_combat ||
       out.frame.order.count!=0u || out.combat.completed!=0u ||
       s.events.geometry_calls!=2u || s.events.hit_calls!=0u ||
       out.suppression_mask[0]!=0u || out.suppression_mask[1]!=0u) return 3;

    init_scene(&s);
    enable_hits(&s);
    if(!stf_collision_pipeline_apply_model2(&s.pipeline,&out)) return 4;
    if(!out.completed_area || !out.area.completed_kind ||
       out.area.kind.kind!=STF_COLLISION_KIND_NORMAL ||
       out.completed_suppressions!=2u ||
       out.suppression_mask[0]!=0u || out.suppression_mask[1]!=0u ||
       !out.completed_checks || !out.completed_combat ||
       !out.frame.checks[0].hit || !out.frame.checks[1].hit ||
       out.frame.checks[0].selected_unit!=1u ||
       out.frame.checks[1].selected_unit!=5u ||
       out.frame.order.count!=2u ||
       out.frame.order.attacker[0]!=1u ||
       out.frame.order.attacker[1]!=0u ||
       out.combat.completed!=2u || s.events.hit_calls!=2u ||
       s.events.attacker[0]!=1u || s.events.attacker[1]!=0u ||
       r32(s.fighters[0]+0xAC8u)!=(UINT32_C(1)<<3u) ||
       r32(s.fighters[1]+0xAC8u)!=(UINT32_C(1)<<4u) ||
       r16(s.workspace+0x274u)!=1u ||
       r16(s.workspace+0x276u)!=1u ||
       r32(s.fighters[1]+0x1234u)!=31u ||
       r32(s.fighters[0]+0x1234u)!=21u) return 5;

    /* A failed recovered attack body cannot be counted as damage. */
    init_scene(&s);
    enable_hits(&s);
    s.events.fail_hit_at=1u;
    if(stf_collision_pipeline_apply_model2(&s.pipeline,&out) ||
       !out.completed_area || out.completed_suppressions!=2u ||
       !out.completed_checks || out.completed_combat ||
       out.combat.attempted!=1u || out.combat.completed!=0u ||
       s.events.hit_calls!=1u ||
       r32(s.fighters[1]+0x1234u)!=30u) return 6;

    /* A failure on the second geometry query retains only the
       first area and prevents any suppression/combat writes. */
    init_scene(&s);
    s.events.invalid_geometry_second=1u;
    if(stf_collision_pipeline_apply_model2(&s.pipeline,&out) ||
       out.completed_area || out.area.completed_areas!=1u ||
       out.completed_suppressions!=0u || out.completed_checks ||
       s.events.geometry_calls!=2u) return 7;

    if(stf_collision_pipeline_apply_model2(NULL,&out) ||
       stf_collision_pipeline_apply_model2(&s.pipeline,NULL)) return 8;
    return 0;
}
