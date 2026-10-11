#include <stdint.h>
#include <string.h>

#include "collision_combat_runtime.h"

enum { F_SIZE=0x2200u, W_SIZE=0x2B0u, PROFILE_BYTES=3u*40u };
static void wr16(uint8_t *p,uint16_t v)
{
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8u);
}
static void wr32(uint8_t *p,uint32_t v)
{
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8u);
    p[2]=(uint8_t)(v>>16u);p[3]=(uint8_t)(v>>24u);
}
static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8u));
}
static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0]|((uint32_t)p[1]<<8u)|
           ((uint32_t)p[2]<<16u)|((uint32_t)p[3]<<24u);
}
typedef struct fixture {
    unsigned resolver_calls;
    unsigned generic_calls;
    unsigned guard_calls;
    bool fail_tail;
    uint8_t sound_event;
    uint8_t selector;
    size_t motion_count;
    uint32_t motion_table[48u];
} fixture;

static bool resolve_motion(
    uint8_t selector,const uint32_t **words,size_t *count,void *context)
{
    fixture *fx=(fixture*)context;
    ++fx->resolver_calls;
    if(words==NULL||count==NULL||selector!=fx->selector)
        return false;
    *words=fx->motion_table;
    *count=fx->motion_count;
    return true;
}
static bool generic_body(
    void *context,uint8_t attacker_id,uint8_t defender_id,
    uint8_t *attacker,size_t attacker_size,
    uint8_t *defender,size_t defender_size,
    uint8_t *workspace,size_t workspace_size,
    const stf_attack_hit_prefix_result *prefix
)
{
    fixture *fx=(fixture*)context;
    if(attacker_id!=1u || defender_id!=0u ||
       attacker==NULL||defender==NULL||workspace==NULL||prefix==NULL||
       attacker_size<F_SIZE || defender_size<F_SIZE||workspace_size<W_SIZE)
       return false;
    ++fx->generic_calls;
    return true;
}
static bool guard_tail(
    void *context,uint8_t attacker_id,uint8_t defender_id,
    uint8_t *attacker,size_t attacker_size,
    uint8_t *defender,size_t defender_size,
    uint8_t *workspace,size_t workspace_size,
    const stf_attack_hit_prefix_result *prefix,
    const stf_attack_hit_normal_entry_result *normal,
    const stf_guard_state_runtime_result *guard
)
{
    fixture *fx=(fixture*)context;
    if(attacker_id!=1u||defender_id!=0u||
       attacker==NULL||defender==NULL||workspace==NULL||
       prefix==NULL||normal==NULL||guard==NULL||
       attacker_size<F_SIZE||defender_size<F_SIZE||workspace_size<W_SIZE||
       (rd32(workspace+0x26Cu)&13u)!=13u||
       rd32(defender+0xC6Cu)!=UINT32_C(0x13)||
       rd32(attacker+0x194u)!=UINT32_C(0x10000002)||
       normal->guard_branch!=STF_ATTACK_GUARD_BLOCK_B_2AB54 &&
       normal->guard_branch!=STF_ATTACK_GUARD_BLOCK_A_2AA70)
       return false;
    fx->sound_event=(uint8_t)guard->guard.sound;
    ++fx->guard_calls;
    /* Fake test callback is intentionally NOT claimed to implement
       loc_2B738's real TGP+effects tail. */
    return !fx->fail_tail;
}

typedef struct scene {
    uint8_t f0[F_SIZE],f1[F_SIZE],workspace[W_SIZE];
    uint8_t enemies[2][STF_ATTACK_HIT_ENEMY_MODEL2_MIN_SIZE];
    uint8_t profile[PROFILE_BYTES];
    stf_damage_unit_effect_state effect;
    stf_collision_combat_model2 model;
    stf_collision_hit_order order;
    stf_collision_combat_result result;
    fixture fx;
} scene;
static void init(scene *s,bool use_resolver)
{
    memset(s,0,sizeof(*s));
    s->f1[4u]=1u;
    s->f1[0x822u]=8u;
    s->f1[0x843u]=1u;
    wr16(s->f1+0x828u,UINT16_C(1)<<12u);
    wr16(s->f1+0x1AAu,10u);
    wr16(s->f1+0x80Cu,50u);
    wr16(s->f1+0x6D0u,UINT16_C(0xFFFD));
    wr16(s->f1+0x1224u,3u);
    wr32(s->f1+0x1228u,UINT32_C(0x11223344));
    wr16(s->f1+0x1226u,UINT16_C(0xAABB));
    wr16(s->f1+0x1248u,2u);
    wr16(s->f1+0x124Au,UINT16_C(0xCCDD));
    wr32(s->f0+0x1A4u,UINT32_C(1)<<13u);
    wr32(s->profile+40u+8u,UINT32_C(0x3F800000)); /* kind1 scale */
    s->fx.motion_count=48u;
    s->fx.motion_table[10u]=UINT32_C(0x234);
    s->fx.selector=0u;
    s->model.fighters[0]=s->f0;
    s->model.fighters[1]=s->f1;
    s->model.fighter_sizes[0]=sizeof(s->f0);
    s->model.fighter_sizes[1]=sizeof(s->f1);
    s->model.enemy_slots[0]=s->enemies[0];
    s->model.enemy_slots[1]=s->enemies[1];
    s->model.enemy_slot_sizes[0]=sizeof(s->enemies[0]);
    s->model.enemy_slot_sizes[1]=sizeof(s->enemies[1]);
    s->model.workspace=s->workspace;
    s->model.workspace_size=sizeof(s->workspace);
    s->model.effect=&s->effect;
    s->model.attack_body=generic_body;
    s->model.attack_body_user_data=&s->fx;
    s->model.normal_profile_table=s->profile;
    s->model.normal_profile_table_size=sizeof(s->profile);
    s->model.guard_tail=guard_tail;
    s->model.guard_tail_user_data=&s->fx;
    s->model.guard_motion_resolver=use_resolver?resolve_motion:NULL;
    s->model.guard_motion_resolver_user_data=&s->fx;
    s->order.count=1u;
    s->order.attacker[0]=1u;
}
int main(void)
{
    scene s;
    init(&s,true);

    /* BLOCK_B (C70 <= 1) resolves sub_2B94C(group 1) before stores. */
    if(!stf_collision_combat_apply_model2(&s.model,&s.order,&s.result) ||
       s.result.attempted!=1u||s.result.completed!=1u||
       !s.result.pairs[0].normal_entry_applied||
       !s.result.pairs[0].guard_applied||
       !s.result.pairs[0].guard_tail_completed||
       !s.result.pairs[0].body_completed||
       !s.result.pairs[0].damage_completed||
       s.result.pairs[0].normal_entry.guard_branch!=
           STF_ATTACK_GUARD_BLOCK_B_2AB54 ||
       !s.result.pairs[0].guard_state.resolved_motion||
       s.result.pairs[0].guard_state.motion.table_index!=10u||
       s.fx.resolver_calls!=1u||s.fx.guard_calls!=1u||
       s.fx.generic_calls!=0u||
       rd32(s.f0+0x198u)!=UINT32_C(0x0B000234)||
       rd32(s.f0+0xC6Cu)!=UINT32_C(0x13)||
       rd32(s.workspace+0x26Cu)!=13u||
       rd16(s.f0+0x5DEu)!=40u||
       rd16(s.f0+0x6D8u)!=UINT16_C(0xFFFD)||
       rd32(s.f1+0x121Cu)!=UINT32_C(0x11223344)||
       rd16(s.f1+0x1220u)!=UINT16_C(0xAABB)||
       rd16(s.f1+0x1244u)!=UINT16_C(0xCCDD))
        return 1;

    /* BLOCK_A (C70 > 1): fixed animation, NO motion resolver. */
    init(&s,false);
    wr32(s.f0+0xC70u,3u);
    if(!stf_collision_combat_apply_model2(&s.model,&s.order,&s.result)||
       s.result.pairs[0].normal_entry.guard_branch!=
           STF_ATTACK_GUARD_BLOCK_A_2AA70 ||
       !s.result.pairs[0].guard_applied||
       s.result.pairs[0].guard_state.resolved_motion||
       rd32(s.f0+0x198u)!=UINT32_C(0x0A00013D)||
       rd32(s.f0+0xC70u)!=2u||
       s.fx.sound_event!=(uint8_t)STF_GUARD_SOUND_KNOCK_3||
       s.fx.resolver_calls!=0u||s.fx.guard_calls!=1u||
       s.fx.generic_calls!=0u)
        return 2;

    /* Missing ROM motion resolver for B must not apply guard stores.
       Prefix counter and +0x26C bit0 are intentionally not rolled back. */
    init(&s,false);
    wr32(s.f1+0x1234u,10u);
    if(stf_collision_combat_apply_model2(&s.model,&s.order,&s.result)||
       s.result.attempted!=1u||s.result.completed!=0u||
       !s.result.pairs[0].prefix_applied||
       !s.result.pairs[0].normal_entry_applied||
       s.result.pairs[0].guard_applied||
       s.result.pairs[0].guard_tail_completed||
       s.fx.guard_calls!=0u||s.fx.generic_calls!=0u||
       rd32(s.f0+0xC6Cu)!=0u||
       rd32(s.f1+0x1234u)!=11u||
       rd32(s.workspace+0x26Cu)!=1u)
        return 3;

    /* Lookup index outside a truncated table is also fail-closed. */
    init(&s,true);
    s.fx.motion_count=10u;
    if(stf_collision_combat_apply_model2(&s.model,&s.order,&s.result)||
       s.result.pairs[0].guard_applied ||
       s.fx.resolver_calls!=1u||
       rd32(s.f0+0x198u)!=0u ||
       s.fx.guard_calls!=0u)
        return 4;

    /* Failed tail preserves guard CPU writes and reports exact boundary,
       but must NOT apply damage_unit. */
    init(&s,true);
    s.fx.fail_tail=true;
    if(stf_collision_combat_apply_model2(&s.model,&s.order,&s.result)||
       s.result.attempted!=1u||s.result.completed!=0u||
       !s.result.pairs[0].guard_applied||
       s.result.pairs[0].guard_tail_completed||
       s.result.pairs[0].body_completed||
       s.result.pairs[0].damage_completed||
       s.fx.guard_calls!=1u||s.fx.generic_calls!=0u||
       rd32(s.f0+0x198u)!=UINT32_C(0x0B000234)||
       rd32(s.workspace+0x26Cu)!=13u)
        return 5;

    /* Non-guard route: legacy body is still required and called. */
    init(&s,true);
    wr32(s.f0+0x1A4u,0u);
    if(!stf_collision_combat_apply_model2(&s.model,&s.order,&s.result)||
       s.result.pairs[0].normal_entry.guard_branch!=STF_ATTACK_GUARD_EXIT||
       s.result.pairs[0].guard_applied||
       s.fx.guard_calls!=0u||s.fx.generic_calls!=1u||
       !s.result.pairs[0].body_completed)
        return 6;

    /* Explicit guard-tail opt-in without the normal profile is rejected
       BEFORE prefix increments, without attempting either callback. */
    init(&s,true);
    s.model.normal_profile_table=NULL;
    s.workspace[0x170u]=0xA5u;
    if(stf_collision_combat_apply_model2(&s.model,&s.order,&s.result)||
       rd32(s.f1+0x1234u)!=0u||
       s.fx.guard_calls!=0u||s.fx.generic_calls!=0u||
       s.workspace[0x170u]!=0xA5u)
        return 7;

    /* Legacy mode (no guard-tail): both normal profile and the generic
       callback are preserved, even when classifier chooses BLOCK_B. */
    init(&s,true);
    s.model.guard_tail=NULL;
    if(!stf_collision_combat_apply_model2(&s.model,&s.order,&s.result)||
       s.result.pairs[0].guard_applied ||
       s.fx.guard_calls!=0u||s.fx.generic_calls!=1u)
        return 8;
    return 0;
}
