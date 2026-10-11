#include "collision_area_side.h"

#include <math.h>
#include <string.h>

static uint16_t read16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8u));
}
static uint32_t read32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) |
           ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}
static void write16(uint8_t *p,uint16_t value)
{
    p[0]=(uint8_t)value;p[1]=(uint8_t)(value>>8u);
}
static void write32(uint8_t *p,uint32_t value)
{
    p[0]=(uint8_t)value;p[1]=(uint8_t)(value>>8u);
    p[2]=(uint8_t)(value>>16u);p[3]=(uint8_t)(value>>24u);
}
static float as_float(uint32_t bits)
{
    float value;
    memcpy(&value,&bits,sizeof(value));
    return value;
}

bool stf_collision_area_side_apply_model2(
    uint8_t *fighter,
    size_t fighter_size,
    uint8_t *workspace,
    size_t workspace_size,
    const uint8_t *angle_lut,
    size_t angle_lut_size,
    stf_collision_area_side_result *result
)
{
    stf_collision_area_side_result local;
    uint32_t axis_bits, effective_angle, sign, sector, seed, flags;
    int32_t signed_heading;
    int32_t encoded_direction=0;
    uint32_t minimum_bits;
    float minimum_float;
    unsigned i, slot;

    if (fighter==NULL || workspace==NULL || angle_lut==NULL || result==NULL ||
        fighter_size<STF_COLLISION_AREA_SIDE_FIGHTER_MIN_SIZE ||
        workspace_size<STF_COLLISION_AREA_SIDE_WORKSPACE_MIN_SIZE ||
        angle_lut_size<STF_COLLISION_AREA_ANGLE_LUT_COUNT ||
        fighter[4u]>1u) {
        return false;
    }

    memset(&local,0,sizeof(local));
    if (!stf_collision_area_plane_choose_model2(
            fighter,fighter_size,&local.plane)) {
        return false;
    }
    if (local.plane==STF_COLLISION_AREA_PLANE_NONE) {
        *result=local;
        return true;
    }

    slot=fighter[4u];
    signed_heading=(int32_t)(int16_t)read16(fighter+0x26u);
    local.heading_1f0=(uint16_t)signed_heading;
    axis_bits=read32(fighter+(local.plane==STF_COLLISION_AREA_PLANE_X
        ? 0x644u : 0x64Cu));
    if (local.plane==STF_COLLISION_AREA_PLANE_X) {
        /* r4 is signed ldis; sign comes from (r4 xor X_bits) >>28 &8 */
        effective_angle=(uint32_t)signed_heading;
        sign=((effective_angle ^ axis_bits)>>28u)&8u;
        seed=0u; /* replaced by the selected orientation below */
    } else {
        /* 0x4000 - signed heading; XOR with upper half of Z float. */
        effective_angle=UINT32_C(0x4000)-(uint32_t)signed_heading;
        sign=((effective_angle ^ (axis_bits>>16u))>>12u)&8u;
        /* At loc_2E074, r12 still contains sign-cleared +0x64C. */
        seed=read32(fighter+0x64Cu)&UINT32_C(0x7FFFFFFF);
    }
    sector=(effective_angle>>12u)&7u;
    local.lut_index=(uint8_t)(sector | sign);
    local.orientation_code=(uint8_t)(angle_lut[local.lut_index]|sign);
    if (local.plane==STF_COLLISION_AREA_PLANE_X) {
        seed=local.orientation_code;
    }
    flags=read32(fighter+0xA08u) | (sign<<8u);

    /* loc_2E030..loc_2E060: read four float planes, keep the minimum.
     * cmpr/bl is strictly less, so ties choose the later entry.
     * subi r11,0,r10; addi 4,r10,r10 => i=1:-8, i=2:-4, i=3:0.
     */
    minimum_bits=read32(fighter+0x650u);
    minimum_float=as_float(minimum_bits);
    if (!isfinite(minimum_float)) {
        return false;
    }
    for(i=1u;i<4u;++i) {
        const uint32_t current_bits=read32(fighter+0x650u+i*4u);
        const float current=as_float(current_bits);
        if (!isfinite(current)) {
            return false;
        }
        if (!(minimum_float < current)) {
            minimum_float=current;
            minimum_bits=current_bits;
            local.min_plane_index=(uint16_t)i;
            encoded_direction=(int32_t)(4*(int)i-12);
        }
    }
    local.minimum_bits=minimum_bits;
    local.minimum_record_offset=encoded_direction;
    local.selected_1f8=(uint16_t)(
        ((uint32_t)encoded_direction<<4u) | (seed&7u));
    local.next_flags_a08=flags;
    local.applied=true;

    /* All sizes and candidate plane words were validated before writes. */
    write16(workspace+0x1F0u+slot*2u,local.heading_1f0);
    write32(fighter+0xA08u,flags);
    write16(workspace+0x23Cu+slot*2u,UINT16_C(2));
    write16(workspace+0x1F4u+slot*2u,local.orientation_code);
    write16(fighter+0xAE2u,
        (uint16_t)(local.orientation_code&UINT8_C(0xF7)));
    write16(workspace+0x1F8u+slot*2u,local.selected_1f8);
    *result=local;
    return true;
}
