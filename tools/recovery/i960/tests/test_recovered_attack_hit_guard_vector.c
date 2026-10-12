#include <math.h>
#include <stdint.h>
#include <string.h>

#include "attack_hit_guard_vector.h"
#include "collision_combat_runtime.h"

enum { F_SIZE=0x2200u, W_SIZE=0x2B0u, PROFILE_SIZE=3u*40u };
static void w16(uint8_t *p,uint16_t n)
{
    p[0]=(uint8_t)n; p[1]=(uint8_t)(n>>8u);
}
static void w32(uint8_t *p,uint32_t n)
{
    p[0]=(uint8_t)n; p[1]=(uint8_t)(n>>8u);
    p[2]=(uint8_t)(n>>16u); p[3]=(uint8_t)(n>>24u);
}
static uint32_t r32(const uint8_t *p)
{
    return (uint32_t)p[0]|((uint32_t)p[1]<<8u)|
        ((uint32_t)p[2]<<16u)|((uint32_t)p[3]<<24u);
}
static uint32_t fb(float value)
{
    uint32_t v;
    memcpy(&v,&value,sizeof(v));
    return v;
}
static float asfloat(uint32_t bits)
{
    float f;
    memcpy(&f,&bits,sizeof(f));
    return f;
}
typedef struct fixture {
    uint8_t calls;
    uint8_t fail_at;
    uint8_t nonfinite_at;
    bool query_invalid;
    bool guard_tail_seen;
    uint8_t generic_calls;
    const uint8_t *defender;
    uint32_t seen[5];
} fixture;
static bool fake_tgp(
    void *user,const stf_attack_hit_guard_vector_query *query,
    uint32_t *response
)
{
    fixture *fx=(fixture*)user;
    static const uint32_t opcodes[5]={
        UINT32_C(0x0D001A1A),UINT32_C(0x12002424),
        UINT32_C(0x12802525),UINT32_C(0x12002424),
        UINT32_C(0x12802525)};
    unsigned i;
    if(fx->calls>=5u||query==NULL||response==NULL) return false;
    i=fx->calls++;
    fx->seen[i]=query->opcode;
    if(query->opcode!=opcodes[i] ||
       query->argument_count!=(uint8_t)(i==0u?1u:2u) ||
       (i==0u && !(asfloat(query->words[0])>0.0f)) ||
       (i==1u && (query->words[0]!=0u || query->words[1]!=fb(2.0f))) ||
       (i==2u && (query->words[0]!=0u || query->words[1]!=fb(2.0f))) ||
       (i==3u && (query->words[0]!=UINT32_C(0xFFFFFFE0) ||
                  query->words[1]!=fb(4.0f))) ||
       (i==4u && (query->words[0]!=UINT32_C(0xFFFFFFE0) ||
                  query->words[1]!=fb(4.0f)))) {
        fx->query_invalid=true;
        return false;
    }
    if(fx->defender!=NULL && r32(fx->defender+0xC6Cu)!=0x13u) {
        fx->query_invalid=true;
        return false;
    }
    if(fx->fail_at==(uint8_t)(i+1u)) return false;
    *response=fx->nonfinite_at==(uint8_t)(i+1u)?
        UINT32_C(0x7FC00000):fb((float)(i+2u));
    return true;
}
static int direct(void)
{
    uint8_t a[F_SIZE]={0u},d[F_SIZE]={0u},scales[16]={0u};
    stf_attack_hit_guard_vector_result out;
    fixture fx={0};
    w32(d+0x5D8u,fb(60.0f));
    w32(d+0x5E0u,fb(91.0f));
    w32(d+0x5E4u,fb(92.0f));
    w32(d+0x5E8u,fb(93.0f));
    if(!stf_attack_hit_guard_vector_apply_model2(
            a,sizeof(a),d,sizeof(d),32,fb(16.0f),1u,
            NULL,0u,fake_tgp,&fx,&out) ||
       out.completed_commands!=5u || out.boosted ||
       fx.calls!=5u || fx.query_invalid ||
       r32(d+0x5E0u)!=fb(5.0f) ||
       r32(d+0x5E4u)!=fb(3.0f) ||
       r32(d+0x5E8u)!=fb(6.0f) ||
       out.resulting[0]!=fb(5.0f)) return 1;

    /* +0x7D2 bit0: first float scales vertical, second horizontal. */
    a[0x7D2u]=1u;
    w32(scales+8u,fb(1.5f));
    w32(scales+12u,fb(2.0f));
    memset(&fx,0,sizeof(fx));
    if(!stf_attack_hit_guard_vector_apply_model2(
            a,sizeof(a),d,sizeof(d),32,fb(16.0f),1u,
            scales,sizeof(scales),fake_tgp,&fx,&out) ||
       !out.boosted || fx.calls!=5u ||
       r32(d+0x5E0u)!=fb(10.0f) ||
       r32(d+0x5E4u)!=fb(4.5f) ||
       r32(d+0x5E8u)!=fb(12.0f)) return 2;

    /* Fail before issuing commands if the required row is missing. */
    memset(&fx,0,sizeof(fx));
    if(stf_attack_hit_guard_vector_apply_model2(
            a,sizeof(a),d,sizeof(d),32,fb(16.0f),1u,
            scales,15u,fake_tgp,&fx,&out) ||
       fx.calls!=0u ||r32(d+0x5E0u)!=fb(10.0f)) return 3;

    /* Command-3 failure retains no new vector writes, but reports
       exactly two preceding successful replies. */
    a[0x7D2u]=0u;
    memset(&fx,0,sizeof(fx));
    fx.fail_at=3u;
    if(stf_attack_hit_guard_vector_apply_model2(
            a,sizeof(a),d,sizeof(d),32,fb(16.0f),1u,
            NULL,0u,fake_tgp,&fx,&out) ||
       fx.calls!=3u||out.completed_commands!=2u ||
       r32(d+0x5E0u)!=fb(10.0f) ||
       r32(d+0x5E4u)!=fb(4.5f) ||
       r32(d+0x5E8u)!=fb(12.0f)) return 4;

    memset(&fx,0,sizeof(fx));
    fx.nonfinite_at=5u;
    if(stf_attack_hit_guard_vector_apply_model2(
            a,sizeof(a),d,sizeof(d),32,fb(16.0f),1u,
            NULL,0u,fake_tgp,&fx,&out) ||
       fx.calls!=5u||out.completed_commands!=4u ||
       r32(d+0x5E4u)!=fb(4.5f)) return 5;
    memset(&fx,0,sizeof(fx));
    if(stf_attack_hit_guard_vector_apply_model2(
            a,0x843u,d,sizeof(d),32,fb(16.0f),1u,
            NULL,0u,fake_tgp,&fx,&out) ||
       stf_attack_hit_guard_vector_apply_model2(
            a,sizeof(a),d,0x5EBu,32,fb(16.0f),1u,
            NULL,0u,fake_tgp,&fx,&out) ||
       stf_attack_hit_guard_vector_apply_model2(
            a,sizeof(a),d,sizeof(d),32,fb(-1.0f),1u,
            NULL,0u,fake_tgp,&fx,&out) ||
       fx.calls!=0u) return 6;

    /* Guard output comparison must use the FINAL post-scale words. */
    {
        uint32_t expected[3]={fb(5.0f),fb(3.0f),fb(6.0f)};
        a[0x7D2u]=0u;
        memset(&fx,0,sizeof(fx));
        if(!stf_attack_hit_guard_vector_apply_verified_model2(
                a,sizeof(a),d,sizeof(d),32,fb(16.0f),1u,
                NULL,0u,fake_tgp,&fx,expected,&out) ||
           !out.vectors_checked || out.mismatch_mask!=0u ||
           out.completed_commands!=5u ||
           out.ulp_distance[0]!=0u || fx.calls!=5u ||
           r32(d+0x5E0u)!=expected[0] ||
           r32(d+0x5E4u)!=expected[1] ||
           r32(d+0x5E8u)!=expected[2]) return 7;

        expected[0]=fb(5.0f)+UINT32_C(1);
        w32(d+0x5E0u,fb(91.0f));
        w32(d+0x5E4u,fb(92.0f));
        w32(d+0x5E8u,fb(93.0f));
        memset(&fx,0,sizeof(fx));
        if(stf_attack_hit_guard_vector_apply_verified_model2(
                a,sizeof(a),d,sizeof(d),32,fb(16.0f),1u,
                NULL,0u,fake_tgp,&fx,expected,&out) ||
           !out.vectors_checked || out.mismatch_mask!=1u ||
           out.ulp_distance[0]!=1u ||
           out.ulp_distance[1]!=0u ||out.ulp_distance[2]!=0u ||
           out.completed_commands!=5u || fx.calls!=5u ||
           r32(d+0x5E0u)!=fb(91.0f) ||
           r32(d+0x5E4u)!=fb(92.0f) ||
           r32(d+0x5E8u)!=fb(93.0f)) return 8;

        /* Original API still operates without external oracle. */
        memset(&fx,0,sizeof(fx));
        if(!stf_attack_hit_guard_vector_apply_model2(
                a,sizeof(a),d,sizeof(d),32,fb(16.0f),1u,
                NULL,0u,fake_tgp,&fx,&out) ||
           out.vectors_checked ||out.mismatch_mask!=0u ||
           r32(d+0x5E0u)!=fb(5.0f)) return 9;

        /* A reference NaN cannot authorize any vector overwrite. */
        expected[0]=UINT32_C(0x7FC00000);
        memset(&fx,0,sizeof(fx));
        if(stf_attack_hit_guard_vector_apply_verified_model2(
                a,sizeof(a),d,sizeof(d),32,fb(16.0f),1u,
                NULL,0u,fake_tgp,&fx,expected,&out) ||
           out.vectors_checked ||fx.calls!=5u ||
           r32(d+0x5E0u)!=fb(5.0f)) return 10;
    }
    return 0;
}
static bool resolve(
    uint8_t selector,const uint32_t **table,size_t *count,void *user
)
{
    static const uint32_t motions[16]={
        0u,0u,0u,0u,0u,0u,0u,0u,0u,0u,0x123u,0u,0u,0u,0u,0u
    };
    (void)user;
    if(selector!=0u||table==NULL||count==NULL) return false;
    *table=motions;*count=16u;
    return true;
}
static bool generic_body(
    void *user,uint8_t attacker_index,uint8_t defender_index,
    uint8_t *attacker,size_t attacker_size,uint8_t *defender,
    size_t defender_size,uint8_t *work,size_t work_size,
    const stf_attack_hit_prefix_result *prefix
)
{
    fixture *fx=(fixture*)user;
    (void)attacker_index;(void)defender_index;(void)attacker;
    (void)attacker_size;(void)defender;(void)defender_size;
    (void)work;(void)work_size;(void)prefix;
    ++fx->generic_calls;
    return false; /* must not be used for guard */
}
static bool tail(
    void *user,uint8_t ai,uint8_t di,
    uint8_t *a,size_t an,uint8_t *d,size_t dn,
    uint8_t *work,size_t wn,
    const stf_attack_hit_prefix_result *prefix,
    const stf_attack_hit_normal_entry_result *normal,
    const stf_guard_state_runtime_result *guard
)
{
    fixture *fx=(fixture*)user;
    if(ai!=1u||di!=0u||a==NULL||d==NULL||work==NULL||
       prefix==NULL||normal==NULL||guard==NULL||
       an<F_SIZE||dn<F_SIZE||wn<W_SIZE||
       fx->calls!=5u||fx->query_invalid ||
       r32(d+0x5E0u)!=fb(5.0f) ||
       r32(d+0x5E4u)!=fb(3.0f) ||
       r32(d+0x5E8u)!=fb(6.0f) ||
       r32(d+0x198u)!=UINT32_C(0x0B000123) ||
       (r32(work+0x26Cu)&13u)!=13u) return false;
    fx->guard_tail_seen=true;
    return true;
}
static int integrated(void)
{
    uint8_t f0[F_SIZE]={0u},f1[F_SIZE]={0u},work[W_SIZE]={0u};
    uint8_t enemy0[STF_ATTACK_HIT_ENEMY_MODEL2_MIN_SIZE]={0u};
    uint8_t enemy1[STF_ATTACK_HIT_ENEMY_MODEL2_MIN_SIZE]={0u};
    uint8_t profiles[PROFILE_SIZE]={0u};
    stf_damage_unit_effect_state effect={0};
    stf_collision_combat_model2 model;
    stf_collision_combat_result result;
    stf_collision_hit_order order;
    fixture fx={0};
    memset(&model,0,sizeof(model));
    memset(&order,0,sizeof(order));
    f1[4u]=1u;f1[0x843u]=1u;f1[0x822u]=8u;
    w16(f1+0x828u,UINT16_C(1)<<12u);
    w16(f1+0x26u,32u);
    w32(profiles+40u+8u,fb(1.0f));
    w32(f0+0x1A4u,UINT32_C(1)<<13u);
    w32(f0+0x5D8u,fb(60.0f));
    model.fighters[0]=f0;model.fighters[1]=f1;
    model.fighter_sizes[0]=sizeof(f0);model.fighter_sizes[1]=sizeof(f1);
    model.enemy_slots[0]=enemy0;model.enemy_slots[1]=enemy1;
    model.enemy_slot_sizes[0]=sizeof(enemy0);
    model.enemy_slot_sizes[1]=sizeof(enemy1);
    model.workspace=work;model.workspace_size=sizeof(work);
    model.effect=&effect;
    model.attack_body=generic_body;
    model.attack_body_user_data=&fx;
    model.normal_profile_table=profiles;
    model.normal_profile_table_size=sizeof(profiles);
    model.guard_tail=tail;model.guard_tail_user_data=&fx;
    model.guard_motion_resolver=resolve;
    model.guard_vector_tgp=fake_tgp;
    model.guard_vector_tgp_user_data=&fx;
    fx.defender=f0;
    order.count=1u;order.attacker[0]=1u;

    if(!stf_collision_combat_apply_model2(&model,&order,&result) ||
       result.completed!=1u||
       !result.pairs[0].guard_applied ||
       !result.pairs[0].guard_vector_applied ||
       !result.pairs[0].guard_tail_completed ||
       !result.pairs[0].body_completed ||
       !result.pairs[0].damage_completed ||
       result.pairs[0].guard_vector.completed_commands!=5u ||
       fx.calls!=5u ||!fx.guard_tail_seen ||fx.generic_calls!=0u ||
       r32(f1+0x1234u)!=1u) return 1;

    /* TGP failures preserve guard-state CPU stores but never call tail
       or damage, and do not overwrite old defender vectors. */
    memset(&fx,0,sizeof(fx));
    fx.defender=f0;fx.fail_at=4u;
    w32(f0+0x5E0u,fb(31.0f));
    w32(f0+0x5E4u,fb(32.0f));
    w32(f0+0x5E8u,fb(33.0f));
    if(stf_collision_combat_apply_model2(&model,&order,&result) ||
       result.attempted!=1u||result.completed!=0u ||
       !result.pairs[0].guard_applied ||
       result.pairs[0].guard_vector_applied ||
       result.pairs[0].guard_tail_completed ||
       result.pairs[0].guard_vector.completed_commands!=3u ||
       fx.calls!=4u||fx.guard_tail_seen||
       r32(f0+0x5E0u)!=fb(31.0f)||
       r32(f0+0x5E4u)!=fb(32.0f)||
       r32(f0+0x5E8u)!=fb(33.0f)) return 2;

    /* Preflight denies an orphan TGP resolver before prefix mutation. */
    fx.fail_at=0u;fx.calls=0u;
    model.guard_tail=NULL;
    if(stf_collision_combat_apply_model2(&model,&order,&result) ||
       fx.calls!=0u ||r32(f1+0x1234u)!=2u) return 3;

    /* The ordered BLOCK_A/B integration checks X/Y/Z before modifying any
     * axis. A one-ULP error must skip both the tail and damage_unit. */
    model.guard_tail=tail;
    model.verify_guard_vectors=true;
    model.guard_vector_expected_xyz[0][0]=fb(5.0f)+UINT32_C(1);
    model.guard_vector_expected_xyz[0][1]=fb(3.0f);
    model.guard_vector_expected_xyz[0][2]=fb(6.0f);
    memset(&fx,0,sizeof(fx));fx.defender=f0;
    w32(f0+0x5E0u,fb(31.0f));
    w32(f0+0x5E4u,fb(32.0f));
    w32(f0+0x5E8u,fb(33.0f));
    if(stf_collision_combat_apply_model2(&model,&order,&result) ||
       !result.pairs[0].guard_applied ||
       result.pairs[0].guard_vector_applied ||
       result.pairs[0].guard_tail_completed ||
       result.pairs[0].damage_completed ||
       !result.pairs[0].guard_vector.vectors_checked ||
       result.pairs[0].guard_vector.mismatch_mask!=1u ||
       result.pairs[0].guard_vector.ulp_distance[0]!=1u ||
       result.pairs[0].guard_vector.completed_commands!=5u ||
       fx.calls!=5u ||fx.guard_tail_seen ||
       r32(f0+0x5E0u)!=fb(31.0f) ||
       r32(f0+0x5E4u)!=fb(32.0f) ||
       r32(f0+0x5E8u)!=fb(33.0f)) return 4;

    model.guard_vector_expected_xyz[0][0]=fb(5.0f);
    memset(&fx,0,sizeof(fx));fx.defender=f0;
    if(!stf_collision_combat_apply_model2(&model,&order,&result) ||
       !result.pairs[0].guard_vector_applied ||
       !result.pairs[0].guard_tail_completed ||
       !result.pairs[0].guard_vector.vectors_checked ||
       result.pairs[0].guard_vector.mismatch_mask!=0u ||
       result.pairs[0].guard_vector.completed_commands!=5u ||
       fx.calls!=5u ||!fx.guard_tail_seen ||
       r32(f0+0x5E0u)!=fb(5.0f)) return 5;

    /* Invalid oracle words are rejected BEFORE the attacker prefix. */
    model.guard_vector_expected_xyz[1][1]=UINT32_C(0x7F800000);
    memset(&fx,0,sizeof(fx));fx.defender=f0;
    {
        uint32_t counter=r32(f1+0x1234u);
        if(stf_collision_combat_apply_model2(&model,&order,&result) ||
           r32(f1+0x1234u)!=counter ||
           fx.calls!=0u ||fx.guard_tail_seen) return 6;
    }

    model.guard_vector_expected_xyz[1][1]=0u;
    model.guard_vector_tgp=NULL;
    {
        uint32_t counter=r32(f1+0x1234u);
        if(stf_collision_combat_apply_model2(&model,&order,&result) ||
           r32(f1+0x1234u)!=counter ||fx.calls!=0u) return 7;
    }
    return 0;
}
int main(void)
{
    int rc=direct();
    if(rc!=0) return rc;
    rc=integrated();
    return rc==0?0:100+rc;
}
