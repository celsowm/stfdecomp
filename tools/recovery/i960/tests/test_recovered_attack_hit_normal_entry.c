#include "attack_hit_normal_entry.h"
#include "collision_combat_runtime.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

enum { F_SIZE = 0x2200u, W_SIZE = 0x2B0u, PROFILE_BYTES = 256u*40u };

static void wr16(uint8_t *p, uint16_t v)
{
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8u);
}
static void wr32(uint8_t *p, uint32_t v)
{
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8u);
    p[2]=(uint8_t)(v>>16u); p[3]=(uint8_t)(v>>24u);
}
static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8u) |
           ((uint32_t)p[2]<<16u) | ((uint32_t)p[3]<<24u);
}
static uint32_t fbits(float value)
{
    uint32_t bits;
    memcpy(&bits,&value,sizeof(bits));
    return bits;
}
static float asfloat(uint32_t bits)
{
    float value;
    memcpy(&value,&bits,sizeof(value));
    return value;
}
static void table_init(uint8_t *table)
{
    memset(table,0,PROFILE_BYTES);
    wr32(table+2u*40u+8u,fbits(1.0f));
    wr32(table+3u*40u+8u,fbits(0.5f));
    wr32(table+4u*40u+8u,fbits(2.0f));
}
static int standalone_tests(void)
{
    uint8_t attacker[F_SIZE]={0u},defender[F_SIZE]={0u};
    uint8_t task[W_SIZE]={0u},table[PROFILE_BYTES];
    stf_attack_hit_prefix_result prefix;
    stf_attack_hit_normal_entry_result out;
    table_init(table);
    memset(&prefix,0,sizeof(prefix));
    prefix.hit_kind_843=2;
    attacker[0x822u]=4u;
    wr16(attacker+0x82Au,10u);
    wr16(attacker+0x26u,UINT16_C(0xFFFD));
    wr32(defender+0x1A4u,UINT32_C(1)<<13u);
    prefix.hit_flags_828=UINT16_C(1)<<12u;

    /* Guard block B and exact CPU strength source, with an explicit
       semantic-only strength scale (coprocessor rounding remains open). */
    if(!stf_attack_hit_normal_entry_apply_model2(
            attacker,sizeof(attacker),defender,sizeof(defender),
            task,sizeof(task),&prefix,table,sizeof(table),&out) ||
       out.guard_branch!=STF_ATTACK_GUARD_BLOCK_B_2AB54 ||
       out.strength.combined_82a_026!=7 ||
       out.strength.raw_822!=4u ||
       out.strength.raw_822_float_bits!=fbits(4.0f) ||
       (rd32(task+0x26Cu)&1u)==0u ||
       !out.host_strength_is_approximate ||
       fabsf(asfloat(out.host_strength_scale_bits)-0.025f)>0.0001f)
        return 1;

    /* At c70 > 1 the same bit12 selects block A. */
    wr32(defender+0xC70u,2u);
    prefix.hit_kind_843=3;
    if(!stf_attack_hit_normal_entry_apply_model2(
            attacker,sizeof(attacker),defender,sizeof(defender),
            task,sizeof(task),&prefix,table,sizeof(table),&out) ||
       out.guard_branch!=STF_ATTACK_GUARD_BLOCK_A_2AA70 ||
       fabsf(asfloat(out.host_strength_scale_bits)-0.0125f)>0.0001f)
        return 2;

    /* With defender flag13 cleared, branch exits guard condition to
       the later 0x2AE40 corridor; never confuse it with returning. */
    wr32(defender+0x1A4u,0u);
    if(!stf_attack_hit_normal_entry_apply_model2(
            attacker,sizeof(attacker),defender,sizeof(defender),
            task,sizeof(task),&prefix,table,sizeof(table),&out) ||
       out.guard_branch!=STF_ATTACK_GUARD_EXIT) return 3;

    /* Prefix already captures the 0x50FE02 hit-kind override. */
    prefix.hit_kind_843=4;
    if(!stf_attack_hit_normal_entry_apply_model2(
            attacker,sizeof(attacker),defender,sizeof(defender),
            task,sizeof(task),&prefix,table,sizeof(table),&out) ||
       fabsf(asfloat(out.host_strength_scale_bits)-0.05f)>0.0001f)
        return 4;

    /* Reject a missing ROM record before setting workspace bit 0. */
    wr32(task+0x26Cu,UINT32_C(0xA0));
    if(stf_attack_hit_normal_entry_apply_model2(
            attacker,sizeof(attacker),defender,sizeof(defender),
            task,sizeof(task),&prefix,table,4u*40u,&out) ||
       rd32(task+0x26Cu)!=UINT32_C(0xA0)) return 5;

    prefix.special_opponent_path=true;
    if(stf_attack_hit_normal_entry_apply_model2(
            attacker,sizeof(attacker),defender,sizeof(defender),
            task,sizeof(task),&prefix,table,sizeof(table),&out) ||
       rd32(task+0x26Cu)!=UINT32_C(0xA0)) return 6;
    prefix.special_opponent_path=false;
    if(stf_attack_hit_normal_entry_apply_model2(
            attacker,sizeof(attacker),defender,sizeof(defender),
            task,0x26Fu,&prefix,table,sizeof(table),&out) ||
       stf_attack_hit_normal_entry_apply_model2(
            attacker,0xC7Fu,defender,sizeof(defender),
            task,sizeof(task),&prefix,table,sizeof(table),&out) ||
       stf_attack_hit_normal_entry_apply_model2(
            attacker,sizeof(attacker),defender,0xC73u,
            task,sizeof(task),&prefix,table,sizeof(table),&out))
        return 7;
    return 0;
}
typedef struct hooks {
    unsigned bodies;
    unsigned fail_at;
} hooks;
static bool body(
    void *user,uint8_t attacker_index,uint8_t defender_index,
    uint8_t *attacker,size_t attacker_size,
    uint8_t *defender,size_t defender_size,
    uint8_t *task,size_t task_size,
    const stf_attack_hit_prefix_result *prefix)
{
    hooks *h=(hooks*)user;
    if(attacker_index>1u || defender_index!=1u-attacker_index ||
       attacker==NULL || defender==NULL || prefix==NULL ||
       attacker_size<F_SIZE || defender_size<F_SIZE ||
       task_size<W_SIZE || (rd32(task+0x26Cu)&1u)==0u) return false;
    ++h->bodies;
    return h->bodies!=h->fail_at;
}
static int combat_tests(void)
{
    uint8_t f0[F_SIZE]={0u},f1[F_SIZE]={0u};
    uint8_t task[W_SIZE]={0u},table[PROFILE_BYTES];
    uint8_t enemy0[STF_ATTACK_HIT_ENEMY_MODEL2_MIN_SIZE]={0u};
    uint8_t enemy1[STF_ATTACK_HIT_ENEMY_MODEL2_MIN_SIZE]={0u};
    stf_damage_unit_effect_state effect={0};
    stf_collision_combat_model2 model;
    stf_collision_combat_result result;
    stf_collision_hit_order order;
    hooks hook={0};
    memset(&model,0,sizeof(model));
    memset(&order,0,sizeof(order));
    table_init(table);
    f1[4u]=1u;
    f0[0x843u]=2u;
    f1[0x843u]=3u;
    f0[0x822u]=4u;
    f1[0x822u]=9u;
    wr32(f0+0x1A4u,UINT32_C(1)<<13u);
    wr16(f0+0x828u,UINT16_C(1)<<12u);
    wr16(f1+0x828u,UINT16_C(1)<<12u);
    model.fighters[0]=f0;model.fighters[1]=f1;
    model.fighter_sizes[0]=sizeof(f0);
    model.fighter_sizes[1]=sizeof(f1);
    model.enemy_slots[0]=enemy0;
    model.enemy_slots[1]=enemy1;
    model.enemy_slot_sizes[0]=sizeof(enemy0);
    model.enemy_slot_sizes[1]=sizeof(enemy1);
    model.workspace=task;model.workspace_size=sizeof(task);
    model.effect=&effect;
    model.attack_body=body;model.attack_body_user_data=&hook;
    model.normal_profile_table=table;
    model.normal_profile_table_size=sizeof(table);
    order.count=1u;order.attacker[0]=1u;

    model.normal_profile_table_size=20u;
    if(stf_collision_combat_apply_model2(&model,&order,&result) ||
       hook.bodies!=0u || rd32(f1+0x1234u)!=0u) return 1;
    model.normal_profile_table_size=sizeof(table);
    if(!stf_collision_combat_apply_model2(&model,&order,&result) ||
       result.attempted!=1u || result.completed!=1u ||
       !result.pairs[0].normal_entry_applied ||
       result.pairs[0].special_applied ||
       !result.pairs[0].body_completed ||
       result.pairs[0].normal_entry.guard_branch!=
           STF_ATTACK_GUARD_BLOCK_B_2AB54 ||
       result.pairs[0].normal_entry.strength.raw_822!=9u ||
       hook.bodies!=1u ||
       rd32(f1+0x1234u)!=1u)
        return 2;

    /* Body failure retains prefix and normal prelude, never damage. */
    hook.bodies=0u;
    hook.fail_at=1u;
    if(stf_collision_combat_apply_model2(&model,&order,&result) ||
       result.attempted!=1u || result.completed!=0u ||
       !result.pairs[0].prefix_applied ||
       !result.pairs[0].normal_entry_applied ||
       result.pairs[0].body_completed ||
       result.pairs[0].damage_completed ||
       rd32(f1+0x1234u)!=2u ||
       hook.bodies!=1u) return 3;

    /* Optional: removing the original profile table leaves the prior
       callback-only contract intact. */
    hook.bodies=0u;hook.fail_at=0u;
    model.normal_profile_table=NULL;
    if(!stf_collision_combat_apply_model2(&model,&order,&result) ||
       result.pairs[0].normal_entry_applied ||
       hook.bodies!=1u) return 4;
    return 0;
}
int main(void)
{
    int rc=standalone_tests();
    if(rc!=0) return rc;
    rc=combat_tests();
    return rc==0?0:100+rc;
}
