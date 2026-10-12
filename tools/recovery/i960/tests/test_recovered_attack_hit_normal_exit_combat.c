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
    unsigned resolver_calls;
    bool fail_stance;
    bool verify_recovered_stance;
    uint8_t *defender_stance;
    uint16_t selectors[4];
    uint8_t records[4][512];
    bool fail_tail;
    bool fail_motion;
    bool verify_normal_hit;
    unsigned motion_queries;
    uint32_t motion_table[64];
} hooks;

static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8u));
}
static void wr16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8u);
}
static bool resolve_stance(
    uint16_t selector, const uint8_t **record,
    size_t *record_size, void *userdata)
{
    hooks *h = (hooks *)userdata;
    size_t i;
    ++h->resolver_calls;
    if (h->fail_stance || record == NULL || record_size == NULL)
        return false;
    for (i = 0u; i < 4u; ++i) {
        if (selector == h->selectors[i]) {
            *record = h->records[i];
            *record_size = sizeof(h->records[i]);
            return true;
        }
    }
    return false;
}
static void build_record(uint8_t *record, uint32_t float_bits)
{
    size_t i;
    memset(record, 0, 512u);
    for (i = 0u; i < STF_GET_KAMAE_ROW_COUNT; ++i)
        record[STF_GET_KAMAE_DESCRIPTOR_OFFSET + i] = UINT8_C(0x15);
    for (i = 0u; i < 60u; ++i)
        wr32(record + 24u + i * 4u, float_bits);
}

static bool resolve_hit_motion(
    uint8_t selector, const uint32_t **words,
    size_t *word_count, void *userdata)
{
    hooks *h = (hooks *)userdata;
    ++h->motion_queries;
    if (selector != 0u || h->fail_motion ||
        words == NULL || word_count == NULL) return false;
    *words = h->motion_table;
    *word_count = 64u;
    return true;
}

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
    if (h->verify_normal_hit) {
        if (rd32(attacker + 0x194u) != UINT32_C(0x10000001) ||
            (rd32(attacker) & (UINT32_C(1) << 16u)) == 0u ||
            defender[0x6F4u] != 1u ||
            (rd32(workspace + 0x26Cu) & 3u) != 3u ||
            rd32(defender + 0x198u) != UINT32_C(0x0B000345) ||
            rd16(defender + 0x5DEu) != 6u)
            return false;
    }
    if (side->request_set_kamae) {
        if (!side->cleared_defender_bit29 ||
            (rd32(defender) & (UINT32_C(1) << 29u)) != 0u)
            return false;
        ++h->stance_requests;
        if (h->verify_recovered_stance &&
            (h->defender_stance == NULL ||
             rd16(h->defender_stance + 0x1E0u) != 10u ||
             rd16(h->defender_stance + 0x2D0u) != 20u ||
             rd16(h->defender_stance + 0x3C0u) != 30u ||
             rd16(h->defender_stance + 0x5A0u) != 40u))
            return false;
    }
    return !h->fail_tail;
}

typedef struct scene {
    uint8_t fighters[2][F_SIZE];
    uint8_t enemies[2][ENEMY_SIZE];
    uint8_t workspace[W_SIZE];
    uint8_t profiles[PROFILE_SIZE];
    uint8_t selector_blocks[2][STF_SET_KAMAE_FULL_SELECTOR_MIN_SIZE];
    uint8_t stances[2][2048];
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
    s->callbacks.defender_stance = s->stances[0];
    s->callbacks.motion_table[18u] = UINT32_C(0x345);
    s->callbacks.motion_table[26u] = UINT32_C(0x456);
    s->model.normal_hit_motion_resolver = resolve_hit_motion;
    s->model.normal_hit_motion_resolver_user_data = &s->callbacks;
    s->model.normal_initial_damage[1] = UINT32_C(20);
    s->model.normal_energy_max = INT16_C(200);
    s->model.normal_disable_energy_scaling = true;
    wr16(s->fighters[0] + 0x1ACu, UINT16_C(100));
    wr16(s->fighters[1] + 0x1ACu, UINT16_C(100));
    for (size_t i = 0u; i < 4u; ++i) {
        static const uint32_t values[] = {
            UINT32_C(0x41200000), UINT32_C(0x41A00000),
            UINT32_C(0x41F00000), UINT32_C(0x42200000)
        };
        s->callbacks.selectors[i] = (uint16_t)(i + 1u);
        build_record(s->callbacks.records[i], values[i]);
    }
    for (size_t i = 0u; i < 2u; ++i) {
        stf_collision_stance_binding *b = &s->model.normal_stance[i];
        wr16(s->selector_blocks[i] + 0x00u, 1u);
        wr16(s->selector_blocks[i] + 0x08u, 2u);
        wr16(s->selector_blocks[i] + 0x0Au, 3u);
        wr16(s->selector_blocks[i] + 0x50u, 4u);
        b->selector_block = s->selector_blocks[i];
        b->selector_block_size = sizeof(s->selector_blocks[i]);
        b->stance_ram = s->stances[i];
        b->stance_ram_size = sizeof(s->stances[i]);
        b->resolver = resolve_stance;
        b->resolver_user_data = &s->callbacks;
    }
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

    /* Opt in to recovered set_kamae before the remaining normal tail.
     * All four motion selectors execute against supplied records. */
    init(&s);
    s.model.recover_normal_stance = true;
    s.callbacks.verify_recovered_stance = true;
    wr32(s.fighters[0], UINT32_C(1) << 29u);
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_stance_applied ||
        !s.output.pairs[0].normal_stance.executed ||
        s.output.pairs[0].normal_stance.kamae.request_count != 4u ||
        s.callbacks.resolver_calls != 4u ||
        s.callbacks.normal_calls != 1u ||
        s.callbacks.stance_requests != 1u) return 8;

    /* When no bit29 request is emitted, no motion lookup occurs. */
    init(&s);
    s.model.recover_normal_stance = true;
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.output.pairs[0].normal_stance_applied ||
        s.callbacks.resolver_calls != 0u) return 9;

    /* Early abort must NOT consume the defender flag or stance records. */
    init(&s);
    s.model.recover_normal_stance = true;
    s.callbacks.fail_stance = true;
    wr32(s.fighters[0], UINT32_C(1) << 29u);
    wr32(s.fighters[1] + 0x860u, UINT32_C(1) << 19u);
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_exit_aborted ||
        s.output.pairs[0].normal_stance_applied ||
        s.callbacks.resolver_calls != 0u ||
        s.callbacks.normal_calls != 0u ||
        (rd32(s.fighters[0]) & (UINT32_C(1) << 29u)) == 0u)
        return 10;

    /* Preflight rejects missing stance memory before prefix mutations. */
    init(&s);
    s.model.recover_normal_stance = true;
    s.model.normal_stance[0].stance_ram_size = 0x600u;
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        rd32(s.fighters[1] + 0x1234u) != 0u) return 11;

    /* A failed ROM lookup leaves an explicit partial state: bit29 cleared,
     * prefix counter incremented, no tail and no damage execution. */
    init(&s);
    s.model.recover_normal_stance = true;
    s.callbacks.fail_stance = true;
    wr32(s.fighters[0], UINT32_C(1) << 29u);
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_exit_applied ||
        s.output.pairs[0].normal_stance_applied ||
        s.output.pairs[0].damage_completed ||
        s.callbacks.resolver_calls != 1u ||
        s.callbacks.normal_calls != 0u ||
        rd32(s.fighters[1] + 0x1234u) != 1u ||
        (rd32(s.fighters[0]) & (UINT32_C(1) << 29u)) != 0u)
        return 12;

    /* Stance opt-in cannot run without the mandatory remainder callback. */
    init(&s);
    s.model.recover_normal_stance = true;
    s.model.normal_exit_tail = NULL;
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        rd32(s.fighters[1] + 0x1234u) != 0u) return 13;

    /* Subsequent callback failure leaves already applied stance intact. */
    init(&s);
    s.model.recover_normal_stance = true;
    s.callbacks.fail_tail = true;
    wr32(s.fighters[0], UINT32_C(1) << 29u);
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_stance_applied ||
        s.output.pairs[0].normal_exit_tail_completed ||
        s.output.pairs[0].damage_completed ||
        rd16(s.stances[0] + 0x1E0u) != 10u)
        return 14;

    /* CPU followthrough: combo -> state -> damage -> finish -> normal motion.
     * Tail observes all recovered writes before the damage_unit stage. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.callbacks.verify_normal_hit = true;
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_hit_started ||
        !s.output.pairs[0].normal_hit.combo_applied ||
        !s.output.pairs[0].normal_hit.state_prelude_applied ||
        !s.output.pairs[0].normal_hit.damage_transform_applied ||
        !s.output.pairs[0].normal_hit.finish_applied ||
        !s.output.pairs[0].normal_hit.reaction_classified ||
        !s.output.pairs[0].normal_hit.normal_reaction_applied ||
        s.output.pairs[0].normal_hit.damage.damage != UINT32_C(20) ||
        s.output.pairs[0].normal_hit.reaction_path !=
            STF_ATTACK_REACTION_NORMAL ||
        s.output.pairs[0].normal_hit.normal_reaction.motion.table_index != 18u ||
        s.callbacks.motion_queries != 1u ||
        s.callbacks.normal_calls != 1u) return 15;

    /* Counter transforms damage, switches hit mode to 3, and selects the
     * motion row of that mode (not the default mode-2 table row). */
    init(&s);
    s.model.recover_normal_hit = true;
    wr32(s.fighters[0] + 0x1A4u, UINT32_C(1) << 8u);
    s.fighters[0][0x822u] = 20u;
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.output.pairs[0].normal_hit.damage.reason !=
            STF_ATTACK_DAMAGE_COUNTER ||
        s.output.pairs[0].normal_hit.damage.hit_mode != 3u ||
        s.output.pairs[0].normal_hit.damage.damage != 37u ||
        s.output.pairs[0].normal_hit.normal_reaction.motion.table_index != 26u ||
        rd32(s.fighters[0] + 0x198u) != UINT32_C(0x0B000456))
        return 16;

    /* Finish blow sets attacker +0x7D2 and skips not-finish normal motion. */
    init(&s);
    s.model.recover_normal_hit = true;
    wr16(s.fighters[0] + 0x1ACu, UINT16_C(10));
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_hit.finish.finish_blow ||
        s.output.pairs[0].normal_hit.reaction_classified ||
        s.output.pairs[0].normal_hit.normal_reaction_applied ||
        s.callbacks.motion_queries != 0u ||
        (s.fighters[1][0x7D2u] & 1u) == 0u)
        return 17;

    /* Generic reaction route is classified but not spuriously resolved
     * through the normal-hit motion table. Tail owns this other route. */
    init(&s);
    s.model.recover_normal_hit = true;
    wr32(s.fighters[0] + 0x1A4u, UINT32_C(1) << 4u);
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.output.pairs[0].normal_hit.reaction_path !=
            STF_ATTACK_REACTION_GENERIC ||
        s.output.pairs[0].normal_hit.normal_reaction_applied ||
        s.callbacks.motion_queries != 0u)
        return 18;

    /* Resolver failure retains earlier combo/damage CPU writes while
     * reporting incomplete hit motion and not executing the remaining tail. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.callbacks.fail_motion = true;
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_hit_started ||
        !s.output.pairs[0].normal_hit.damage_transform_applied ||
        !s.output.pairs[0].normal_hit.reaction_classified ||
        s.output.pairs[0].normal_hit.normal_reaction_applied ||
        s.output.pairs[0].damage_completed ||
        s.callbacks.normal_calls != 0u ||
        s.callbacks.motion_queries != 1u ||
        s.fighters[0][0x6F4u] != 1u)
        return 19;

    /* Preflight failure happens before the prefix mutates shared state. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.normal_hit_motion_resolver = NULL;
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        rd32(s.fighters[1] + 0x1234u) != 0u)
        return 20;

    /* The original abort must never enter CPU followthrough or its ROM
     * resolver, even when the optional mode is enabled. */
    init(&s);
    s.model.recover_normal_hit = true;
    wr32(s.fighters[1] + 0x5B8u, 1u);
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_exit_aborted ||
        s.output.pairs[0].normal_hit_started ||
        s.callbacks.motion_queries != 0u ||
        s.callbacks.normal_calls != 0u)
        return 21;

    /* Remaining callback failure does not roll back recovered CPU writes. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.callbacks.fail_tail = true;
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_hit.normal_reaction_applied ||
        s.output.pairs[0].normal_exit_tail_completed ||
        s.output.pairs[0].damage_completed ||
        rd32(s.fighters[0] + 0x198u) != UINT32_C(0x0B000345))
        return 22;

    /* Legacy mode without opt-in must not perform combo and damage writes. */
    init(&s);
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.output.pairs[0].normal_hit_started ||
        s.fighters[0][0x6F4u] != 0u ||
        s.callbacks.motion_queries != 0u)
        return 23;

    return 0;
}
