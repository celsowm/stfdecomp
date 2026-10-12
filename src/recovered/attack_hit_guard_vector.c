#include "attack_hit_guard_vector.h"
#include "tgp_command_trace.h"

#include <math.h>
#include <string.h>

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8u) |
           ((uint32_t)p[2]<<16u) | ((uint32_t)p[3]<<24u);
}
static void wr32(uint8_t *p,uint32_t bits)
{
    p[0]=(uint8_t)bits;p[1]=(uint8_t)(bits>>8u);
    p[2]=(uint8_t)(bits>>16u);p[3]=(uint8_t)(bits>>24u);
}
static float asfloat(uint32_t bits)
{
    float result;
    memcpy(&result,&bits,sizeof(result));
    return result;
}
static uint32_t asbits(float f)
{
    uint32_t bits;
    memcpy(&bits,&f,sizeof(bits));
    return bits;
}
static bool device(
    stf_attack_hit_guard_vector_tgp_fn tgp,
    void *user_data,
    uint32_t opcode,
    uint8_t argument_count,
    uint32_t first,
    uint32_t second,
    uint32_t *response
)
{
    stf_attack_hit_guard_vector_query request;
    request.opcode=opcode;
    request.argument_count=argument_count;
    request.words[0]=first;
    request.words[1]=second;
    return tgp(user_data,&request,response) &&
        isfinite(asfloat(*response));
}

bool stf_attack_hit_guard_vector_apply_verified_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    int32_t combined_heading,
    uint32_t strength_r9_bits,
    uint8_t hit_kind,
    const uint8_t *boost_table,
    size_t boost_table_size,
    stf_attack_hit_guard_vector_tgp_fn tgp,
    void *tgp_user_data,
    const uint32_t expected_xyz_bits[3],
    stf_attack_hit_guard_vector_result *result
)
{
    stf_attack_hit_guard_vector_result local;
    uint32_t replies[5] = {0u};
    uint32_t first_angle;
    float magnitude;
    float body_parameter;
    float strength;
    size_t scale_offset=0u;
    bool boosted;
    unsigned i;

    if (attacker==NULL||defender==NULL||result==NULL||tgp==NULL||
        attacker_size<STF_ATTACK_HIT_GUARD_VECTOR_ATTACKER_MIN_SIZE||
        defender_size<STF_ATTACK_HIT_GUARD_VECTOR_DEFENDER_MIN_SIZE) {
        return false;
    }

    memset(&local,0,sizeof(local));
    boosted=(attacker[0x7D2u]&1u)!=0u;
    local.boosted=boosted;
    if (boosted) {
        scale_offset=(size_t)hit_kind*8u;
        if(boost_table==NULL || scale_offset>boost_table_size ||
           8u>boost_table_size-scale_offset ||
           !isfinite(asfloat(rd32(boost_table+scale_offset))) ||
           !isfinite(asfloat(rd32(boost_table+scale_offset+4u)))) {
            *result=local;
            return false;
        }
    }
    body_parameter=asfloat(rd32(defender+0x5D8u));
    strength=asfloat(strength_r9_bits);
    if (!isfinite(body_parameter)||!isfinite(strength)||
        strength<0.0f) {
        *result=local;
        return false;
    }
    /* Exact CPU float-expression shape, with portable IEEE-754 math.
       TGP operations remain delegated to the original device/provider. */
    magnitude=60.0f+(body_parameter-60.0f)*0.30000001f;
    if (!isfinite(magnitude)||magnitude==0.0f) {
        *result=local;
        return false;
    }
    magnitude=(strength/magnitude)*4.0f;
    if (!isfinite(magnitude)||magnitude<0.0f) {
        *result=local;
        return false;
    }
    local.initial_magnitude_bits=asbits(magnitude);
    first_angle=(uint32_t)(UINT32_C(0)-(uint32_t)combined_heading);

    if (!device(tgp,tgp_user_data,UINT32_C(0x0D001A1A),1u,
                local.initial_magnitude_bits,0u,&replies[0])) goto fail;
    ++local.completed_commands;
    if (!device(tgp,tgp_user_data,UINT32_C(0x12002424),2u,
                0u,replies[0],&replies[1])) goto fail;
    ++local.completed_commands;
    if (!device(tgp,tgp_user_data,UINT32_C(0x12802525),2u,
                0u,replies[0],&replies[2])) goto fail;
    ++local.completed_commands;
    if (!device(tgp,tgp_user_data,UINT32_C(0x12002424),2u,
                first_angle,replies[2],&replies[3])) goto fail;
    ++local.completed_commands;
    if (!device(tgp,tgp_user_data,UINT32_C(0x12802525),2u,
                first_angle,replies[2],&replies[4])) goto fail;
    ++local.completed_commands;

    local.unboosted[0]=replies[3];
    local.unboosted[1]=replies[1];
    local.unboosted[2]=replies[4];

    for(i=0u;i<3u;++i) {
        float v=asfloat(local.unboosted[i]);
        if (boosted) {
            /* ldl flt_2B904[hit_kind*8], r8 loads vertical first,
               horizontal second (r9), not a uniform strength boost. */
            const uint32_t scale=rd32(
                boost_table+scale_offset+(i==1u?0u:4u));
            v*=asfloat(scale);
        }
        if(!isfinite(v)) goto fail;
        local.resulting[i]=asbits(v);
    }

    if (expected_xyz_bits != NULL) {
        stf_tgp_vector_difference comparison;
        if (!stf_tgp_command_trace_compare_vectors(
                expected_xyz_bits, local.resulting, &comparison)) {
            goto fail;
        }
        local.vectors_checked = true;
        local.mismatch_mask = comparison.mismatch_mask;
        memcpy(local.ulp_distance, comparison.ulp_distance,
               sizeof(local.ulp_distance));
        if (local.mismatch_mask != 0u) {
            goto fail;
        }
    }

    wr32(defender+0x5E0u,local.resulting[0]);
    wr32(defender+0x5E4u,local.resulting[1]);
    wr32(defender+0x5E8u,local.resulting[2]);
    *result=local;
    return true;

fail:
    *result=local;
    return false;
}

/* Preserve all prior opt-out semantics for existing consumers. */
bool stf_attack_hit_guard_vector_apply_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    int32_t combined_heading,
    uint32_t strength_r9_bits,
    uint8_t hit_kind,
    const uint8_t *boost_table,
    size_t boost_table_size,
    stf_attack_hit_guard_vector_tgp_fn tgp,
    void *tgp_user_data,
    stf_attack_hit_guard_vector_result *result
)
{
    return stf_attack_hit_guard_vector_apply_verified_model2(
        attacker, attacker_size, defender, defender_size,
        combined_heading, strength_r9_bits, hit_kind,
        boost_table, boost_table_size, tgp, tgp_user_data,
        NULL, result
    );
}
