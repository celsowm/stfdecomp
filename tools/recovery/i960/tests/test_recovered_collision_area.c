#include <math.h>
#include <stdint.h>
#include <string.h>
#include "collision_area.h"

enum { TEST_FIGHTER_SIZE = 0x1D20u, TEST_TASK_SIZE = 0x2B0u };

static void w16(uint8_t *p, uint16_t n)
{
    p[0]=(uint8_t)n; p[1]=(uint8_t)(n>>8u);
}
static void w32(uint8_t *p, uint32_t n)
{
    p[0]=(uint8_t)n; p[1]=(uint8_t)(n>>8u);
    p[2]=(uint8_t)(n>>16u); p[3]=(uint8_t)(n>>24u);
}
static uint32_t r32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8u) |
        ((uint32_t)p[2]<<16u) | ((uint32_t)p[3]<<24u);
}
static uint32_t float_to_bits(float f)
{
    uint32_t bits;
    memcpy(&bits,&f,sizeof(bits));
    return bits;
}
static float bits_to_float(uint32_t bits)
{
    float f;
    memcpy(&f,&bits,sizeof(f));
    return f;
}
static void make_response(uint32_t data[STF_COLLISION_AREA_RESPONSE_WORDS], uint8_t index)
{
    size_t i;
    memset(data,0,STF_COLLISION_AREA_RESPONSE_WORDS*sizeof(uint32_t));
    for (i=0u;i<4u;++i) data[i]=UINT32_C(0x11223300)+(uint32_t)i+index*16u;
    data[4]=UINT32_C(0x40000000);
    data[5]=UINT32_C(0x40800000);
    data[6]=float_to_bits(2.0f);
    data[7]=UINT32_C(0x80000000);
    data[8]=float_to_bits(3.0f);
    data[9]=float_to_bits(4.0f);
    data[10]=float_to_bits(5.0f);
    data[11]=float_to_bits(6.0f);
    data[12]=float_to_bits(7.0f);
    data[13]=float_to_bits(8.0f);
    data[14]=10u+index; data[15]=20u+index;
    data[16]=30u+index; data[17]=40u+index;
}

typedef struct geometry_fixture {
    uint8_t calls;
    uint8_t indices[2];
    uint8_t fail_at;
    stf_collision_area_query queries[2];
} geometry_fixture;

static bool geometry(
    void *ptr, uint8_t fighter_index,
    const stf_collision_area_query *query,
    uint32_t response[STF_COLLISION_AREA_RESPONSE_WORDS]
)
{
    geometry_fixture *f=(geometry_fixture *)ptr;
    if (f->calls>=2u || query==NULL || fighter_index>1u) return false;
    f->indices[f->calls]=fighter_index;
    f->queries[f->calls]=*query;
    ++f->calls;
    if (f->fail_at == f->calls) return false;
    make_response(response,fighter_index);
    return true;
}

static int query_checks(void)
{
    uint8_t f[TEST_FIGHTER_SIZE]={0u};
    stf_collision_area_query q;
    uint32_t height=float_to_bits(20.0f);
    w32(f+0x1Cu,UINT32_C(0xABCD1234));
    if (!stf_collision_area_query_build_model2(
            f,sizeof(f),10u,11u,height,&q)) return 1;
    if (q.words[0]!=UINT32_C(0x38007070) ||
        q.words[1]!=UINT32_C(0xABCD1234) ||
        q.words[2]!=10u || q.words[3]!=11u || q.words[5]!=0u ||
        fabsf(bits_to_float(q.words[4])-29.81f)>0.0001f) return 2;

    f[0x1A18u]=2u;
    if (!stf_collision_area_query_build_model2(
            f,sizeof(f),10u,11u,height,&q) ||
        fabsf(bits_to_float(q.words[4])-19.81f)>0.0001f ||
        q.words[5]!=0u) return 3;
    f[0x1A18u]=3u; f[4u]=1u;
    if (!stf_collision_area_query_build_model2(
            f,sizeof(f),10u,11u,height,&q) ||
        q.words[5]!=UINT32_C(0x80000001)) return 4;
    f[0x1A18u]=0u;
    w32(f+0x70Cu,UINT32_C(1)<<9u);
    if (!stf_collision_area_query_build_model2(
            f,sizeof(f),10u,11u,height,&q) ||
        fabsf(bits_to_float(q.words[4])-19.81f)>0.0001f) return 5;
    if (stf_collision_area_query_build_model2(
            f,STF_COLLISION_AREA_FIGHTER_MIN_SIZE-1u,
            0u,0u,height,&q) ||
        stf_collision_area_query_build_model2(
            f,sizeof(f),0u,0u,height,NULL)) return 6;
    return 0;
}

static int response_checks(void)
{
    uint8_t f[TEST_FIGHTER_SIZE];
    uint8_t workspace[TEST_TASK_SIZE];
    uint32_t response[STF_COLLISION_AREA_RESPONSE_WORDS];
    stf_collision_area_result out;

    memset(f,0xCC,sizeof(f));
    memset(workspace,0xCC,sizeof(workspace));
    make_response(response,0u);
    f[4u]=0u; f[0x1A18u]=0u; f[0xA29u]=0u;

    if (stf_collision_area_response_apply_model2(
            f,sizeof(f),workspace,sizeof(workspace),response,
            STF_COLLISION_AREA_RESPONSE_WORDS-1u,false,&out) ||
        f[0x660u]!=0xCCu || workspace[0x118u]!=0xCCu) return 1;
    if (!stf_collision_area_response_apply_model2(
            f,sizeof(f),workspace,sizeof(workspace),response,
            STF_COLLISION_AREA_RESPONSE_WORDS,false,&out)) return 2;
    if (out.fighter_index!=0u || out.inverted_phase ||
        out.plane_644!=float_to_bits(2.0f) ||
        out.plane_64c!=float_to_bits(-1.0f) ||
        out.radius_624!=0u) return 3;
    if (r32(workspace+0x118u)!=response[0] ||
        r32(workspace+0x124u)!=response[3] ||
        r32(f+0x670u)!=response[4] || r32(f+0x710u)!=response[5] ||
        r32(f+0x660u)!=float_to_bits(2.0f) ||
        r32(f+0x664u)!=0u ||
        r32(f+0x668u)!=float_to_bits(3.0f) ||
        r32(f+0x66Cu)!=float_to_bits(-4.0f) ||
        r32(f+0xA58u)!=response[10] ||
        r32(f+0xA64u)!=response[13] ||
        r32(f+0x650u)!=0u || r32(f+0x654u)!=20u ||
        r32(f+0x658u)!=30u || r32(f+0x65Cu)!=0u ||
        r32(f+0x614u)!=10u || r32(f+0x674u)!=20u ||
        r32(f+0x624u)!=0u || r32(f+0x618u)!=40u) return 4;

    /* Phase 3 substitutes secondary tangent and negates X and Z. */
    f[0x1A18u]=3u; f[0xA29u]=1u; f[4u]=1u;
    if (!stf_collision_area_response_apply_model2(
            f,sizeof(f),workspace,sizeof(workspace),response,
            STF_COLLISION_AREA_RESPONSE_WORDS,false,&out) ||
        !out.inverted_phase ||
        r32(f+0x660u)!=float_to_bits(-5.0f) ||
        r32(f+0x664u)!=float_to_bits(6.0f) ||
        r32(f+0x668u)!=float_to_bits(-7.0f) ||
        r32(f+0x66Cu)!=float_to_bits(8.0f) ||
        r32(f+0x644u)!=float_to_bits(1.0f) ||
        r32(f+0x64Cu)!=float_to_bits(1.0f) ||
        r32(f+0x624u)!=30u) return 5;

    /* Special game mode suppresses the slope vector and clipping writes. */
    if (!stf_collision_area_response_apply_model2(
            f,sizeof(f),workspace,sizeof(workspace),response,
            STF_COLLISION_AREA_RESPONSE_WORDS,true,&out) ||
        r32(f+0x644u)!=0u || r32(f+0x64Cu)!=0u ||
        r32(f+0x650u)!=10u || r32(f+0x658u)!=30u) return 6;

    /* Nonfinite response is rejected before any side effect. */
    response[10]=UINT32_C(0x7FC00000);
    workspace[0x118u]=0xBDu;
    if (stf_collision_area_response_apply_model2(
            f,sizeof(f),workspace,sizeof(workspace),response,
            STF_COLLISION_AREA_RESPONSE_WORDS,false,&out) ||
        workspace[0x118u]!=0xBDu) return 7;

    if (stf_collision_area_response_apply_model2(
            f,sizeof(f),workspace,STF_COLLISION_AREA_WORKSPACE_MIN_SIZE-1u,
            response,STF_COLLISION_AREA_RESPONSE_WORDS,false,&out)) return 8;
    return 0;
}

static int stage_checks(void)
{
    uint8_t f0[TEST_FIGHTER_SIZE], f1[TEST_FIGHTER_SIZE];
    uint8_t ram[TEST_TASK_SIZE], image[0x400u];
    stf_copro_attack_rom_view view;
    stf_collision_copro_area_model2 model;
    stf_collision_copro_area_result output;
    geometry_fixture mock;
    size_t i;
    memset(f0,0,sizeof(f0));memset(f1,0,sizeof(f1));
    memset(ram,0x77,sizeof(ram));memset(image,0,sizeof(image));
    memset(&view,0,sizeof(view));memset(&model,0,sizeof(model));
    memset(&mock,0,sizeof(mock));

    view.image=image;view.image_size=sizeof(image);
    view.image_base=UINT32_C(0x000CE000);
    view.profile_table_address=view.image_base+0x20u;
    view.profile_count=42u;
    w32(image+0x20u+1u*4u,UINT32_C(1)<<3u);
    w32(image+0x20u+2u*4u,UINT32_C(1)<<4u);
    for (i=0u;i<32u;++i) {
        w32(image+0x200u+i*4u,15u);
        w32(image+0x300u+i*4u,15u);
    }
    w32(image+0x200u+2u*4u,3u);
    w32(image+0x300u+4u*4u,4u);

    f0[4u]=0u;f1[4u]=1u;
    f0[0x820u]=1u; f1[0x820u]=2u;
    w32(f0+0x1D18u,view.image_base+0x200u);
    w32(f1+0x1D18u,view.image_base+0x300u);
    w32(f0+0x1A4u,UINT32_C(1)<<8u);
    w32(f1+0x1A4u,UINT32_C(1)<<8u);
    w16(f0+0x1AAu,10u);w16(f1+0x1AAu,10u);
    w16(f0+0x808u,5u);w16(f1+0x808u,5u);

    model.fighter[0]=f0;model.fighter_size[0]=sizeof(f0);
    model.fighter[1]=f1;model.fighter_size[1]=sizeof(f1);
    model.workspace=ram;model.workspace_size=sizeof(ram);
    model.rom_view=&view;model.geometry=geometry;
    model.geometry_user_data=&mock;
    model.cage_height_bits=float_to_bits(5.0f);
    model.stage_soko_bits=float_to_bits(0.0f);
    model.stage_x_bits=float_to_bits(12.0f);

    model.fighter_size[1]=1u;
    if (stf_collision_copro_area_stage_model2(&model,&output) ||
        mock.calls!=0u || ram[0x170u]!=0x77u ||
        r32(f0+0xAC8u)!=0u) return 1;
    model.fighter_size[1]=sizeof(f1);

    if (!stf_collision_copro_area_stage_model2(&model,&output)) return 2;
    if (output.completed_areas!=2u || mock.calls!=2u ||
        mock.indices[0]!=0u || mock.indices[1]!=1u ||
        mock.queries[0].words[0]!=UINT32_C(0x38007070) ||
        mock.queries[1].words[5]!=1u ||
        output.attack[0].attack_units_0768!=(UINT32_C(1)<<2u) ||
        output.attack[1].attack_units_0768!=(UINT32_C(1)<<4u) ||
        r32(f0+0xAC8u)!=(UINT32_C(1)<<3u) ||
        r32(f1+0xAC8u)!=(UINT32_C(1)<<4u) ||
        r32(ram+0x170u)!=0u ||
        r32(ram+0x118u)!=UINT32_C(0x11223310) ||
        r32(f0+0x614u)!=10u || r32(f1+0x614u)!=11u) return 3;

    /* Second geometry failure retains the completed first area, fail-closed. */
    memset(&mock,0,sizeof(mock));
    mock.fail_at=2u;
    memset(ram,0x77,sizeof(ram));
    if (stf_collision_copro_area_stage_model2(&model,&output) ||
        output.completed_areas!=1u || mock.calls!=2u ||
        r32(ram+0x118u)!=UINT32_C(0x11223300) ||
        r32(ram+0x170u)!=0u) return 4;
    mock.fail_at=0u;mock.calls=0u;

    /* A malformed second selector table is rejected before RAM reset. */
    w32(f1+0x1D18u,view.image_base+0x3F0u);
    memset(ram,0x77,sizeof(ram));
    if (stf_collision_copro_area_stage_model2(&model,&output) ||
        mock.calls!=0u || ram[0x170u]!=0x77u) return 5;

    return 0;
}

int main(void)
{
    int status=query_checks();
    if (status!=0) return 10+status;
    status=response_checks();
    if (status!=0) return 20+status;
    status=stage_checks();
    if (status!=0) return 30+status;
    return 0;
}
