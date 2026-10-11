#include <math.h>
#include <stdint.h>
#include <string.h>

#include "collision_area.h"
#include "collision_area_distance.h"

enum { FIGHTER_SIZE = 0x1D20u, WORKSPACE_SIZE = 0x2B0u };

static void put32(uint8_t *p, uint32_t n)
{
    p[0]=(uint8_t)n; p[1]=(uint8_t)(n>>8u);
    p[2]=(uint8_t)(n>>16u); p[3]=(uint8_t)(n>>24u);
}
static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8u) |
        ((uint32_t)p[2]<<16u) | ((uint32_t)p[3]<<24u);
}
static uint32_t fb(float f)
{
    uint32_t u;
    memcpy(&u,&f,sizeof(u));
    return u;
}

static int test_query_and_distance_semantics(void)
{
    uint8_t fighter[FIGHTER_SIZE]={0u};
    stf_collision_area_distance_query q;
    uint32_t distance=0;
    stf_collision_area_plane side;
    put32(fighter+0x1F4u,fb(3.0f));
    put32(fighter+0x1F8u,fb(4.0f));
    put32(fighter+0x1FCu,fb(0.0f));
    if (!stf_collision_area_distance_query_build_model2(
            fighter,sizeof(fighter),&q) ||
        q.words[0]!=UINT32_C(0x15802B2B) ||
        q.words[1]!=fb(3.0f) || q.words[2]!=fb(4.0f) ||
        q.words[3]!=0u || q.words[4]!=0u ||
        q.words[5]!=0u || q.words[6]!=0u) return 1;
    if (!stf_collision_area_distance_semantic_bits(&q,&distance) ||
        distance != fb(5.0f) ||
        !stf_collision_area_distance_response_apply_model2(
            fighter,sizeof(fighter),distance) ||
        get32(fighter+0x680u)!=distance) return 2;

    q.words[4]=fb(3.0f);
    q.words[5]=fb(4.0f);
    if (!stf_collision_area_distance_semantic_bits(&q,&distance) ||
        distance != fb(0.0f)) return 3;
    q.words[0]=0u;
    if (stf_collision_area_distance_semantic_bits(&q,&distance)) return 4;
    q.words[0]=UINT32_C(0x15802B2B);
    q.words[3]=UINT32_C(0x7FC00000);
    if (stf_collision_area_distance_semantic_bits(&q,&distance)) return 5;
    if (stf_collision_area_distance_query_build_model2(
            fighter,STF_COLLISION_AREA_DISTANCE_FIGHTER_MIN_SIZE-1u,&q) ||
        stf_collision_area_distance_query_build_model2(
            fighter,sizeof(fighter),NULL)) return 6;

    put32(fighter+0x680u,UINT32_C(0x11223344));
    if (stf_collision_area_distance_response_apply_model2(
            fighter,sizeof(fighter),UINT32_C(0x7F800000)) ||
        stf_collision_area_distance_response_apply_model2(
            fighter,sizeof(fighter),fb(-1.0f)) ||
        stf_collision_area_distance_response_apply_model2(
            fighter,STF_COLLISION_AREA_DISTANCE_FIGHTER_MIN_SIZE-1u,
            fb(2.0f)) ||
        get32(fighter+0x680u)!=UINT32_C(0x11223344)) return 7;

    put32(fighter+0x644u,fb(1.0f));
    put32(fighter+0x64Cu,fb(2.0f));
    if (!stf_collision_area_plane_choose_model2(
            fighter,sizeof(fighter),&side) ||
        side!=STF_COLLISION_AREA_PLANE_X) return 8;
    put32(fighter+0x644u,fb(-2.0f));
    put32(fighter+0x64Cu,fb(-1.0f));
    if (!stf_collision_area_plane_choose_model2(
            fighter,sizeof(fighter),&side) ||
        side!=STF_COLLISION_AREA_PLANE_Z) return 9;
    put32(fighter+0x644u,0u);put32(fighter+0x64Cu,fb(2.0f));
    if (!stf_collision_area_plane_choose_model2(
            fighter,sizeof(fighter),&side) ||
        side!=STF_COLLISION_AREA_PLANE_NONE) return 10;
    put32(fighter+0x644u,fb(2.0f));put32(fighter+0x64Cu,0u);
    if (!stf_collision_area_plane_choose_model2(
            fighter,sizeof(fighter),&side) ||
        side!=STF_COLLISION_AREA_PLANE_X) return 11;
    put32(fighter+0x644u,0u);
    if (!stf_collision_area_plane_choose_model2(
            fighter,sizeof(fighter),&side) ||
        side!=STF_COLLISION_AREA_PLANE_NONE) return 12;
    put32(fighter+0x644u,fb(1.0f));put32(fighter+0x64Cu,fb(1.0f));
    if (!stf_collision_area_plane_choose_model2(
            fighter,sizeof(fighter),&side) ||
        side!=STF_COLLISION_AREA_PLANE_X) return 13;
    if (stf_collision_area_plane_choose_model2(
            fighter,0x64Fu,&side) ||
        stf_collision_area_plane_choose_model2(
            fighter,sizeof(fighter),NULL)) return 14;
    return 0;
}

typedef struct geometry_test {
    unsigned count;
    unsigned geometry_count;
    unsigned distance_count;
    unsigned distance_fail_at;
    uint8_t order[4];
} geometry_test;

static bool geometry_query(
    void *opaque, uint8_t fighter_index,
    const stf_collision_area_query *query,
    uint32_t response[STF_COLLISION_AREA_RESPONSE_WORDS]
)
{
    geometry_test *ctx=(geometry_test*)opaque;
    unsigned i;
    if (ctx->count>=4u || fighter_index>1u || query==NULL ||
        query->words[0]!=UINT32_C(0x38007070)) return false;
    ctx->order[ctx->count++]=(uint8_t)(10u+fighter_index);
    ++ctx->geometry_count;
    for (i=0u;i<STF_COLLISION_AREA_RESPONSE_WORDS;++i) response[i]=0u;
    response[6u]=fb(fighter_index==0u?1.0f:2.0f);
    response[8u]=fb(fighter_index==0u?2.0f:1.0f);
    return true;
}

static bool distance_query(
    void *opaque, uint8_t fighter_index,
    const stf_collision_area_distance_query *query,
    uint32_t *response_bits
)
{
    geometry_test *ctx=(geometry_test*)opaque;
    if (ctx->count>=4u || fighter_index>1u || query==NULL ||
        query->words[0]!=UINT32_C(0x15802B2B) ||
        response_bits==NULL) return false;
    ctx->order[ctx->count++]=(uint8_t)(20u+fighter_index);
    ++ctx->distance_count;
    if (ctx->distance_count==ctx->distance_fail_at) return false;
    return stf_collision_area_distance_semantic_bits(query,response_bits);
}

static int test_two_fighter_integration(void)
{
    uint8_t f0[FIGHTER_SIZE], f1[FIGHTER_SIZE];
    uint8_t ram[WORKSPACE_SIZE];
    stf_copro_attack_rom_view view={0};
    stf_collision_copro_area_model2 model;
    stf_collision_copro_area_result result;
    geometry_test ctx;
    memset(f0,0,sizeof(f0));
    memset(f1,0,sizeof(f1));
    memset(ram,0xAE,sizeof(ram));
    memset(&model,0,sizeof(model));
    memset(&ctx,0,sizeof(ctx));
    f1[4u]=1u;
    f0[0x1A18u]=2u;f1[0x1A18u]=2u;
    put32(f0+0x1F4u,fb(3.0f));put32(f0+0x1F8u,fb(4.0f));
    put32(f1+0x1FCu,fb(5.0f));
    model.fighter[0]=f0;model.fighter_size[0]=sizeof(f0);
    model.fighter[1]=f1;model.fighter_size[1]=sizeof(f1);
    model.workspace=ram;model.workspace_size=sizeof(ram);
    model.rom_view=&view;
    model.geometry=geometry_query;
    model.geometry_user_data=&ctx;
    model.distance=distance_query;
    model.distance_user_data=&ctx;
    model.cage_height_bits=fb(10.0f);
    if (!stf_collision_copro_area_stage_model2(&model,&result) ||
        result.completed_areas!=2u || result.completed_distances!=2u ||
        ctx.geometry_count!=2u || ctx.distance_count!=2u ||
        ctx.order[0]!=10u || ctx.order[1]!=20u ||
        ctx.order[2]!=11u || ctx.order[3]!=21u ||
        get32(f0+0x680u)!=fb(5.0f) ||
        get32(f1+0x680u)!=fb(5.0f) ||
        result.plane[0]!=STF_COLLISION_AREA_PLANE_X ||
        result.plane[1]!=STF_COLLISION_AREA_PLANE_Z) return 1;

    /* If second scalar query fails, first area+distance and second
       area remain visible; the failing scalar reply is NOT committed. */
    memset(&ctx,0,sizeof(ctx));
    ctx.distance_fail_at=2u;
    put32(f1+0x680u,UINT32_C(0x12345678));
    if (stf_collision_copro_area_stage_model2(&model,&result) ||
        result.completed_areas!=2u || result.completed_distances!=1u ||
        ctx.geometry_count!=2u || ctx.distance_count!=2u ||
        get32(f1+0x680u)!=UINT32_C(0x12345678)) return 2;

    /* A failed first distance prevents the second area query. */
    memset(&ctx,0,sizeof(ctx));
    ctx.distance_fail_at=1u;
    if (stf_collision_copro_area_stage_model2(&model,&result) ||
        result.completed_areas!=1u || result.completed_distances!=0u ||
        ctx.geometry_count!=1u || ctx.distance_count!=1u) return 3;

    /* Backward-compatible prefix-only mode has no extra device calls. */
    memset(&ctx,0,sizeof(ctx));
    model.distance=NULL;
    if (!stf_collision_copro_area_stage_model2(&model,&result) ||
        result.completed_areas!=2u || result.completed_distances!=0u ||
        ctx.distance_count!=0u || ctx.geometry_count!=2u) return 4;
    return 0;
}

int main(void)
{
    int rc=test_query_and_distance_semantics();
    if (rc != 0) return rc;
    rc=test_two_fighter_integration();
    if (rc != 0) return rc+100;
    return 0;
}
