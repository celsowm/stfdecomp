#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "collision_combat_runtime.h"

enum { F_SIZE = 0x2200u, W_SIZE = 0x2B0u,
       ENEMY_SIZE = 0x10Au, PROFILE_SIZE = 5u * 40u };

static void wr32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8u);
    p[2] = (uint8_t)(v >> 16u);
    p[3] = (uint8_t)(v >> 24u);
}
static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) |
           ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}
typedef struct hooks {
    unsigned legacy_calls;
    unsigned normal_calls;
    unsigned stance_requests;
    bool fail_tail;
} hooks;

static bool legacy_body(
    void *userdata, uint8_t attacker_index, uint8_t defender_index,
    uint8_t *attacker, size_t attacker_size,
    uint8_t *defender, size_t defender_size,
    uint8_t *workspace, size_t workspace_size,
    const stf_attack_hit_prefix_result *prefix)
{
    hooks *h = (hooks *)userdata;
    if (attacker_index != 1u || defender_index != 0u ||
        attacker == NULL || defender == NULL || workspace == NULL ||
        prefix == NULL || attacker_size < F_SIZE || defender_size < F_SIZE ||
        workspace_size < W_SIZE) return false;
    ++h->legacy_calls;
    return true;
}

static bool normal_tail(
    void *userdata, uint8_t attacker_index, uint8_t defender_index,
    uint8_t *attacker, size_t attacker_size,
    uint8_t *defender, size_t defender_size,
    uint8_t *workspace, size_t workspace_size,
    const stf_attack_hit_prefix_result *prefix,
    const stf_attack_hit_normal_entry_result *normal,
    const stf_attack_side_exit_result *side)
{
    hooks *h = (hooks *)userdata;
    if (attacker_index != 1u || defender_index != 0u ||
        attacker == NULL || defender == NULL || workspace == NULL ||
        prefix == NULL || normal == NULL || side == NULL ||
        attacker_size < F_SIZE || defender_size < F_SIZE ||
        workspace_size < W_SIZE ||
        normal->guard_branch != STF_ATTACK_GUARD_EXIT ||
        side->path != STF_ATTACK_SIDE_EXIT_CONTINUE ||
        rd32(attacker + 0x1234u) != 1u) return false;
    ++h->normal_calls;
    if (side->request_set_kamae) {
        if (!side->cleared_defender_bit29 ||
            (rd32(defender) & (UINT32_C(1) << 29u)) != 0u)
            return false;
        ++h->stance_requests;
    }
    return !h->fail_tail;
}

typedef struct scene {
    uint8_t fighters[2][F_SIZE];
    uint8_t enemies[2][ENEMY_SIZE];
    uint8_t workspace[W_SIZE];
    uint8_t profiles[PROFILE_SIZE];
    stf_damage_unit_effect_state effect;
    stf_collision_combat_model2 model;
    stf_collision_hit_order order;
    stf_collision_combat_result output;
    hooks callbacks;
} scene;

static void init(scene *s)
{
    memset(s, 0, sizeof(*s));
    s->fighters[1][4u] = 1u;
    s->fighters[1][0x822u] = 8u;
    s->fighters[1][0x843u] = 1u;
    s->fighters[1][0x828u + 1u] = 2u; /* hit flag bit9 */
    wr32(s->profiles + 40u + 8u, UINT32_C(0x3F800000));
    wr32(s->profiles + 4u * 40u + 8u, UINT32_C(0x3F800000));
    s->model.fighters[0] = s->fighters[0];
    s->model.fighters[1] = s->fighters[1];
    s->model.fighter_sizes[0] = F_SIZE;
    s->model.fighter_sizes[1] = F_SIZE;
    s->model.enemy_slots[0] = s->enemies[0];
    s->model.enemy_slots[1] = s->enemies[1];
    s->model.enemy_slot_sizes[0] = ENEMY_SIZE;
    s->model.enemy_slot_sizes[1] = ENEMY_SIZE;
    s->model.workspace = s->workspace;
    s->model.workspace_size = W_SIZE;
    s->model.effect = &s->effect;
    s->model.attack_body = legacy_body;
    s->model.attack_body_user_data = &s->callbacks;
    s->model.normal_profile_table = s->profiles;
    s->model.normal_profile_table_size = PROFILE_SIZE;
    s->model.normal_exit_tail = normal_tail;
    s->model.normal_exit_tail_user_data = &s->callbacks;
    s->order.count = 1u;
    s->order.attacker[0] = 1u;
}
int main(void)
{
    scene s;

    /* Normal 0x2AE40 -> callback, never generic body. */
    init(&s);
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.output.attempted != 1u || s.output.completed != 1u ||
        !s.output.pairs[0].normal_entry_applied ||
        s.output.pairs[0].normal_entry.guard_branch != STF_ATTACK_GUARD_EXIT ||
        !s.output.pairs[0].normal_exit_applied ||
        s.output.pairs[0].normal_exit_aborted ||
        !s.output.pairs[0].normal_exit_tail_completed ||
        !s.output.pairs[0].body_completed ||
        !s.output.pairs[0].damage_completed ||
        s.callbacks.normal_calls != 1u ||
        s.callbacks.legacy_calls != 0u) return 1;

    /* Abort from attacker +0x5B8 bit0 restores lifecycle count and
       clears both +0x108 markers without calling any body. */
    init(&s);
    wr32(s.fighters[1] + 0x5B8u, UINT32_C(1));
    s.enemies[0][0x108u] = 1u;
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_exit_applied ||
        !s.output.pairs[0].normal_exit_aborted ||
        s.output.pairs[0].normal_exit_tail_completed ||
        s.callbacks.normal_calls != 0u ||
        s.callbacks.legacy_calls != 0u ||
        rd32(s.fighters[1] + 0x1234u) != 0u ||
        s.enemies[0][0x108u] != 0u ||
        s.enemies[1][0x108u] != 0u) return 2;

    /* Defender bit29 override is consumed only on continuing path.
       The tail gets a mandatory set_kamae event. */
    init(&s);
    wr32(s.fighters[0], UINT32_C(1) << 29u);
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].prefix.opponent_override ||
        !s.output.pairs[0].normal_exit.cleared_defender_bit29 ||
        !s.output.pairs[0].normal_exit.request_set_kamae ||
        s.callbacks.stance_requests != 1u ||
        (rd32(s.fighters[0]) & (UINT32_C(1) << 29u)) != 0u)
        return 3;

    /* Bit19 + defender bit29 forces abort BEFORE consuming the flag. */
    init(&s);
    wr32(s.fighters[0], UINT32_C(1) << 29u);
    wr32(s.fighters[1] + 0x860u, UINT32_C(1) << 19u);
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_exit_aborted ||
        s.output.pairs[0].normal_exit.cleared_defender_bit29 ||
        s.callbacks.stance_requests != 0u ||
        s.callbacks.normal_calls != 0u ||
        (rd32(s.fighters[0]) & (UINT32_C(1) << 29u)) == 0u)
        return 4;

    /* A failed tail preserves partial ROM-side mutation, no damage. */
    init(&s);
    s.callbacks.fail_tail = true;
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.output.attempted != 1u || s.output.completed != 0u ||
        !s.output.pairs[0].normal_exit_applied ||
        s.output.pairs[0].normal_exit_tail_completed ||
        s.output.pairs[0].damage_completed ||
        s.callbacks.normal_calls != 1u ||
        rd32(s.fighters[1] + 0x1234u) != 1u) return 5;

    /* Opt-in fails before prefix mutation without a ROM profile. */
    init(&s);
    s.model.normal_profile_table = NULL;
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        rd32(s.fighters[1] + 0x1234u) != 0u) return 6;

    /* No new callback -> legacy generic body is preserved. */
    init(&s);
    s.model.normal_exit_tail = NULL;
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.output.pairs[0].normal_exit_applied ||
        s.callbacks.normal_calls != 0u ||
        s.callbacks.legacy_calls != 1u) return 7;

    return 0;
}
