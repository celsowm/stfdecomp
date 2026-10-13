#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "collision_combat_runtime.h"
#include "collision_particle_frame_runtime.h"

enum {
    F_SIZE = 0x2200u,
    W_SIZE = 0x2B0u,
    SLOTS_BYTES = STF_COLLISION_PARTICLE_EXTENDED_SLOTS *
                  STF_COLLISION_PARTICLE_SLOT_SIZE
};

#define CHECK(condition, scenario) do { \
    if (!(condition)) { \
        fprintf(stderr, "damage particle combat scenario %d failed at line %d\\n", \
                (scenario), __LINE__); \
        return (scenario); \
    } \
} while (0)

static void wr16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
}
static void wr32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
    p[2] = (uint8_t)(value >> 16u);
    p[3] = (uint8_t)(value >> 24u);
}
static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) |
        ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}

typedef struct fixture {
    unsigned calls;
    uint8_t particle_mask; /* bit (call-1) requests workspace bit 4 */
    uint8_t skip_mask;     /* bit (call-1) clears damage_unit bit 0 */
} fixture;

static bool body(
    void *user, uint8_t attacker_index, uint8_t defender_index,
    uint8_t *attacker, size_t attacker_size,
    uint8_t *defender, size_t defender_size,
    uint8_t *workspace, size_t workspace_size,
    const stf_attack_hit_prefix_result *prefix
)
{
    fixture *fx = (fixture *)user;
    uint32_t flags;
    unsigned mask;
    if (fx == NULL || attacker == NULL || defender == NULL ||
        workspace == NULL || prefix == NULL ||
        attacker_index == defender_index || attacker_size < F_SIZE ||
        defender_size < F_SIZE || workspace_size < W_SIZE ||
        fx->calls >= 2u) {
        return false;
    }
    mask = 1u << fx->calls++;
    flags = rd32(workspace + 0x26Cu);
    if ((fx->particle_mask & mask) != 0u) {
        flags |= UINT32_C(1) << 4u;
    }
    if ((fx->skip_mask & mask) != 0u) {
        flags &= ~UINT32_C(1);
    }
    wr32(workspace + 0x26Cu, flags);
    return true;
}

typedef struct scene {
    uint8_t fighters[2][F_SIZE];
    uint8_t enemies[2][STF_ATTACK_HIT_ENEMY_MODEL2_MIN_SIZE];
    uint8_t workspace[W_SIZE];
    uint8_t slots[SLOTS_BYTES];
    stf_collision_particle_descriptor
        descriptors[STF_COLLISION_PARTICLE_KIND_COUNT];
    stf_collision_particle_stage stage;
    stf_damage_unit_effect_state effect;
    stf_collision_combat_model2 model;
    stf_collision_hit_order order;
    stf_collision_combat_result result;
    fixture fx;
} scene;

static void init(scene *s)
{
    memset(s, 0, sizeof(*s));
    s->fighters[1][4u] = 1u;
    s->fighters[0][0x822u] = 7u;
    s->fighters[1][0x822u] = 11u;
    wr16(s->fighters[0] + 0x6F0u, UINT16_C(2));
    wr16(s->fighters[1] + 0x6F0u, UINT16_C(1));
    wr32(s->fighters[0] + 0xAF0u, UINT32_MAX);
    wr32(s->fighters[0] + 0xAF4u, UINT32_MAX);
    wr32(s->fighters[1] + 0xAF0u, UINT32_MAX);
    wr32(s->fighters[1] + 0xAF4u, UINT32_MAX);
    s->model.fighters[0] = s->fighters[0];
    s->model.fighters[1] = s->fighters[1];
    s->model.fighter_sizes[0] = F_SIZE;
    s->model.fighter_sizes[1] = F_SIZE;
    s->model.enemy_slots[0] = s->enemies[0];
    s->model.enemy_slots[1] = s->enemies[1];
    s->model.enemy_slot_sizes[0] = sizeof(s->enemies[0]);
    s->model.enemy_slot_sizes[1] = sizeof(s->enemies[1]);
    s->model.workspace = s->workspace;
    s->model.workspace_size = W_SIZE;
    s->model.effect = &s->effect;
    s->model.num_rounds_to_win = 2u;
    s->model.attack_body = body;
    s->model.attack_body_user_data = &s->fx;
    s->model.recover_damage_particles = true;
    s->model.damage_particle_descriptors = s->descriptors;
    s->model.damage_particle_slots = s->slots;
    s->model.damage_particle_slots_size = sizeof(s->slots);
    s->model.damage_particle_stage = &s->stage;
    s->descriptors[1].effect_address = UINT32_C(0x2000);
    s->descriptors[1].initial_value = UINT32_C(0x33445566);
    s->stage.position_a[0] = UINT32_C(0x11111111);
    s->stage.position_a[1] = UINT32_C(0x33333333);
    s->stage.position_b[2] = UINT32_C(0x22222222);
    s->stage.flags = UINT8_C(0x08);
    s->order.count = 1u;
    s->order.attacker[0] = 1u;
    s->fx.particle_mask = 1u;
}

int main(void)
{
    scene s;
    unsigned i;

    /* CPU damage_unit_post requests one effect and sub_32A5C allocates
     * slot zero, consuming staged positions and preserving the flag mask. */
    init(&s);
    CHECK(stf_collision_combat_apply_model2(&s.model, &s.order, &s.result), 1);
    CHECK(s.result.completed == 1u && s.result.attempted == 1u &&
          s.result.pairs[0].damage_completed &&
          s.result.pairs[0].damage.post_applied &&
          s.result.pairs[0].damage.post.request_particle_setup &&
          s.result.pairs[0].damage_particle_processed &&
          s.result.pairs[0].damage_particle.requested &&
          s.result.pairs[0].damage_particle.particle.allocated &&
          s.result.pairs[0].damage_particle.particle.slot_index == 0u &&
          rd32(s.slots + 0x00u) == UINT32_C(0x11111111) &&
          rd32(s.slots + 0x04u) == UINT32_C(0x33333333) &&
          rd32(s.slots + 0x14u) == UINT32_C(0x22222222) &&
          rd32(s.slots + 0x1Cu) == UINT32_C(0x2000) &&
          rd32(s.slots + 0x20u) == UINT32_C(0x33445566) &&
          s.slots[0x19u] == UINT8_C(0x09) &&
          s.stage.draw_particle == 0u &&
          s.stage.position_a[0] == 0u &&
          s.stage.position_b[2] == 0u &&
          s.stage.flags == UINT8_C(0x09), 2);

    /* No request still completes the post-damage flow, but must not
     * allocate or mutate any staged fields. */
    init(&s);
    s.fx.particle_mask = 0u;
    s.stage.draw_particle = UINT16_C(7);
    CHECK(stf_collision_combat_apply_model2(&s.model, &s.order, &s.result), 3);
    CHECK(s.result.pairs[0].damage_particle_processed &&
          !s.result.pairs[0].damage_particle.requested &&
          rd32(s.slots + 0x1Cu) == 0u &&
          s.stage.draw_particle == 7u &&
          s.stage.position_a[0] == UINT32_C(0x11111111) &&
          s.stage.flags == UINT8_C(0x08), 4);

    /* A body that clears the damage-unit bit-0 gate must suppress both
     * the damage post tail and the particle allocator. */
    init(&s);
    s.fx.skip_mask = 1u;
    CHECK(stf_collision_combat_apply_model2(&s.model, &s.order, &s.result), 5);
    CHECK(s.result.pairs[0].damage.damage.skipped &&
          !s.result.pairs[0].damage.post_applied &&
          s.result.pairs[0].damage_completed &&
          !s.result.pairs[0].damage_particle_processed &&
          rd32(s.slots + 0x1Cu) == 0u &&
          s.stage.position_a[0] == UINT32_C(0x11111111), 6);

    /* Two ordered hits share a pool. The first consumes the staged
     * positions, and the second fills slot 1 without double-spawning
     * or replaying the first stage. */
    init(&s);
    s.order.count = 2u;
    s.order.attacker[1] = 0u;
    s.fx.particle_mask = 3u;
    CHECK(stf_collision_combat_apply_model2(&s.model, &s.order, &s.result), 7);
    CHECK(s.result.completed == 2u && s.fx.calls == 2u &&
          s.result.pairs[0].damage_particle.particle.slot_index == 0u &&
          s.result.pairs[1].damage_particle.particle.slot_index == 1u &&
          rd32(s.slots + 0x00u) == UINT32_C(0x11111111) &&
          rd32(s.slots + STF_COLLISION_PARTICLE_SLOT_SIZE) == 0u &&
          rd32(s.slots + STF_COLLISION_PARTICLE_SLOT_SIZE + 0x1Cu) ==
              UINT32_C(0x2000), 8);

    /* Normal modes examine exactly 16 slots: a full pool consumes the
     * staged request even when no particle can be allocated. */
    init(&s);
    for (i = 0u; i < STF_COLLISION_PARTICLE_NORMAL_SLOTS; ++i) {
        wr32(s.slots + i * STF_COLLISION_PARTICLE_SLOT_SIZE + 0x1Cu,
             UINT32_C(0xBEEF));
    }
    CHECK(stf_collision_combat_apply_model2(&s.model, &s.order, &s.result), 9);
    CHECK(s.result.pairs[0].damage_particle_processed &&
          s.result.pairs[0].damage_particle.requested &&
          !s.result.pairs[0].damage_particle.particle.allocated &&
          s.result.pairs[0].damage_particle.particle.slot_limit == 16u &&
          s.stage.position_a[0] == 0u &&
          s.stage.draw_particle == 0u &&
          rd32(s.slots + 16u * STF_COLLISION_PARTICLE_SLOT_SIZE + 0x1Cu) ==
              0u, 10);

    /* Original 0x1A/0x1B special submodes scan the full 32 slots.
     * With the first 16 occupied, allocation starts at slot 16. */
    init(&s);
    s.model.also_sub_mode = UINT8_C(0x1A);
    for (i = 0u; i < STF_COLLISION_PARTICLE_NORMAL_SLOTS; ++i) {
        wr32(s.slots + i * STF_COLLISION_PARTICLE_SLOT_SIZE + 0x1Cu,
             UINT32_C(0xBEEF));
    }
    CHECK(stf_collision_combat_apply_model2(&s.model, &s.order, &s.result), 11);
    CHECK(s.result.pairs[0].damage_particle.particle.allocated &&
          s.result.pairs[0].damage_particle.particle.slot_index == 16u &&
          s.result.pairs[0].damage_particle.particle.slot_limit == 32u &&
          rd32(s.slots + 16u * STF_COLLISION_PARTICLE_SLOT_SIZE + 0x1Cu) ==
              UINT32_C(0x2000), 12);

    /* Fail configuration validation BEFORE the first prefix write. */
    init(&s);
    s.model.damage_particle_slots = NULL;
    CHECK(!stf_collision_combat_apply_model2(&s.model, &s.order, &s.result) &&
          s.fx.calls == 0u &&
          rd32(s.fighters[1] + 0x1234u) == 0u, 13);
    init(&s);
    s.model.damage_particle_descriptors = NULL;
    CHECK(!stf_collision_combat_apply_model2(&s.model, &s.order, &s.result) &&
          s.fx.calls == 0u &&
          rd32(s.fighters[1] + 0x1234u) == 0u, 14);
    init(&s);
    s.model.damage_particle_stage = NULL;
    CHECK(!stf_collision_combat_apply_model2(&s.model, &s.order, &s.result) &&
          s.fx.calls == 0u &&
          rd32(s.fighters[1] + 0x1234u) == 0u, 15);
    init(&s);
    s.model.damage_particle_slots_size =
        STF_COLLISION_PARTICLE_NORMAL_SLOTS *
        STF_COLLISION_PARTICLE_SLOT_SIZE - 1u;
    CHECK(!stf_collision_combat_apply_model2(&s.model, &s.order, &s.result) &&
          rd32(s.fighters[1] + 0x1234u) == 0u, 16);
    init(&s);
    s.model.also_sub_mode = UINT8_C(0x1B);
    s.model.damage_particle_slots_size =
        STF_COLLISION_PARTICLE_EXTENDED_SLOTS *
        STF_COLLISION_PARTICLE_SLOT_SIZE - 1u;
    CHECK(!stf_collision_combat_apply_model2(&s.model, &s.order, &s.result) &&
          rd32(s.fighters[1] + 0x1234u) == 0u, 17);

    /* Legacy opt-out retains damage_unit CPU writes but does not touch
     * any caller-owned particle slots or staged positions. */
    init(&s);
    s.model.recover_damage_particles = false;
    CHECK(stf_collision_combat_apply_model2(&s.model, &s.order, &s.result), 18);
    CHECK(s.result.pairs[0].damage.post.request_particle_setup &&
          !s.result.pairs[0].damage_particle_processed &&
          rd32(s.slots + 0x1Cu) == 0u &&
          s.stage.position_a[0] == UINT32_C(0x11111111), 19);

    /* End-to-end: the real ordered combat adapter creates a particle,
     * then the caller explicitly advances efc_disp/32B10 frame ticks
     * against the SAME live slot pool and ROM descriptor bindings. */
    {
        static const uint16_t animation[] = {
            UINT16_C(0x120), UINT16_C(0x121)
        };
        stf_collision_particle_draw draws[STF_COLLISION_PARTICLE_NORMAL_SLOTS];
        stf_collision_particle_frame_result step_result;

        init(&s);
        s.descriptors[1].duration = UINT16_C(2);
        s.descriptors[1].frame_divisor = UINT16_C(1);
        s.descriptors[1].frames = animation;
        s.descriptors[1].frame_count = 2u;
        CHECK(stf_collision_combat_apply_model2(&s.model, &s.order, &s.result) &&
              s.result.pairs[0].damage_particle.particle.allocated, 20);
        CHECK(stf_collision_particle_frame_step_combat_model2(
                  &s.model, draws, STF_COLLISION_PARTICLE_NORMAL_SLOTS,
                  &step_result), 21);
        CHECK(step_result.updated && step_result.draws_built &&
              step_result.update.advanced == 1u &&
              step_result.render.draw_count == 1u &&
              draws[0].slot_index == 0u &&
              draws[0].frame_id == UINT16_C(0x120) &&
              draws[0].position[0] == UINT32_C(0x11111111) &&
              draws[0].use_saved_graphics_state &&
              draws[0].scale_bits == UINT32_C(0x3F800000) &&
              s.slots[0x18u] == UINT8_C(1), 22);
        CHECK(stf_collision_particle_frame_step_combat_model2(
                  &s.model, draws, STF_COLLISION_PARTICLE_NORMAL_SLOTS,
                  &step_result) &&
              step_result.update.advanced == 1u &&
              step_result.render.draw_count == 1u &&
              draws[0].frame_id == UINT16_C(0x121) &&
              draws[0].scale_bits == UINT32_C(0x3FC00000) &&
              s.slots[0x18u] == UINT8_C(2), 23);
        CHECK(stf_collision_particle_frame_step_combat_model2(
                  &s.model, draws, STF_COLLISION_PARTICLE_NORMAL_SLOTS,
                  &step_result) &&
              step_result.update.released == 1u &&
              step_result.render.draw_count == 0u &&
              rd32(s.slots + 0x1Cu) == 0u, 24);
    }

    /* The frame wrapper also preserves the original 16-slot
     * update/render limit when extended combat allocation used slot 16. */
    {
        stf_collision_particle_draw draws[STF_COLLISION_PARTICLE_NORMAL_SLOTS];
        stf_collision_particle_frame_result step_result;
        uint8_t saved_slot16[STF_COLLISION_PARTICLE_SLOT_SIZE];

        init(&s);
        s.model.also_sub_mode = UINT8_C(0x1B);
        for (i = 0u; i < STF_COLLISION_PARTICLE_NORMAL_SLOTS; ++i) {
            wr32(s.slots + i * STF_COLLISION_PARTICLE_SLOT_SIZE + 0x1Cu,
                 UINT32_C(0xBEEF));
        }
        CHECK(stf_collision_combat_apply_model2(&s.model, &s.order, &s.result) &&
              s.result.pairs[0].damage_particle.particle.slot_index == 16u, 25);
        memcpy(saved_slot16,
               s.slots + 16u * STF_COLLISION_PARTICLE_SLOT_SIZE,
               sizeof(saved_slot16));
        /* Occupied slots 0..15 carry deliberately unrecognized effect
         * addresses, so the frame's fail-closed preflight must not
         * change slot 16 nor any other live slot. */
        CHECK(!stf_collision_particle_frame_step_combat_model2(
                  &s.model, draws, STF_COLLISION_PARTICLE_NORMAL_SLOTS,
                  &step_result) &&
              memcmp(saved_slot16,
                     s.slots + 16u * STF_COLLISION_PARTICLE_SLOT_SIZE,
                     sizeof(saved_slot16)) == 0, 26);
    }

    return 0;
}
