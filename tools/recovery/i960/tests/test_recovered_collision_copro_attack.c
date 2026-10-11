#include <stdint.h>
#include <string.h>

#include "collision_copro_attack.h"
#include "collision_frame.h"

enum { TEST_FIGHTER_SIZE = 0x1E20u };

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

int main(void)
{
    uint8_t f0[TEST_FIGHTER_SIZE];
    uint8_t f1[TEST_FIGHTER_SIZE];
    uint8_t image[0x400u];
    uint8_t workspace[STF_COLLISION_ATTACK_WORKSPACE_MODEL2_MIN_SIZE];
    uint8_t effect[STF_COLLISION_ATTACK_EFFECT_MODEL2_SIZE];
    uint8_t copro_xyz[12u];
    uint8_t r5_0[12u], r5_1[12u];
    uint16_t mapping[STF_COLLISION_ATTACK_MAPPING_COUNT];
    stf_copro_attack_rom_view view;
    stf_copro_attack_flag_result flags[2], one;
    stf_collision_frame_model2 frame;
    stf_collision_frame_result collision;
    unsigned i;

    memset(f0, 0, sizeof(f0));
    memset(f1, 0, sizeof(f1));
    memset(image, 0, sizeof(image));
    memset(workspace, 0xAB, sizeof(workspace));
    memset(effect, 0xCD, sizeof(effect));
    memset(copro_xyz, 0, sizeof(copro_xyz));
    memset(r5_0, 0, sizeof(r5_0));
    memset(r5_1, 0, sizeof(r5_1));
    memset(mapping, 0, sizeof(mapping));
    memset(&view, 0, sizeof(view));
    memset(&frame, 0, sizeof(frame));

    view.image=image;
    view.image_size=sizeof(image);
    view.image_base=UINT32_C(0x000CE000);
    view.profile_table_address=UINT32_C(0x000CE020);
    view.profile_count=42u;
    wr32(image+0x20u+1u*4u, UINT32_C(1)<<4u);
    wr32(image+0x20u+2u*4u, UINT32_C(1)<<7u);
    for (i=0u;i<32u;++i) {
        /* Most descriptors select an absent profile bit. */
        wr32(image+0x200u+i*4u,15u);
        wr32(image+0x300u+i*4u,15u);
    }
    wr32(image+0x200u+2u*4u,4u);
    wr32(image+0x300u+3u*4u,39u); /* tests i960 bit-index modulo 32 */

    f1[4u]=1u;
    f0[0x820u]=1u;
    f1[0x820u]=2u;
    wr32(f0+0x1D18u,view.image_base+0x200u);
    wr32(f1+0x1D18u,view.image_base+0x300u);
    wr32(f0+0x1A4u,UINT32_C(1)<<8u);
    wr32(f1+0x1A4u,UINT32_C(1)<<8u);
    wr16(f0+0x1AAu,10u); wr16(f1+0x1AAu,10u);
    wr16(f0+0x808u,5u); wr16(f1+0x808u,5u);
    wr16(f0+0x1A8u,3u);wr16(f1+0x1A8u,9u);
    wr32(f1+0x860u,UINT32_C(1)<<22u);
    wr16(workspace+0x8Eu,9u);
    mapping[4u]=UINT16_C(1)<<3u;
    mapping[5u]=UINT16_C(1)<<7u;
    for (i=0u;i<12u;++i) {
        f0[0x1F4u+5u*12u+i]=(uint8_t)(0x70u+i);
        f1[0x1F4u+3u*12u+i]=(uint8_t)(0x60u+i);
    }

    /* A bad second selector pointer leaves first fighter and task unchanged. */
    wr32(f1+0x1D18u,view.image_base+0x3C0u);
    if (stf_collision_copro_attack_prepare_model2(
            workspace,sizeof(workspace),f0,sizeof(f0),f1,sizeof(f1),
            &view,flags) || rd32(f0+0xAC8u)!=0u ||
        rd32(f0+0x768u)!=0u || workspace[0x170u]!=0xABu) return 1;
    wr32(f1+0x1D18u,view.image_base+0x300u);

    if (!stf_collision_copro_attack_prepare_model2(
            workspace,sizeof(workspace),f0,sizeof(f0),f1,sizeof(f1),
            &view,flags)) return 2;
    if (!flags[0].eligible || !flags[1].eligible ||
        flags[0].attack_units_0768!=(UINT32_C(1)<<2u) ||
        flags[1].attack_units_0768!=(UINT32_C(1)<<3u) ||
        flags[0].attack_profile_0ac8!=(UINT32_C(1)<<4u) ||
        flags[1].attack_profile_0ac8!=(UINT32_C(1)<<7u) ||
        rd32(f0+0x768u)!=(UINT32_C(1)<<2u) ||
        rd32(f1+0x768u)!=(UINT32_C(1)<<3u) ||
        rd32(f0+0xAC8u)!=(UINT32_C(1)<<4u) ||
        rd32(f1+0xAC8u)!=(UINT32_C(1)<<7u) ||
        rd32(workspace+0x170u)!=0u || rd32(workspace+0x98u)!=0u) return 3;

    /* Build the already-recovered pair check from actual computed +0xAC8. */
    frame.fighters[0]=f0;frame.fighters[1]=f1;
    frame.fighter_sizes[0]=sizeof(f0);frame.fighter_sizes[1]=sizeof(f1);
    frame.workspace=workspace;frame.workspace_size=sizeof(workspace);
    frame.mapping=mapping;
    frame.r5_selected_xyz[0]=r5_0;frame.r5_selected_xyz_sizes[0]=12u;
    frame.r5_selected_xyz[1]=r5_1;frame.r5_selected_xyz_sizes[1]=12u;
    frame.effect=effect;frame.effect_size=sizeof(effect);
    frame.copro_hit_xyz=copro_xyz;frame.copro_hit_xyz_size=sizeof(copro_xyz);
    wr32(workspace+0x2ACu,0u);
    wr16(workspace+0x90u,0u);
    if (!stf_collision_frame_check_computed_model2(&frame,&collision) ||
        !collision.checks[0].hit || !collision.checks[1].hit ||
        collision.checks[0].selected_unit!=3u ||
        collision.checks[1].selected_unit!=5u ||
        collision.order.count!=2u || collision.order.attacker[0]!=1u ||
        collision.order.attacker[1]!=0u) return 4;

    /* Inactive kind 41 bypasses the table, storing zero profile/mask. */
    f0[0x820u]=41u;
    if (!stf_calc_attack_flag_apply_model2(
            f0,sizeof(f0),&view,&one) || one.eligible ||
        rd32(f0+0xAC8u)!=0u || rd32(f0+0x768u)!=0u) return 5;
    /* Disabled attack and early timing both write zeros. */
    f0[0x820u]=1u;
    wr32(f0+0x1A4u,0u);
    if (!stf_calc_attack_flag_apply_model2(
            f0,sizeof(f0),&view,&one) || one.eligible) return 6;
    wr32(f0+0x1A4u,UINT32_C(1)<<8u);
    wr16(f0+0x1AAu,1u);
    if (!stf_calc_attack_flag_apply_model2(
            f0,sizeof(f0),&view,&one) || one.eligible) return 7;
    wr16(f0+0x1AAu,10u);

    /* OOB access is rejected without changing the live fighter fields. */
    wr32(f0+0xAC8u,UINT32_C(0x12345678));
    view.profile_count=1u;
    if (stf_calc_attack_flag_apply_model2(
            f0,sizeof(f0),&view,&one) ||
        rd32(f0+0xAC8u)!=UINT32_C(0x12345678)) return 8;
    view.profile_count=42u;
    if (stf_calc_attack_flag_read_model2(
            f0,sizeof(f0)-1u,&view,&one)) return 9;
    if (stf_collision_copro_attack_prepare_model2(
            workspace,sizeof(workspace)-1u,f0,sizeof(f0),f1,sizeof(f1),
            &view,flags)) return 10;

    /* Frame API catches undersized fighters before mutation. */
    frame.fighter_sizes[0]=STF_COLLISION_ATTACK_FIGHTER_MODEL2_MIN_SIZE;
    if (stf_collision_frame_check_computed_model2(&frame,&collision)) return 11;
    return 0;
}
