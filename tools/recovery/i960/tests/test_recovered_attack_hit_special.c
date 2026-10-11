#include <stdint.h>
#include <string.h>

#include "attack_hit_special.h"
#include "collision_combat_runtime.h"

enum { F_SIZE=0x2200u, W_SIZE=0x2B0u };
static void w16(uint8_t *p, uint16_t v)
{
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8u);
}
static void w32(uint8_t *p, uint32_t v)
{
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8u);
    p[2]=(uint8_t)(v>>16u);p[3]=(uint8_t)(v>>24u);
}
static uint32_t r32(const uint8_t *p)
{
    return (uint32_t)p[0]|((uint32_t)p[1]<<8u)|
           ((uint32_t)p[2]<<16u)|((uint32_t)p[3]<<24u);
}
static uint32_t fb(float f)
{
    uint32_t v;
    memcpy(&v,&f,sizeof(v));
    return v;
}
typedef struct fixture {
    unsigned tgp_calls;
    unsigned record_calls;
    unsigned body_calls;
    bool fail_tgp;
    bool fail_record;
    uint16_t last_motion;
    stf_attack_hit_special_tgp_query last_request;
} fixture;
static const uint8_t k_record[4]={5u,0x02u,0x40u,0xF1u};
static bool tgp(void *user,
                const stf_attack_hit_special_tgp_query *query,
                uint32_t response[2])
{
    fixture *ctx=(fixture*)user;
    ++ctx->tgp_calls;
    if (ctx->fail_tgp||query==NULL||response==NULL) return false;
    ctx->last_request=*query;
    response[0]=fb(2.5f);
    response[1]=fb(3.5f);
    return true;
}
static bool record(void *user,uint16_t motion,
                   const uint8_t **out,size_t *len)
{
    fixture *ctx=(fixture*)user;
    ++ctx->record_calls;
    ctx->last_motion=motion;
    if(ctx->fail_record||out==NULL||len==NULL) return false;
    *out=k_record;
    *len=sizeof(k_record);
    return true;
}
static bool body(
    void *user,uint8_t attacker_index,uint8_t defender_index,
    uint8_t *attacker,size_t attacker_size,
    uint8_t *defender,size_t defender_size,
    uint8_t *workspace,size_t workspace_size,
    const stf_attack_hit_prefix_result *prefix)
{
    fixture *ctx=(fixture*)user;
    (void)attacker_index;(void)defender_index;(void)attacker;
    (void)attacker_size;(void)defender;(void)defender_size;
    (void)workspace;(void)workspace_size;(void)prefix;
    ++ctx->body_calls;
    return true;
}
static int direct_special(void)
{
    uint8_t a[F_SIZE]={0u},b[F_SIZE]={0u};
    stf_attack_hit_special_result result;
    fixture ctx={0};
    w16(b+0x19Cu,UINT16_C(0x8012));
    w32(b+0x1Cu,fb(3.0f));
    w32(b,(UINT32_C(1)<<10u)|(UINT32_C(1)<<4u));
    w32(a,(UINT32_C(1)<<2u)|(UINT32_C(1)<<6u));
    w32(a+0x1A4u,(UINT32_C(1)<<21u)|(UINT32_C(1)<<4u));
    a[0x83Fu]=1u;
    w32(a+0x1F8u,fb(10.0f));
    w32(a+0x18u,fb(15.0f));
    w32(a+0x20u,fb(20.0f));

    /* Invalid record must reject *before* motion state is overwritten. */
    if(stf_attack_hit_special_apply_model2(
            a,sizeof(a),b,sizeof(b),k_record,3u,
            NULL,NULL,&result) ||
       r32(a+0x198u)!=0u) return 1;
    if(!stf_attack_hit_special_apply_model2(
            a,sizeof(a),b,sizeof(b),k_record,sizeof(k_record),
            NULL,NULL,&result) ||
       result.motion_19c!=UINT16_C(0x8012) ||
       result.animation_halfword!=UINT16_C(0xC002) ||
       result.animation_byte_822!=UINT8_C(0xF1) ||
       !result.cleared_motion_flag||result.needed_tgp ||
       r32(a+0x198u)!=UINT32_C(0x0D00C002) ||
       r32(b+0x198u)!=UINT32_C(0x0C008012) ||
       a[0x822u]!=UINT8_C(0xF1) ||
       r32(a+0x1Cu)!=fb(3.0f) ||
       r32(a+0x84u)!=fb(7.0f) ||
       r32(a+0x18u)!=fb(15.0f) ||
       r32(a+0x20u)!=fb(20.0f) ||
       (r32(a)&(UINT32_C(1)<<2u))!=0u ||
       (r32(a)&(UINT32_C(1)<<6u))==0u ||
       (r32(b)&(UINT32_C(1)<<4u))!=0u ||
       (r32(a+0x1A4u)&((UINT32_C(1)<<21u)|(UINT32_C(1)<<4u)))!=0u)
        return 2;

    /* The optional TGP branch uses 3 original argument words. */
    memset(a,0,sizeof(a));memset(b,0,sizeof(b));
    w16(b+0x19Cu,UINT16_C(0x1234));
    w32(b+0x1Cu,fb(4.0f));
    w32(a,UINT32_C(1)<<2u);
    w32(a+0x1F8u,fb(12.0f));
    w32(a+0x18u,fb(10.0f));
    w32(a+0x20u,fb(20.0f));
    w16(a+0x26u,UINT16_C(0xA120));
    w32(a+0x80u,UINT32_C(0x11223344));
    w32(a+0x88u,UINT32_C(0x88776655));
    if(stf_attack_hit_special_apply_model2(
            a,sizeof(a),b,sizeof(b),k_record,sizeof(k_record),
            NULL,NULL,&result) ||
       r32(a+0x198u)!=0u || r32(b+0x198u)!=0u) return 3;
    ctx.fail_tgp=true;
    if(stf_attack_hit_special_apply_model2(
            a,sizeof(a),b,sizeof(b),k_record,sizeof(k_record),
            tgp,&ctx,&result) ||
       r32(a+0x198u)!=0u || r32(b+0x198u)!=0u ||
       ctx.tgp_calls!=1u) return 4;
    ctx.fail_tgp=false;
    if(!stf_attack_hit_special_apply_model2(
            a,sizeof(a),b,sizeof(b),k_record,sizeof(k_record),
            tgp,&ctx,&result) ||
       !result.needed_tgp ||
       ctx.tgp_calls!=2u ||
       ctx.last_request.opcode!=UINT32_C(0x2D805B5B) ||
       ctx.last_request.heading_26!=(int32_t)(int16_t)UINT16_C(0xA120) ||
       ctx.last_request.vector_80!=UINT32_C(0x11223344) ||
       ctx.last_request.vector_88!=UINT32_C(0x88776655) ||
       r32(a+0x18u)!=fb(7.5f) ||
       r32(a+0x20u)!=fb(16.5f) ||
       r32(a+0x84u)!=fb(8.0f) ||
       r32(a+0x198u)!=UINT32_C(0x0D004002) ||
       r32(b+0x198u)!=UINT32_C(0x0C001234)) return 5;

    /* Defender bit10 controls attacker bit6, including its clear case. */
    w32(a,UINT32_C(1)<<6u);
    w32(b,0u);
    if(!stf_attack_hit_special_apply_model2(
            a,sizeof(a),b,sizeof(b),k_record,sizeof(k_record),
            NULL,NULL,&result) ||
       (r32(a)&(UINT32_C(1)<<6u))!=0u ||
       result.cleared_motion_flag) return 6;

    if(stf_attack_hit_special_apply_model2(
            a,STF_ATTACK_HIT_SPECIAL_ATTACKER_MIN_SIZE-1u,
            b,sizeof(b),k_record,sizeof(k_record),NULL,NULL,&result) ||
       stf_attack_hit_special_apply_model2(
            a,sizeof(a),b,STF_ATTACK_HIT_SPECIAL_DEFENDER_MIN_SIZE-1u,
            k_record,sizeof(k_record),NULL,NULL,&result) ||
       stf_attack_hit_special_apply_model2(
            a,sizeof(a),b,sizeof(b),k_record,sizeof(k_record),
            NULL,NULL,NULL)) return 7;
    return 0;
}
static int combat_special(void)
{
    uint8_t f[2][F_SIZE],enemy[2][STF_ATTACK_HIT_ENEMY_MODEL2_MIN_SIZE];
    uint8_t workspace[W_SIZE];
    stf_damage_unit_effect_state effect;
    stf_collision_combat_model2 model;
    stf_collision_hit_order order;
    stf_collision_combat_result result;
    fixture ctx;
    memset(&model,0,sizeof(model));
    memset(&order,0,sizeof(order));
    memset(f,0,sizeof(f));
    memset(enemy,0,sizeof(enemy));
    memset(workspace,0,sizeof(workspace));
    memset(&effect,0,sizeof(effect));
    memset(&ctx,0,sizeof(ctx));
    f[1][4u]=1u;
    f[0][0x19Fu]=UINT8_C(0x12);
    w16(f[0]+0x19Cu,UINT16_C(0x8012));
    w32(f[0]+0x1Cu,fb(3.0f));
    w32(f[1]+0x1F8u,fb(10.0f));
    w32(f[1],(UINT32_C(1)<<2u));
    f[1][0x83Fu]=1u;
    w32(f[1]+0x1234u,40u);
    model.fighters[0]=f[0];model.fighters[1]=f[1];
    model.fighter_sizes[0]=sizeof(f[0]);
    model.fighter_sizes[1]=sizeof(f[1]);
    model.enemy_slots[0]=enemy[0];model.enemy_slots[1]=enemy[1];
    model.enemy_slot_sizes[0]=sizeof(enemy[0]);
    model.enemy_slot_sizes[1]=sizeof(enemy[1]);
    model.workspace=workspace;model.workspace_size=sizeof(workspace);
    model.effect=&effect;
    model.attack_body=body;
    model.attack_body_user_data=&ctx;
    model.special_record=record;
    model.special_record_user_data=&ctx;
    model.special_tgp=tgp;
    model.special_tgp_user_data=&ctx;
    order.count=1u;order.attacker[0]=1u;
    if(!stf_collision_combat_apply_model2(&model,&order,&result) ||
       result.completed!=1u ||result.attempted!=1u ||
       !result.pairs[0].prefix_applied ||
       !result.pairs[0].special_applied ||
       !result.pairs[0].body_completed ||
       !result.pairs[0].damage_completed ||
       !result.pairs[0].damage.damage.skipped ||
       ctx.record_calls!=1u ||ctx.last_motion!=UINT16_C(0x8012) ||
       ctx.body_calls!=0u ||ctx.tgp_calls!=0u ||
       r32(f[1]+0x1234u)!=41u ||
       enemy[1][0x108u]!=1u ||
       r32(workspace+0x26Cu)!=0u ||
       r32(f[1]+0x198u)!=UINT32_C(0x0D00C002) ||
       r32(f[1]+0x84u)!=fb(7.0f)) return 1;

    /* Failure of the original record lookup does not run the generic
       body, and leaves prefix increments visible (nontransactional). */
    ctx.fail_record=true;
    w32(f[1]+0x1234u,60u);
    if(stf_collision_combat_apply_model2(&model,&order,&result) ||
       result.attempted!=1u ||result.completed!=0u ||
       !result.pairs[0].prefix_applied ||
       result.pairs[0].special_applied ||
       result.pairs[0].body_completed ||
       ctx.body_calls!=0u ||
       r32(f[1]+0x1234u)!=61u) return 2;

    /* Opt-out retains the previously supported generic body corridor. */
    model.special_record=NULL;
    if(!stf_collision_combat_apply_model2(&model,&order,&result) ||
       result.pairs[0].special_applied ||
       !result.pairs[0].prefix.special_opponent_path ||
       ctx.body_calls!=1u) return 3;
    return 0;
}
int main(void)
{
    const int first=direct_special();
    if(first!=0) return first;
    { int rc=combat_special(); return rc==0?0:100+rc; }
}
