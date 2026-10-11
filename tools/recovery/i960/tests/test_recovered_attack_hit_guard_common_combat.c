#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "collision_combat_runtime.h"

enum { F_SIZE=0x2200u, W_SIZE=0x2B0u, PROFILE_SIZE=3u*40u, ENEMY_SIZE=0x10Au };

static void w16(uint8_t *p,uint16_t n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8u);}
static void w32(uint8_t *p,uint32_t n)
{
    p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8u);
    p[2]=(uint8_t)(n>>16u);p[3]=(uint8_t)(n>>24u);
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

typedef struct fixture {
    uint32_t table[16];
    unsigned resolver_calls, tail_calls, legacy_calls;
    bool fail_resolver, fail_tail;
} fixture;

static bool resolve(
    uint8_t selector,const uint32_t **words,size_t *count,void *user
)
{
    fixture *fx=(fixture*)user;
    ++fx->resolver_calls;
    if(fx->fail_resolver||selector!=0u||words==NULL||count==NULL)
        return false;
    *words=fx->table;*count=16u;
    return true;
}
static bool generic(
    void *user,uint8_t attacker,uint8_t defender,
    uint8_t *a,size_t an,uint8_t *d,size_t dn,
    uint8_t *workspace,size_t wn,const stf_attack_hit_prefix_result *prefix
)
{
    fixture *fx=(fixture*)user;
    if(attacker!=1u||defender!=0u||a==NULL||d==NULL||
       workspace==NULL||prefix==NULL||an<F_SIZE||dn<F_SIZE||wn<W_SIZE)
        return false;
    ++fx->legacy_calls;
    return true;
}
static bool tail(
    void *user,uint8_t attacker,uint8_t defender,
    uint8_t *a,size_t an,uint8_t *d,size_t dn,
    uint8_t *workspace,size_t wn,
    const stf_attack_hit_prefix_result *prefix,
    const stf_attack_hit_normal_entry_result *normal,
    const stf_guard_common_runtime_result *common
)
{
    fixture *fx=(fixture*)user;
    if(attacker!=1u||defender!=0u||a==NULL||d==NULL||
       workspace==NULL||prefix==NULL||normal==NULL||common==NULL||
       an<F_SIZE||dn<F_SIZE||wn<W_SIZE ||
       normal->guard_branch!=STF_ATTACK_GUARD_BRANCH_2AC74 ||
       common->guard.skipped ||
       common->motion.table_index!=2u ||
       common->motion.motion!=UINT32_C(0x345) ||
       r32(a+0x194u)!=UINT32_C(0x10000002) ||
       r32(d+0x198u)!=UINT32_C(0x0A000345) ||
       r32(a+0x1238u)!=1u ||
       r16(d+0x5DEu)!=10u ||
       (r32(workspace+0x26Cu)&5u)!=5u ||
       !common->guard.request_sound_cane_2d ||
       common->guard.skill_amount!=4u) return false;
    ++fx->tail_calls;
    return !fx->fail_tail;
}

typedef struct scene {
    uint8_t f0[F_SIZE],f1[F_SIZE],workspace[W_SIZE];
    uint8_t enemy[2][ENEMY_SIZE];
    uint8_t profiles[PROFILE_SIZE];
    stf_damage_unit_effect_state effect;
    stf_collision_combat_model2 combat;
    stf_collision_combat_result output;
    stf_collision_hit_order order;
    fixture fx;
} scene;

static void init(scene *s)
{
    memset(s,0,sizeof(*s));
    s->f1[4u]=1u;
    s->f1[0x822u]=9u;
    s->f1[0x843u]=1u;
    w16(s->f1+0x1AAu,10u);
    w16(s->f1+0x80Cu,50u);
    w32(s->f0+0x1A4u,UINT32_C(1)<<13u);
    w32(s->profiles+40u+8u,UINT32_C(0x3F800000));
    s->fx.table[2u]=UINT32_C(0x345);
    s->combat.fighters[0]=s->f0;
    s->combat.fighters[1]=s->f1;
    s->combat.fighter_sizes[0]=sizeof(s->f0);
    s->combat.fighter_sizes[1]=sizeof(s->f1);
    s->combat.enemy_slots[0]=s->enemy[0];
    s->combat.enemy_slots[1]=s->enemy[1];
    s->combat.enemy_slot_sizes[0]=sizeof(s->enemy[0]);
    s->combat.enemy_slot_sizes[1]=sizeof(s->enemy[1]);
    s->combat.workspace=s->workspace;
    s->combat.workspace_size=sizeof(s->workspace);
    s->combat.effect=&s->effect;
    s->combat.attack_body=generic;
    s->combat.attack_body_user_data=&s->fx;
    s->combat.normal_profile_table=s->profiles;
    s->combat.normal_profile_table_size=sizeof(s->profiles);
    s->combat.guard_motion_resolver=resolve;
    s->combat.guard_motion_resolver_user_data=&s->fx;
    s->combat.guard_common_tail=tail;
    s->combat.guard_common_tail_user_data=&s->fx;
    s->combat.guard_common_limit=30u;
    s->order.count=1u;s->order.attacker[0]=1u;
}
int main(void)
{
    scene s;
    init(&s);
    if(!stf_collision_combat_apply_model2(
            &s.combat,&s.order,&s.output) ||
       s.output.attempted!=1u||s.output.completed!=1u||
       !s.output.pairs[0].normal_entry_applied||
       s.output.pairs[0].normal_entry.guard_branch!=
           STF_ATTACK_GUARD_BRANCH_2AC74||
       !s.output.pairs[0].guard_common_applied||
       s.output.pairs[0].guard_common_aborted||
       !s.output.pairs[0].guard_common_tail_completed||
       !s.output.pairs[0].body_completed||
       !s.output.pairs[0].damage_completed||
       s.fx.resolver_calls!=1u||s.fx.tail_calls!=1u||
       s.fx.legacy_calls!=0u||
       r32(s.f1+0x1234u)!=0u||
       r32(s.f1+0x1238u)!=1u||
       s.enemy[1][0x108u]!=1u||s.enemy[1][0x109u]!=1u||
       r32(s.f0+0x198u)!=UINT32_C(0x0A000345))
        return 1;

    /* Early abort at loc_2B8C8 must not query the motion table.
       It retracts prefix's counter increment and clears BOTH enemy
       +0x108 bytes, not the unrelated +0x109 marker. */
    init(&s);
    w32(s.f1+0x860u,UINT32_C(1)<<19u);
    s.f1[0x822u]=0u;
    s.enemy[0][0x108u]=1u;
    w32(s.f1+0x1234u,25u);
    s.fx.fail_resolver=true; /* should never be called on abort */
    if(!stf_collision_combat_apply_model2(
            &s.combat,&s.order,&s.output)||
       s.output.completed!=1u||s.output.attempted!=1u||
       !s.output.pairs[0].guard_common_aborted||
       s.output.pairs[0].guard_common_applied||
       s.output.pairs[0].guard_common_tail_completed||
       !s.output.pairs[0].body_completed||
       s.fx.resolver_calls!=0u||s.fx.tail_calls!=0u||
       s.fx.legacy_calls!=0u||
       r32(s.f1+0x1234u)!=25u||
       s.enemy[0][0x108u]!=0u||s.enemy[1][0x108u]!=0u||
       s.enemy[1][0x109u]!=0u)
        return 2;

    /* Motion lookup failure preserves prefix progress but must not
       change the common guard state, invoke tail or apply damage. */
    init(&s);
    s.fx.fail_resolver=true;
    w32(s.f1+0x1234u,5u);
    if(stf_collision_combat_apply_model2(
            &s.combat,&s.order,&s.output)||
       s.output.attempted!=1u||s.output.completed!=0u||
       !s.output.pairs[0].prefix_applied||
       !s.output.pairs[0].normal_entry_applied||
       s.output.pairs[0].guard_common_applied||
       s.fx.resolver_calls!=1u||s.fx.tail_calls!=0u||
       r32(s.f1+0x1234u)!=6u||
       r32(s.f0+0x198u)!=0u)
        return 3;

    /* The recovered CPU writes persist on tail failure; no damage. */
    init(&s);
    s.fx.fail_tail=true;
    if(stf_collision_combat_apply_model2(
            &s.combat,&s.order,&s.output)||
       s.output.attempted!=1u||s.output.completed!=0u||
       !s.output.pairs[0].guard_common_applied||
       s.output.pairs[0].guard_common_tail_completed||
       s.output.pairs[0].damage_completed||
       s.fx.tail_calls!=1u||s.fx.legacy_calls!=0u||
       r32(s.f0+0x198u)!=UINT32_C(0x0A000345))
        return 4;

    /* Shared preflight: insufficient enemy+0x109 capacity is rejected
       before prefix increments, even if chosen route might abort. */
    init(&s);
    s.combat.enemy_slot_sizes[0]=0x109u;
    if(stf_collision_combat_apply_model2(
            &s.combat,&s.order,&s.output)||
       r32(s.f1+0x1234u)!=0u||
       s.fx.resolver_calls!=0u||s.fx.tail_calls!=0u)
        return 5;

    /* Opt-in without profile must fail before the first mutation. */
    init(&s);
    s.combat.normal_profile_table=NULL;
    if(stf_collision_combat_apply_model2(
            &s.combat,&s.order,&s.output)||
       r32(s.f1+0x1234u)!=0u) return 6;

    /* Opt-out retains the legacy generic continuation. */
    init(&s);
    s.combat.guard_common_tail=NULL;
    if(!stf_collision_combat_apply_model2(
            &s.combat,&s.order,&s.output)||
       s.output.pairs[0].guard_common_applied||
       s.output.pairs[0].guard_common_aborted||
       s.fx.resolver_calls!=0u||s.fx.tail_calls!=0u||
       s.fx.legacy_calls!=1u)
        return 7;
    return 0;
}
