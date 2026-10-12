#include <stdbool.h>
#include <stdio.h>
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
    bool verify_knockback;
    bool verify_tgp_knockback;
    bool verify_pre_damage;
    bool fail_damage_event;
    unsigned damage_event_calls;
    unsigned damage_ring_events;
    uint8_t *damage_defender;
    uint8_t *damage_attacker;
    unsigned motion_queries;
    unsigned tgp_calls;
    unsigned fail_tgp_at;
    unsigned nan_tgp_at;
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

/* Synthetic five-reply TGP transcript. The output words deliberately
 * differ from host sin/cos, so the regression cannot accidentally
 * pass by using portable math. The header/argument checks are strict;
 * real traces can replace this fixture unchanged. */
static bool replay_tgp_motion(
    void *userdata,
    const stf_attack_hit_guard_vector_query *query,
    uint32_t *reply)
{
    hooks *h = (hooks *)userdata;
    static const uint32_t opcodes[5] = {
        UINT32_C(0x0D001A1A), UINT32_C(0x12002424),
        UINT32_C(0x12802525), UINT32_C(0x12002424),
        UINT32_C(0x12802525)
    };
    static const uint32_t replies[5] = {
        UINT32_C(0x40000000), UINT32_C(0x3F800000),
        UINT32_C(0x40000000), UINT32_C(0x3FA00000),
        UINT32_C(0x3FC00000)
    };
    const unsigned index = h->tgp_calls;
    if (query == NULL || reply == NULL || index >= 5u)
        return false;
    ++h->tgp_calls;
    if (query->opcode != opcodes[index] ||
        query->argument_count != (index == 0u ? 1u : 2u) ||
        (index == 0u && query->words[0] == 0u) ||
        (index == 1u && (query->words[0] != 0u ||
                         query->words[1] != replies[0])) ||
        (index == 2u && (query->words[0] != 0u ||
                         query->words[1] != replies[0])) ||
        (index == 3u && (query->words[0] != 0u ||
                         query->words[1] != replies[2])) ||
        (index == 4u && (query->words[0] != 0u ||
                         query->words[1] != replies[2])))
        return false;
    if (h->fail_tgp_at == index + 1u) return false;
    *reply = h->nan_tgp_at == index + 1u
        ? UINT32_C(0x7FC00000) : replies[index];
    return true;
}

static bool consume_damage_event(
    void *userdata, uint8_t attacker_index, uint8_t defender_index,
    const stf_damage_calculation_result *damage,
    bool rings_recovered,
    const stf_ring_damage_flow_result *rings)
{
    hooks *h = (hooks *)userdata;
    if (attacker_index != 1u || defender_index != 0u ||
        damage == NULL || rings == NULL ||
        h->damage_defender == NULL || h->damage_attacker == NULL)
        return false;
    ++h->damage_event_calls;
    if (damage->request_ring_scatter) ++h->damage_ring_events;
    if (rings_recovered && !damage->request_ring_scatter &&
        (rings->requested || rings->spawned_count != 0u))
        return false;
    return !h->fail_damage_event;
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
    if (h->verify_knockback &&
        (rd32(defender + 0x198u) != UINT32_C(0x08000177) ||
         rd32(defender + 0x5E8u) == 0u ||
         rd32(defender + 0x5E0u) != 0u))
        return false;
    if (h->verify_tgp_knockback &&
        (rd32(defender + 0x5E0u) != UINT32_C(0x3FA00000) ||
         rd32(defender + 0x5E4u) != UINT32_C(0x3F800000) ||
         rd32(defender + 0x5E8u) != UINT32_C(0x3FC00000)))
        return false;
    if (h->verify_pre_damage &&
        (rd16(defender + 0x1ACu) != 100u ||
         rd16(attacker + 0x1F70u) != 0u))
        return false;
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
    uint8_t knockback_rom_bytes[4096];
    stf_motion_hit_rom_view knockback_rom;
    uint8_t selector_blocks[2][STF_SET_KAMAE_FULL_SELECTOR_MIN_SIZE];
    uint8_t stances[2][2048];
    stf_ring_pool ring_pool;
    stf_ring_slot ring_slots[STF_RING_POOL_SLOT_COUNT];
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
    wr32(s->profiles + 40u + 0u, UINT32_C(0x3F800000));
    wr32(s->profiles + 40u + 4u, UINT32_C(0x3F800000));
    wr32(s->profiles + 40u + 0x10u, UINT32_C(0x3F800000));
    wr32(s->profiles + 40u + 0x18u, UINT32_C(0x3F800000));
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
    s->callbacks.damage_defender = s->fighters[0];
    s->callbacks.damage_attacker = s->fighters[1];
    s->model.normal_damage_event = consume_damage_event;
    s->model.normal_damage_event_user_data = &s->callbacks;
    s->model.normal_game_timer = INT16_C(99);
    s->model.normal_ring_pool = &s->ring_pool;
    s->model.normal_ring_slots = s->ring_slots;
    stf_ring_pool_init(&s->ring_pool);
    s->callbacks.motion_table[18u] = UINT32_C(0x345);
    s->callbacks.motion_table[26u] = UINT32_C(0x456);
    s->callbacks.motion_table[34u] = UINT32_C(0x177);
    s->callbacks.motion_table[40u] = UINT32_C(0x200);
    s->model.normal_hit_motion_resolver = resolve_hit_motion;
    s->model.normal_hit_motion_resolver_user_data = &s->callbacks;
    s->model.normal_initial_damage[1] = UINT32_C(20);
    s->model.normal_energy_max = INT16_C(200);
    s->model.normal_disable_energy_scaling = true;
    s->knockback_rom.image = s->knockback_rom_bytes;
    s->knockback_rom.image_size = sizeof(s->knockback_rom_bytes);
    s->knockback_rom.base_address = UINT32_C(0x00100000);
    s->knockback_rom.animation_related_address = UINT32_C(0x00100020);
    s->knockback_rom.animation_count = 512u;
    s->model.normal_knockback_rom = &s->knockback_rom;
    s->model.normal_knockback_context[1].initial_r9_bits =
        UINT32_C(0x3F800000); /* caller-supplied 1.0 */
    s->model.normal_knockback_context[1].limit_xang = INT16_C(100);
    wr32(s->knockback_rom_bytes + 0x20u + 0x177u * 4u,
         UINT32_C(0x00100800));
    s->knockback_rom_bytes[0x800u + 0x0Du] = UINT8_C(0x11);
    wr16(s->knockback_rom_bytes + 0x800u + 0x0Eu, UINT16_C(0));
    wr32(s->knockback_rom_bytes + 0x800u + 0x10u,
         UINT32_C(0x3F800000)); /* motion strength scale 1.0 */
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
        s.callbacks.legacy_calls != 0u) { fprintf(stderr, "normal-exit combat scenario 1 failed\n"); return 1; }

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
        s.enemies[1][0x108u] != 0u) { fprintf(stderr, "normal-exit combat scenario 2 failed\n"); return 2; }

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
        { fprintf(stderr, "normal-exit combat scenario 3 failed\n"); return 3; }

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
        { fprintf(stderr, "normal-exit combat scenario 4 failed\n"); return 4; }

    /* A failed tail preserves partial ROM-side mutation, no damage. */
    init(&s);
    s.callbacks.fail_tail = true;
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.output.attempted != 1u || s.output.completed != 0u ||
        !s.output.pairs[0].normal_exit_applied ||
        s.output.pairs[0].normal_exit_tail_completed ||
        s.output.pairs[0].damage_completed ||
        s.callbacks.normal_calls != 1u ||
        rd32(s.fighters[1] + 0x1234u) != 1u) { fprintf(stderr, "normal-exit combat scenario 5 failed\n"); return 5; }

    /* Opt-in fails before prefix mutation without a ROM profile. */
    init(&s);
    s.model.normal_profile_table = NULL;
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        rd32(s.fighters[1] + 0x1234u) != 0u) { fprintf(stderr, "normal-exit combat scenario 6 failed\n"); return 6; }

    /* No new callback -> legacy generic body is preserved. */
    init(&s);
    s.model.normal_exit_tail = NULL;
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.output.pairs[0].normal_exit_applied ||
        s.callbacks.normal_calls != 0u ||
        s.callbacks.legacy_calls != 1u) { fprintf(stderr, "normal-exit combat scenario 7 failed\n"); return 7; }

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
        s.callbacks.stance_requests != 1u) { fprintf(stderr, "normal-exit combat scenario 8 failed\n"); return 8; }

    /* When no bit29 request is emitted, no motion lookup occurs. */
    init(&s);
    s.model.recover_normal_stance = true;
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.output.pairs[0].normal_stance_applied ||
        s.callbacks.resolver_calls != 0u) { fprintf(stderr, "normal-exit combat scenario 9 failed\n"); return 9; }

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
        { fprintf(stderr, "normal-exit combat scenario 10 failed\n"); return 10; }

    /* Preflight rejects missing stance memory before prefix mutations. */
    init(&s);
    s.model.recover_normal_stance = true;
    s.model.normal_stance[0].stance_ram_size = 0x600u;
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        rd32(s.fighters[1] + 0x1234u) != 0u) { fprintf(stderr, "normal-exit combat scenario 11 failed\n"); return 11; }

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
        { fprintf(stderr, "normal-exit combat scenario 12 failed\n"); return 12; }

    /* Stance opt-in cannot run without the mandatory remainder callback. */
    init(&s);
    s.model.recover_normal_stance = true;
    s.model.normal_exit_tail = NULL;
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        rd32(s.fighters[1] + 0x1234u) != 0u) { fprintf(stderr, "normal-exit combat scenario 13 failed\n"); return 13; }

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
        { fprintf(stderr, "normal-exit combat scenario 14 failed\n"); return 14; }

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
        s.callbacks.normal_calls != 1u) { fprintf(stderr, "normal-exit combat scenario 15 failed\n"); return 15; }

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
        { fprintf(stderr, "normal-exit combat scenario 16 failed\n"); return 16; }

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
        { fprintf(stderr, "normal-exit combat scenario 17 failed\n"); return 17; }

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
        { fprintf(stderr, "normal-exit combat scenario 18 failed\n"); return 18; }

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
        { fprintf(stderr, "normal-exit combat scenario 19 failed\n"); return 19; }

    /* Down-combo route selects group-4 motion, records +0x6F5 and
     * leaves calc_mht_adr for the real tail. */
    init(&s);
    s.model.recover_normal_hit = true;
    wr32(s.fighters[0] + 0x1A4u, UINT32_C(1) << 14u);
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.output.pairs[0].normal_hit.reaction_path !=
            STF_ATTACK_REACTION_DOWN_COMBO ||
        !s.output.pairs[0].normal_hit.down_reaction_applied ||
        s.output.pairs[0].normal_hit.down_route !=
            STF_DOWN_REACTION_GENERIC ||
        s.output.pairs[0].normal_hit.down_reaction.motion.table_index != 34u ||
        !s.output.pairs[0].normal_hit.down_reaction.reaction.requires_calc_mht ||
        s.fighters[0][0x6F5u] != 1u ||
        rd32(s.fighters[0] + 0x198u) != UINT32_C(0x08000177) ||
        rd16(s.fighters[0] + 0x5DEu) != 14u ||
        s.callbacks.motion_queries != 1u) { fprintf(stderr, "normal-exit combat scenario 24 failed\n"); return 24; }

    /* Generic down override under defender bit4 has a fixed motion and
     * must not query ROM, even when caller makes the resolver fail. */
    init(&s);
    s.model.recover_normal_hit = true;
    wr32(s.fighters[0] + 0x1A4u, UINT32_C(1) << 4u);
    s.callbacks.fail_motion = true;
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.output.pairs[0].normal_hit.reaction_path !=
            STF_ATTACK_REACTION_GENERIC ||
        !s.output.pairs[0].normal_hit.down_reaction_applied ||
        s.output.pairs[0].normal_hit.down_reaction.reaction.selected_motion !=
            UINT32_C(0x106) ||
        s.output.pairs[0].normal_hit.down_reaction.reaction.requires_sub_2b94c ||
        s.callbacks.motion_queries != 0u ||
        s.fighters[0][0x6F5u] != 0u) { fprintf(stderr, "normal-exit combat scenario 25 failed\n"); return 25; }

    /* Special bit16 selects group 5 and computes the alternate stun. */
    init(&s);
    s.model.recover_normal_hit = true;
    wr32(s.fighters[0] + 0x1A4u, UINT32_C(1) << 16u);
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.output.pairs[0].normal_hit.reaction_path !=
            STF_ATTACK_REACTION_SPECIAL_BIT16 ||
        !s.output.pairs[0].normal_hit.down_reaction_applied ||
        s.output.pairs[0].normal_hit.down_route !=
            STF_DOWN_REACTION_SPECIAL_BIT16 ||
        s.output.pairs[0].normal_hit.down_reaction.motion.table_index != 40u ||
        s.output.pairs[0].normal_hit.damage.damage != 15u ||
        rd32(s.fighters[0] + 0x198u) != UINT32_C(0x08010200) ||
        rd16(s.fighters[0] + 0x5DEu) != 0u ||
        s.callbacks.motion_queries != 1u) { fprintf(stderr, "normal-exit combat scenario 26 failed\n"); return 26; }

    /* Down combo can reroute to special according to attacker flags+22,
     * defender bit11 (without bit4); combo+0x6F5 is not incremented. */
    init(&s);
    s.model.recover_normal_hit = true;
    wr32(s.fighters[1] + 0x1A4u, UINT32_C(1) << 22u);
    wr32(s.fighters[0] + 0x1A4u,
         (UINT32_C(1) << 14u) | (UINT32_C(1) << 11u));
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_hit.down_reaction_applied ||
        s.output.pairs[0].normal_hit.down_route !=
            STF_DOWN_REACTION_SPECIAL_BIT16 ||
        s.output.pairs[0].normal_hit.down_reaction.motion.table_index != 40u ||
        s.fighters[0][0x6F5u] != 0u) { fprintf(stderr, "normal-exit combat scenario 27 failed\n"); return 27; }

    /* Fail to resolve group-4 ROM: report classified reaction and
     * previous damage state, but do not run the external tail. */
    init(&s);
    s.model.recover_normal_hit = true;
    wr32(s.fighters[0] + 0x1A4u, UINT32_C(1) << 14u);
    s.callbacks.fail_motion = true;
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_hit.reaction_classified ||
        s.output.pairs[0].normal_hit.down_reaction_applied ||
        s.output.pairs[0].normal_hit.damage.damage != 15u ||
        s.callbacks.motion_queries != 1u ||
        s.callbacks.normal_calls != 0u ||
        s.fighters[0][0x6F5u] != 0u ||
        s.output.pairs[0].damage_completed) { fprintf(stderr, "normal-exit combat scenario 28 failed\n"); return 28; }

    /* Preflight failure happens before the prefix mutates shared state. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.normal_hit_motion_resolver = NULL;
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        rd32(s.fighters[1] + 0x1234u) != 0u)
        { fprintf(stderr, "normal-exit combat scenario 20 failed\n"); return 20; }

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
        { fprintf(stderr, "normal-exit combat scenario 21 failed\n"); return 21; }

    /* Remaining callback failure does not roll back recovered CPU writes. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.callbacks.fail_tail = true;
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_hit.normal_reaction_applied ||
        s.output.pairs[0].normal_exit_tail_completed ||
        s.output.pairs[0].damage_completed ||
        rd32(s.fighters[0] + 0x198u) != UINT32_C(0x0B000345))
        { fprintf(stderr, "normal-exit combat scenario 22 failed\n"); return 22; }

    /* Legacy mode without opt-in must not perform combo and damage writes. */
    init(&s);
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.output.pairs[0].normal_hit_started ||
        s.fighters[0][0x6F4u] != 0u ||
        s.callbacks.motion_queries != 0u)
        { fprintf(stderr, "normal-exit combat scenario 23 failed\n"); return 23; }

    /* Actual damage runs AFTER the tail's pre-damage effects and BEFORE
     * damage_unit. Recovered ring flow consumes the caller's stage/RNG
     * data and reports spawned ring slots to the post-damage event sink. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.recover_normal_damage = true;
    s.model.recover_normal_ring_scatter = true;
    s.callbacks.verify_pre_damage = true;
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_damage_applied ||
        !s.output.pairs[0].normal_damage_event_completed ||
        !s.output.pairs[0].normal_rings_processed ||
        s.output.pairs[0].normal_damage.scaled_damage != 20u ||
        s.output.pairs[0].normal_damage.receiver_energy_after != 80 ||
        !s.output.pairs[0].normal_damage.request_ring_scatter ||
        !s.output.pairs[0].normal_rings.requested ||
        !s.output.pairs[0].normal_rings.request_ring_sound ||
        s.output.pairs[0].normal_rings.spawned_count != 2u ||
        s.ring_pool.occupied_mask == 0u ||
        rd16(s.fighters[0] + 0x1ACu) != 80u ||
        rd16(s.fighters[0] + 0xC54u) != 20u ||
        rd16(s.fighters[1] + 0x1F70u) != 20u ||
        s.callbacks.damage_event_calls != 1u ||
        s.callbacks.damage_ring_events != 1u ||
        !s.output.pairs[0].damage_completed)
        { fprintf(stderr, "normal damage+ring scenario 29 failed\n"); return 29; }

    /* Timer at zero records dealer damage, but leaves fighter HP unchanged
     * and suppresses the ring event. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.recover_normal_damage = true;
    s.model.recover_normal_ring_scatter = true;
    s.model.normal_game_timer = 0;
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_damage_applied ||
        s.output.pairs[0].normal_damage.energy_applied ||
        s.output.pairs[0].normal_damage.request_ring_scatter ||
        s.output.pairs[0].normal_rings.requested ||
        s.output.pairs[0].normal_rings.spawned_count != 0u ||
        s.ring_pool.occupied_mask != 0u ||
        rd16(s.fighters[0] + 0x1ACu) != 100u ||
        rd16(s.fighters[1] + 0x1F70u) != 20u ||
        s.callbacks.damage_event_calls != 1u ||
        s.callbacks.damage_ring_events != 0u)
        { fprintf(stderr, "normal damage timer scenario 30 failed\n"); return 30; }

    /* Character 16 bypasses energy loss in damage_calculation. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.recover_normal_damage = true;
    s.model.recover_normal_ring_scatter = true;
    s.fighters[0][0x1B1u] = 16u;
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.output.pairs[0].normal_damage.energy_applied ||
        s.output.pairs[0].normal_damage.request_ring_scatter ||
        rd16(s.fighters[0] + 0x1ACu) != 100u ||
        rd16(s.fighters[1] + 0x1F70u) != 20u ||
        s.callbacks.damage_ring_events != 0u)
        { fprintf(stderr, "normal damage character-16 scenario 31 failed\n"); return 31; }

    /* KO clamps remaining energy to zero and sets defender bit5. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.recover_normal_damage = true;
    s.model.recover_normal_ring_scatter = true;
    wr16(s.fighters[0] + 0x1ACu, 10u);
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_damage_applied ||
        !s.output.pairs[0].normal_damage.receiver_knocked_out ||
        rd16(s.fighters[0] + 0x1ACu) != 0u ||
        (rd32(s.fighters[0]) & (UINT32_C(1) << 5u)) == 0u ||
        !s.output.pairs[0].damage_completed)
        { fprintf(stderr, "normal damage KO scenario 32 failed\n"); return 32; }

    /* Event handler failure is not rolled back; no downstream damage_unit
     * is executed after the callback returns false. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.recover_normal_damage = true;
    s.model.recover_normal_ring_scatter = true;
    s.callbacks.fail_damage_event = true;
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_damage_applied ||
        !s.output.pairs[0].normal_rings_processed ||
        s.output.pairs[0].normal_damage_event_completed ||
        s.output.pairs[0].damage_completed ||
        s.callbacks.damage_event_calls != 1u ||
        rd16(s.fighters[0] + 0x1ACu) != 80u ||
        s.ring_pool.occupied_mask == 0u)
        { fprintf(stderr, "normal damage event failure scenario 33 failed\n"); return 33; }

    /* Reject invalid opt-ins before attacker prefix increments counters. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.recover_normal_damage = true;
    s.model.normal_damage_event = NULL;
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        rd32(s.fighters[1] + 0x1234u) != 0u)
        { fprintf(stderr, "normal damage preflight scenario 34 failed\n"); return 34; }

    init(&s);
    s.model.recover_normal_hit = true;
    s.model.recover_normal_damage = true;
    s.model.recover_normal_ring_scatter = true;
    s.model.normal_ring_pool = NULL;
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        rd32(s.fighters[1] + 0x1234u) != 0u)
        { fprintf(stderr, "normal ring preflight scenario 35 failed\n"); return 35; }

    /* The early abort does not run energy, ring or sound events. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.recover_normal_damage = true;
    s.model.recover_normal_ring_scatter = true;
    wr32(s.fighters[1] + 0x5B8u, 1u);
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_exit_aborted ||
        s.output.pairs[0].normal_damage_applied ||
        s.callbacks.damage_event_calls != 0u ||
        rd16(s.fighters[0] + 0x1ACu) != 100u ||
        s.ring_pool.occupied_mask != 0u)
        { fprintf(stderr, "normal damage abort scenario 36 failed\n"); return 36; }

    /* Flag off: no damage writes, even when the host callback is installed. */
    init(&s);
    s.model.recover_normal_hit = true;
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.output.pairs[0].normal_damage_applied ||
        s.callbacks.damage_event_calls != 0u ||
        rd16(s.fighters[0] + 0x1ACu) != 100u)
        { fprintf(stderr, "normal damage opt-out scenario 37 failed\n"); return 37; }

    /* Metal character: damage requests rings but the original ring planner
     * suppresses the spawn and the ring sound event. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.recover_normal_damage = true;
    s.model.recover_normal_ring_scatter = true;
    s.fighters[0][0x1B1u] = 3u;
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_damage.request_ring_scatter ||
        !s.output.pairs[0].normal_rings.requested ||
        !s.output.pairs[0].normal_rings.suppressed ||
        s.output.pairs[0].normal_rings.spawned_count != 0u ||
        s.ring_pool.occupied_mask != 0u)
        { fprintf(stderr, "normal damage metal scenario 38 failed\n"); return 38; }

    /* Opt-in ROM-backed down-hit knockback resolves calc_mht_adr
     * with tag 0x11, uses caller-provided r9 + heading, and commits
     * all three axes before the remaining normal combat tail. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.recover_normal_knockback = true;
    s.callbacks.verify_knockback = true;
    wr32(s.fighters[0] + 0x1A4u, UINT32_C(1) << 14u);
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_knockback_started ||
        !s.output.pairs[0].normal_knockback_motion_resolved ||
        !s.output.pairs[0].normal_knockback_vector_applied ||
        s.output.pairs[0].normal_knockback_motion.lookup_status !=
            STF_MOTION_HIT_FOUND ||
        s.output.pairs[0].normal_knockback_motion.record_offset !=
            UINT32_C(0x80D) ||
        s.output.pairs[0].normal_knockback_motion.prefix.source !=
            STF_MOTION_PREFIX_RECORD ||
        rd32(s.fighters[0] + 0x5E8u) == 0u ||
        s.callbacks.normal_calls != 1u ||
        s.callbacks.motion_queries != 1u)
        { fprintf(stderr, "normal knockback ROM scenario 39 failed\n"); return 39; }

    /* NOT_FOUND tag 8 selects the genuine per-hit-kind profile fallback,
     * and the caller's magnitude still reaches the resulting vector. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.recover_normal_knockback = true;
    wr32(s.fighters[0] + 0x1A4u, UINT32_C(1) << 14u);
    s.knockback_rom_bytes[0x800u + 0x0Du] = UINT8_C(8);
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_knockback_vector_applied ||
        s.output.pairs[0].normal_knockback_motion.lookup_status !=
            STF_MOTION_HIT_NOT_FOUND ||
        s.output.pairs[0].normal_knockback_motion.used_mht_record ||
        s.output.pairs[0].normal_knockback_motion.prefix.source !=
            STF_MOTION_PREFIX_PROFILE ||
        rd32(s.fighters[0] + 0x5E8u) == 0u)
        { fprintf(stderr, "normal knockback fallback scenario 40 failed\n"); return 40; }

    /* Bad motion tag is an INVALID ROM record, not a silent fallback.
     * The already-committed reaction remains, vector is untouched,
     * tail/damage_unit must not execute. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.recover_normal_knockback = true;
    wr32(s.fighters[0] + 0x1A4u, UINT32_C(1) << 14u);
    s.knockback_rom_bytes[0x800u + 0x0Du] = UINT8_C(0xFF);
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_knockback_started ||
        s.output.pairs[0].normal_knockback_motion_resolved ||
        s.output.pairs[0].normal_knockback_vector_applied ||
        s.output.pairs[0].damage_completed ||
        rd32(s.fighters[0] + 0x198u) != UINT32_C(0x08000177) ||
        rd32(s.fighters[0] + 0x5E8u) != 0u ||
        s.callbacks.normal_calls != 0u)
        { fprintf(stderr, "normal knockback invalid ROM scenario 41 failed\n"); return 41; }

    /* A normal reaction does not enter the generic/down calc_mht corridor. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.recover_normal_knockback = true;
    s.knockback_rom_bytes[0x800u + 0x0Du] = UINT8_C(0xFF);
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.output.pairs[0].normal_knockback_started ||
        s.output.pairs[0].normal_knockback_vector_applied ||
        s.callbacks.normal_calls != 1u)
        { fprintf(stderr, "normal knockback skipped-normal scenario 42 failed\n"); return 42; }

    /* Explicit special-bit16 uses a different reaction path; do not
     * duplicate the generic/down shared vector. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.recover_normal_knockback = true;
    wr32(s.fighters[0] + 0x1A4u, UINT32_C(1) << 16u);
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_hit.down_reaction_applied ||
        s.output.pairs[0].normal_knockback_started ||
        s.output.pairs[0].normal_knockback_vector_applied)
        { fprintf(stderr, "normal knockback skipped-special scenario 43 failed\n"); return 43; }

    /* Missing ROM view (or disabled parent mode) must fail preflight
     * without incrementing attacker lifecycle counters. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.recover_normal_knockback = true;
    s.model.normal_knockback_rom = NULL;
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        rd32(s.fighters[1] + 0x1234u) != 0u)
        { fprintf(stderr, "normal knockback preflight scenario 44 failed\n"); return 44; }

    init(&s);
    s.model.recover_normal_knockback = true;
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        rd32(s.fighters[1] + 0x1234u) != 0u)
        { fprintf(stderr, "normal knockback parent-gate scenario 45 failed\n"); return 45; }

    /* Opted-out knockback preserves earlier behavior and leaves axes alone. */
    init(&s);
    s.model.recover_normal_hit = true;
    wr32(s.fighters[0] + 0x1A4u, UINT32_C(1) << 14u);
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.output.pairs[0].normal_knockback_started ||
        rd32(s.fighters[0] + 0x5E8u) != 0u ||
        s.callbacks.normal_calls != 1u)
        { fprintf(stderr, "normal knockback opt-out scenario 46 failed\n"); return 46; }

    /* Normal side-exit abort must not even attempt ROM resolution. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.recover_normal_knockback = true;
    wr32(s.fighters[1] + 0x5B8u, 1u);
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        !s.output.pairs[0].normal_exit_aborted ||
        s.output.pairs[0].normal_knockback_started ||
        rd32(s.fighters[0] + 0x5E8u) != 0u ||
        s.callbacks.normal_calls != 0u)
        { fprintf(stderr, "normal knockback abort scenario 47 failed\n"); return 47; }

    /* Five exact query/reply steps override the host trig values. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.recover_normal_knockback = true;
    s.model.normal_knockback_tgp = replay_tgp_motion;
    s.model.normal_knockback_tgp_user_data = &s.callbacks;
    s.callbacks.verify_tgp_knockback = true;
    wr32(s.fighters[0] + 0x1A4u, UINT32_C(1) << 14u);
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.callbacks.tgp_calls != 5u ||
        s.output.pairs[0].normal_knockback_tgp_completed != 5u ||
        !s.output.pairs[0].normal_knockback_vector_applied ||
        rd32(s.fighters[0] + 0x5E0u) != UINT32_C(0x3FA00000) ||
        rd32(s.fighters[0] + 0x5E4u) != UINT32_C(0x3F800000) ||
        rd32(s.fighters[0] + 0x5E8u) != UINT32_C(0x3FC00000) ||
        s.callbacks.normal_calls != 1u)
        { fprintf(stderr, "TGP exact reply scenario 48 failed\n"); return 48; }

    /* A rejected sqrt command leaves the defender vector untouched. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.recover_normal_knockback = true;
    s.model.normal_knockback_tgp = replay_tgp_motion;
    s.model.normal_knockback_tgp_user_data = &s.callbacks;
    s.callbacks.fail_tgp_at = 1u;
    wr32(s.fighters[0] + 0x1A4u, UINT32_C(1) << 14u);
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.callbacks.tgp_calls != 1u ||
        s.output.pairs[0].normal_knockback_tgp_completed != 0u ||
        s.output.pairs[0].normal_knockback_motion_resolved ||
        rd32(s.fighters[0] + 0x5E8u) != 0u ||
        s.callbacks.normal_calls != 0u)
        { fprintf(stderr, "TGP sqrt failure scenario 49 failed\n"); return 49; }

    /* Failure on the fourth command records the first three replies,
     * but never writes partial X/Y/Z state or calls the remainder tail. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.recover_normal_knockback = true;
    s.model.normal_knockback_tgp = replay_tgp_motion;
    s.model.normal_knockback_tgp_user_data = &s.callbacks;
    s.callbacks.fail_tgp_at = 4u;
    wr32(s.fighters[0] + 0x1A4u, UINT32_C(1) << 14u);
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.callbacks.tgp_calls != 4u ||
        s.output.pairs[0].normal_knockback_tgp_completed != 3u ||
        !s.output.pairs[0].normal_knockback_motion_resolved ||
        s.output.pairs[0].normal_knockback_vector_applied ||
        rd32(s.fighters[0] + 0x5E0u) != 0u ||
        rd32(s.fighters[0] + 0x5E4u) != 0u ||
        rd32(s.fighters[0] + 0x5E8u) != 0u ||
        s.callbacks.normal_calls != 0u ||
        s.output.pairs[0].damage_completed)
        { fprintf(stderr, "TGP partial trace scenario 50 failed\n"); return 50; }

    /* An invalid (NaN) TGP reply is never committed. Earlier successful
     * commands are counted for tracing even when the final reply is bad. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.recover_normal_knockback = true;
    s.model.normal_knockback_tgp = replay_tgp_motion;
    s.model.normal_knockback_tgp_user_data = &s.callbacks;
    s.callbacks.nan_tgp_at = 5u;
    wr32(s.fighters[0] + 0x1A4u, UINT32_C(1) << 14u);
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.callbacks.tgp_calls != 5u ||
        s.output.pairs[0].normal_knockback_tgp_completed != 4u ||
        s.output.pairs[0].normal_knockback_vector_applied ||
        rd32(s.fighters[0] + 0x5E8u) != 0u)
        { fprintf(stderr, "TGP NaN failure scenario 51 failed\n"); return 51; }

    /* The sqrt reply itself can be invalid even when the device call
     * succeeded. It is not a completed command and never reaches trig. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.recover_normal_knockback = true;
    s.model.normal_knockback_tgp = replay_tgp_motion;
    s.model.normal_knockback_tgp_user_data = &s.callbacks;
    s.callbacks.nan_tgp_at = 1u;
    wr32(s.fighters[0] + 0x1A4u, UINT32_C(1) << 14u);
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.callbacks.tgp_calls != 1u ||
        s.output.pairs[0].normal_knockback_tgp_completed != 0u ||
        s.output.pairs[0].normal_knockback_motion_resolved ||
        rd32(s.fighters[0] + 0x5E8u) != 0u ||
        s.callbacks.normal_calls != 0u)
        { fprintf(stderr, "TGP invalid sqrt scenario 54 failed\n"); return 54; }

    /* A configured TGP provider without the parent knockback opt-in is a
     * preflight error, before attacker lifecycle state has changed. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.normal_knockback_tgp = replay_tgp_motion;
    s.model.normal_knockback_tgp_user_data = &s.callbacks;
    if (stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        rd32(s.fighters[1] + 0x1234u) != 0u ||
        s.callbacks.tgp_calls != 0u)
        { fprintf(stderr, "TGP invalid opt-in scenario 52 failed\n"); return 52; }

    /* Even with a TGP provider, normal hits never require the generic
     * down-hit vector commands. */
    init(&s);
    s.model.recover_normal_hit = true;
    s.model.recover_normal_knockback = true;
    s.model.normal_knockback_tgp = replay_tgp_motion;
    s.model.normal_knockback_tgp_user_data = &s.callbacks;
    if (!stf_collision_combat_apply_model2(&s.model, &s.order, &s.output) ||
        s.callbacks.tgp_calls != 0u ||
        s.output.pairs[0].normal_knockback_tgp_completed != 0u)
        { fprintf(stderr, "TGP unused branch scenario 53 failed\n"); return 53; }

    return 0;
}
