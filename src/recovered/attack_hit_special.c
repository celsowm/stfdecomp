#include "attack_hit_special.h"

#include <math.h>
#include <string.h>

static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8u));
}
static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) |
        ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}
static void wr32(uint8_t *p, uint32_t v)
{
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8u);
    p[2]=(uint8_t)(v>>16u); p[3]=(uint8_t)(v>>24u);
}
static float to_float(uint32_t bits)
{
    float f;
    memcpy(&f,&bits,sizeof(f));
    return f;
}
static uint32_t to_bits(float f)
{
    uint32_t bits;
    memcpy(&bits,&f,sizeof(bits));
    return bits;
}

bool stf_attack_hit_special_apply_model2(
    uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    const uint8_t *tag5_record,
    size_t tag5_record_size,
    stf_attack_hit_special_tgp_fn tgp,
    void *tgp_user_data,
    stf_attack_hit_special_result *result
)
{
    stf_attack_hit_special_result local;
    uint32_t attacker_state, defender_state, attacker_motion_flags;
    uint32_t next_state, next_defender_state, next_motion_flags;
    uint16_t motion, anim_word;
    uint32_t response[2] = {0u, 0u};
    uint32_t translated[2] = {0u, 0u};
    uint32_t height_bits = 0u;
    bool moving, requires_tgp;

    if (attacker == NULL || defender == NULL || tag5_record == NULL ||
        result == NULL ||
        attacker_size < STF_ATTACK_HIT_SPECIAL_ATTACKER_MIN_SIZE ||
        defender_size < STF_ATTACK_HIT_SPECIAL_DEFENDER_MIN_SIZE ||
        tag5_record_size < STF_ATTACK_HIT_SPECIAL_RECORD_MIN_SIZE) {
        return false;
    }

    memset(&local,0,sizeof(local));
    attacker_state=rd32(attacker);
    defender_state=rd32(defender);
    attacker_motion_flags=rd32(attacker+0x1A4u);
    motion=rd16(defender+0x19Cu);
    anim_word=rd16(tag5_record+1u);
    if ((motion & UINT16_C(0x8000)) != 0u) {
        anim_word ^= UINT16_C(0x8000);
    }
    moving=(attacker_state & (UINT32_C(1)<<2u)) != 0u;
    requires_tgp=moving && (attacker[0x83Fu]&1u)==0u;
    local.motion_19c=motion;
    local.animation_halfword=anim_word;
    local.animation_byte_822=tag5_record[3u];
    local.needed_tgp=requires_tgp;
    local.cleared_motion_flag=moving;

    /* The sub_1980C TGP request uses the heading and two motion
       vectors from attacker, after sub_19888 copied the opponent's
       +0x1C. The request arguments are otherwise unchanged. */
    if (requires_tgp) {
        stf_attack_hit_special_tgp_query query;
        float p18, p20;
        const float x18=to_float(rd32(attacker+0x18u));
        const float x20=to_float(rd32(attacker+0x20u));
        if (tgp==NULL) return false;
        query.opcode=UINT32_C(0x2D805B5B);
        query.heading_26=rd16(attacker+0x26u);
        query.vector_80=rd32(attacker+0x80u);
        query.vector_88=rd32(attacker+0x88u);
        if (!tgp(tgp_user_data,&query,response)) return false;
        p18=x18-to_float(response[0]);
        p20=x20-to_float(response[1]);
        if (!isfinite(p18)||!isfinite(p20)) return false;
        translated[0]=to_bits(p18);
        translated[1]=to_bits(p20);
    }
    if (moving) {
        float height=to_float(rd32(attacker+0x1F8u)) -
            to_float(rd32(defender+0x1Cu));
        if (!isfinite(height)) return false;
        height_bits=to_bits(height);
    }

    next_motion_flags=attacker_motion_flags & ~(UINT32_C(1)<<21u);
    next_state=attacker_state;
    if (moving) {
        next_state &= ~(UINT32_C(1)<<2u);
        next_motion_flags &= ~(UINT32_C(1)<<4u);
    }
    next_defender_state=defender_state & ~(UINT32_C(1)<<4u);
    next_state = (next_state & ~(UINT32_C(1)<<6u)) |
        ((defender_state & (UINT32_C(1)<<10u)) != 0u ?
          (UINT32_C(1)<<6u) : 0u);

    /* sub_19888 emits motion state before invoking sub_1980C.
       All optional device failures have already been checked. */
    wr32(attacker+0x198u,UINT32_C(0x0D000000)+anim_word);
    wr32(attacker+0x1Cu,rd32(defender+0x1Cu));
    if (moving) {
        if (requires_tgp) {
            wr32(attacker+0x18u,translated[0]);
            wr32(attacker+0x20u,translated[1]);
        }
        wr32(attacker+0x84u,height_bits);
    }
    attacker[0x822u]=local.animation_byte_822;
    wr32(defender+0x198u,UINT32_C(0x0C000000)+motion);
    wr32(defender,next_defender_state);
    wr32(attacker+0x1A4u,next_motion_flags);
    wr32(attacker,next_state);
    local.attacker_flags_000=next_state;
    local.attacker_flags_1a4=next_motion_flags;
    local.defender_flags_000=next_defender_state;
    *result=local;
    return true;
}
