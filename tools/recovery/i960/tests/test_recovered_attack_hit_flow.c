#include <stdint.h>
#include <string.h>

#include "attack_hit.h"
#include "attack_hit_combo.h"
#include "attack_hit_damage.h"
#include "attack_hit_finish.h"
#include "attack_hit_motion_prefix.h"
#include "attack_hit_motion_vector.h"
#include "attack_hit_normal_reaction.h"
#include "attack_hit_reaction.h"
#include "attack_hit_side_exit.h"
#include "attack_hit_stance.h"
#include "attack_hit_sound.h"
#include "attack_hit_strength.h"
#include "damage_calculation.h"
#include "skill_accounting.h"
#include "ring_scatter_damage_flow.h"

enum {
    FLOW_ATTACKER_SIZE = STF_DAMAGE_DEALER_MIN_SIZE,
    FLOW_DEFENDER_SIZE = STF_DAMAGE_RECEIVER_MIN_SIZE
};

static uint16_t read_le16(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8u));
}

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8u) |
           ((uint32_t)data[2] << 16u) |
           ((uint32_t)data[3] << 24u);
}

static void write_le16(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
}

static void write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}


typedef struct flow_kamae_fixture {
    uint16_t selectors[4];
    uint8_t records[4][512];
} flow_kamae_fixture;

static uint32_t float_bits(float value)
{
    uint32_t bits = 0u;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static bool resolve_flow_motion(
    uint16_t selector,
    const uint8_t **motion_record,
    size_t *motion_record_size,
    void *user_data
)
{
    flow_kamae_fixture *fixture = (flow_kamae_fixture *)user_data;
    size_t index = 0u;

    if (fixture == NULL || motion_record == NULL || motion_record_size == NULL) {
        return false;
    }

    for (index = 0u; index < 4u; ++index) {
        if (fixture->selectors[index] == selector) {
            *motion_record = fixture->records[index];
            *motion_record_size = sizeof(fixture->records[index]);
            return true;
        }
    }
    return false;
}

static void build_flow_motion_record(uint8_t *record, float base)
{
    size_t index = 0u;

    memset(record, 0, 512u);
    for (index = 0u; index < STF_GET_KAMAE_ROW_COUNT; ++index) {
        record[STF_GET_KAMAE_DESCRIPTOR_OFFSET + index] = UINT8_C(0x15);
    }
    for (index = 0u; index < 60u; ++index) {
        write_le32(
            record + 24u + index * 4u,
            float_bits(base + (float)index)
        );
    }
}

static int run_accepted_hit(void)
{
    uint8_t attacker[FLOW_ATTACKER_SIZE];
    uint8_t defender[FLOW_DEFENDER_SIZE];
    uint8_t workspace[STF_ATTACK_HIT_WORKSPACE_MODEL2_MIN_SIZE];
    uint8_t enemy0[STF_ATTACK_SIDE_EXIT_ENEMY_MIN_SIZE];
    stf_attack_hit_prefix_result prefix;
    stf_attack_hit_strength_input strength_input;
    uint32_t strength_bits = 0u;
    stf_attack_side_exit_result side;
    stf_attack_hit_stance_result stance_result;
    uint8_t selector_block[STF_SET_KAMAE_FULL_SELECTOR_MIN_SIZE];
    uint8_t stance_ram[2048];
    flow_kamae_fixture kamae_fixture;
    stf_attack_hit_combo_result combo;
    stf_attack_damage_result damage_transform;
    stf_attack_hit_sound_plan sound;
    stf_attack_finish_result finish;
    stf_attack_reaction_inputs reaction_in;
    stf_attack_reaction_path reaction_path;
    stf_normal_reaction_result normal;
    stf_motion_prefix_inputs motion_in;
    stf_motion_fallback_profile profile;
    stf_motion_prefix_result motion_prefix;
    stf_motion_vector_inputs vector_in;
    stf_motion_vector_result vector_out;
    stf_damage_calculation_result damage;
    stf_skill_accounting_result skill;
    stf_ring_damage_flow_inputs ring_inputs;
    stf_ring_damage_flow_result ring_flow;
    stf_ring_pool ring_pool;
    stf_ring_slot ring_slots[STF_RING_POOL_SLOT_COUNT];
    uint32_t total_skill = UINT32_C(100);

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    memset(enemy0, 0, sizeof(enemy0));
    memset(&reaction_in, 0, sizeof(reaction_in));
    memset(&motion_in, 0, sizeof(motion_in));
    memset(&profile, 0, sizeof(profile));
    memset(&vector_in, 0, sizeof(vector_in));
    memset(&ring_inputs, 0, sizeof(ring_inputs));
    memset(selector_block, 0, sizeof(selector_block));
    memset(stance_ram, 0, sizeof(stance_ram));
    memset(&kamae_fixture, 0, sizeof(kamae_fixture));
    memset(ring_slots, 0, sizeof(ring_slots));
    stf_ring_pool_init(&ring_pool);

    attacker[0x822u] = UINT8_C(20);
    write_le16(attacker + 0x1ACu, UINT16_C(100));
    write_le16(defender + 0x1ACu, UINT16_C(100));
    write_le32(defender + 0x1F4u, UINT32_C(0x3F800000));
    write_le32(defender + 0x1FCu, UINT32_C(0x00000000));
    write_le32(attacker + 0x1F4u, UINT32_C(0x00000000));
    write_le32(attacker + 0x1FCu, UINT32_C(0x00000000));
    write_le32(defender + 0x20Cu, UINT32_C(0x41200000));
    write_le32(defender + 0x210u, UINT32_C(0x40000000));
    write_le32(defender + 0x214u, UINT32_C(0x41A00000));
    write_le32(defender, UINT32_C(1) << 29u);

    kamae_fixture.selectors[0] = UINT16_C(0x1010);
    kamae_fixture.selectors[1] = UINT16_C(0x2020);
    kamae_fixture.selectors[2] = UINT16_C(0x3030);
    kamae_fixture.selectors[3] = UINT16_C(0x4040);
    write_le16(selector_block + 0x00u, kamae_fixture.selectors[0]);
    write_le16(selector_block + 0x08u, kamae_fixture.selectors[1]);
    write_le16(selector_block + 0x0Au, kamae_fixture.selectors[2]);
    write_le16(selector_block + 0x50u, kamae_fixture.selectors[3]);
    build_flow_motion_record(kamae_fixture.records[0], 10.0f);
    build_flow_motion_record(kamae_fixture.records[1], 20.0f);
    build_flow_motion_record(kamae_fixture.records[2], 30.0f);
    build_flow_motion_record(kamae_fixture.records[3], 40.0f);

    if (!stf_attack_hit_prefix_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            enemy0, sizeof(enemy0),
            &prefix
        )) {
        return 1;
    }

    if (read_le32(attacker + 0x1234u) != 1u ||
        enemy0[0x108u] != 1u ||
        read_le32(workspace + 0x26Cu) != 1u) {
        return 2;
    }

    if (!stf_attack_hit_strength_prepare_model2(
            attacker, sizeof(attacker),
            workspace, sizeof(workspace),
            &strength_input
        ) ||
        strength_input.raw_822 != UINT8_C(20) ||
        strength_input.workspace_26c != UINT32_C(1)) {
        return 3;
    }

    if (!stf_attack_hit_strength_scale_bits(
            strength_input.raw_822_float_bits,
            UINT32_C(0x3F800000),
            &strength_bits
        ) ||
        strength_bits == 0u) {
        return 4;
    }

    if (!stf_attack_hit_side_exit_apply_model2(
            STF_ATTACK_SIDE_EXIT_NORMAL_2AE40,
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            &side
        ) || side.path != STF_ATTACK_SIDE_EXIT_CONTINUE ||
        !side.cleared_defender_bit29 ||
        !side.request_set_kamae ||
        (read_le32(defender) & (UINT32_C(1) << 29u)) != 0u) {
        return 5;
    }

    if (!stf_attack_hit_apply_stance_event(
            &side,
            read_le32(defender),
            selector_block,
            sizeof(selector_block),
            0u,
            stance_ram,
            sizeof(stance_ram),
            resolve_flow_motion,
            &kamae_fixture,
            &stance_result
        ) ||
        !stance_result.executed ||
        stance_result.kamae.request_count != 4u ||
        stance_result.kamae.selectors[0] != kamae_fixture.selectors[0] ||
        stance_result.kamae.selectors[3] != kamae_fixture.selectors[3] ||
        read_le16(stance_ram + 0x1E0u) != UINT16_C(10) ||
        read_le16(stance_ram + 0x5A0u) != UINT16_C(40)) {
        return 19;
    }

    if (!stf_attack_hit_combo_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            UINT8_C(0),
            &combo
        ) ||
        combo.skipped ||
        combo.hit_skill != UINT32_C(20) ||
        combo.bonus_skill != 0u ||
        combo.defender_combo_6f4 != UINT8_C(1) ||
        combo.workspace_26c != UINT32_C(3)) {
        return 6;
    }

    if (!stf_total_skill_add(
            UINT16_C(1),
            UINT32_C(1) << 2u,
            UINT32_C(0),
            attacker[4u],
            combo.hit_skill,
            total_skill,
            &skill
        ) ||
        !skill.applied ||
        skill.selected_flag != (UINT32_C(1) << 2u) ||
        skill.total_skill_after != UINT32_C(120)) {
        return 16;
    }
    total_skill = skill.total_skill_after;

    if (!stf_attack_hit_damage_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            UINT32_C(20),
            &damage_transform
        ) ||
        damage_transform.damage != UINT32_C(20) ||
        damage_transform.reason != STF_ATTACK_DAMAGE_HIT ||
        damage_transform.hit_mode != UINT32_C(2)) {
        return 7;
    }

    if (!stf_attack_hit_sound_plan_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            damage_transform.damage,
            &sound
        ) ||
        sound.kind != STF_ATTACK_HIT_SOUND_SINGLE ||
        sound.source != STF_ATTACK_HIT_SOUND_SOURCE_DWORD_DB6F4 ||
        sound.tier != STF_ATTACK_HIT_SOUND_TIER_MEDIUM ||
        sound.source_index != UINT32_C(1)) {
        return 8;
    }

    if (!stf_attack_hit_finish_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            INT16_C(200),
            false,
            damage_transform.damage,
            &finish
        ) ||
        finish.scaled_damage != UINT32_C(20) ||
        finish.finish_blow ||
        finish.bonus_skill != 0u) {
        return 9;
    }

    reaction_in.defender_flags_1a4 = read_le32(defender + 0x1A4u);
    reaction_in.defender_flags_70c = read_le32(defender + 0x70Cu);
    reaction_in.hit_flags_50fe00 = 0u;
    reaction_in.damage = damage_transform.damage;
    reaction_in.attacker_kind_821 = attacker[0x821u];
    reaction_in.scaled_damage = finish.scaled_damage;
    reaction_in.defender_energy_1ac = (int16_t)read_le16(defender + 0x1ACu);

    if (!stf_attack_hit_reaction_classify(&reaction_in, &reaction_path) ||
        reaction_path != STF_ATTACK_REACTION_NORMAL) {
        return 10;
    }

    if (!stf_attack_hit_normal_reaction_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            damage_transform.damage,
            damage_transform.hit_mode,
            UINT8_C(0),
            UINT32_C(0x10),
            &normal
        ) ||
        normal.defender_198 != UINT32_C(0x0B000010) ||
        normal.defender_5de != INT16_C(6) ||
        normal.reaction_argument != 1 ||
        !normal.requires_sub_2b94c) {
        return 11;
    }

    motion_in.initial_r9_bits = strength_bits;
    motion_in.limit_xang = INT16_C(100);
    motion_in.defender_5d8_bits = UINT32_C(0x42700000);
    profile.scale_normal_bits = UINT32_C(0x3F800000);
    profile.angle_normal = 0;

    if (!stf_attack_hit_motion_prefix_compute(
            &motion_in, NULL, 0u, &profile, &motion_prefix
        ) ||
        motion_prefix.angle_r6 != 0 ||
        motion_prefix.sqrt_r4_bits == 0u) {
        return 12;
    }

    if (!stf_attack_hit_motion_vector_compute(
            &motion_prefix, &vector_in, &vector_out
        ) ||
        vector_out.x_5e0_bits != UINT32_C(0) ||
        vector_out.y_5e4_bits != UINT32_C(0) ||
        vector_out.z_5e8_bits != motion_prefix.sqrt_r4_bits) {
        return 13;
    }

    write_le32(defender + 0x5E0u, vector_out.x_5e0_bits);
    write_le32(defender + 0x5E4u, vector_out.y_5e4_bits);
    write_le32(defender + 0x5E8u, vector_out.z_5e8_bits);

    if (!stf_damage_calculation_apply_model2(
            defender, sizeof(defender),
            attacker, sizeof(attacker),
            INT16_C(200),
            false,
            INT16_C(1),
            damage_transform.damage,
            &damage
        )) {
        return 14;
    }

    if (!damage.energy_applied ||
        damage.scaled_damage != UINT32_C(20) ||
        damage.receiver_energy_after != INT16_C(80) ||
        !damage.request_ring_scatter ||
        read_le16(defender + 0x1ACu) != UINT16_C(80) ||
        read_le16(defender + 0x0C54u) != UINT16_C(20) ||
        read_le16(attacker + 0x1F70u) != UINT16_C(20) ||
        read_le32(defender + 0x5E8u) != vector_out.z_5e8_bits ||
        read_le32(attacker + 0x1234u) != UINT32_C(1) ||
        enemy0[0x108u] != UINT8_C(1) ||
        total_skill != UINT32_C(120)) {
        return 15;
    }


    if (!stf_ring_scatter_apply_damage_event_model2(
            &damage,
            defender, sizeof(defender),
            attacker, sizeof(attacker),
            &ring_inputs,
            &ring_pool,
            ring_slots,
            &ring_flow
        )) {
        return 17;
    }

    if (!ring_flow.requested ||
        ring_flow.suppressed ||
        !ring_flow.request_ring_sound ||
        ring_flow.spawned_count != UINT8_C(2) ||
        ring_flow.recycled_count != UINT8_C(0) ||
        ring_flow.slot_indices[0] != UINT8_C(23) ||
        ring_flow.slot_indices[1] != UINT8_C(22) ||
        ring_pool.head != UINT8_C(23) ||
        ring_pool.tail != UINT8_C(22) ||
        !ring_slots[23].active ||
        !ring_slots[22].active ||
        ring_slots[23].source_ring_index != UINT8_C(0) ||
        ring_slots[22].source_ring_index != UINT8_C(1) ||
        ring_slots[23].blink_from_frame != UINT16_C(60) ||
        ring_slots[23].expire_at_frame != UINT16_C(90) ||
        ring_slots[23].drop_variant != UINT8_C(0)) {
        return 18;
    }

    return 0;
}

static int run_rejected_hit(void)
{
    uint8_t attacker[FLOW_ATTACKER_SIZE];
    uint8_t defender[FLOW_DEFENDER_SIZE];
    uint8_t workspace[STF_ATTACK_HIT_WORKSPACE_MODEL2_MIN_SIZE];
    uint8_t enemy0[STF_ATTACK_SIDE_EXIT_ENEMY_MIN_SIZE];
    uint8_t enemy1[STF_ATTACK_SIDE_EXIT_ENEMY_MIN_SIZE];
    stf_attack_hit_prefix_result prefix;
    stf_attack_side_exit_result side;
    stf_attack_abort_cleanup_result cleanup;

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    memset(enemy0, 0, sizeof(enemy0));
    memset(enemy1, 0, sizeof(enemy1));

    if (!stf_attack_hit_prefix_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            enemy0, sizeof(enemy0),
            &prefix
        )) {
        return 1;
    }

    write_le32(attacker + 0x860u, UINT32_C(1) << 19u);

    if (!stf_attack_hit_side_exit_apply_model2(
            STF_ATTACK_SIDE_EXIT_GUARD_2AC74,
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            &side
        ) || side.path != STF_ATTACK_SIDE_EXIT_ABORT_2B8C8) {
        return 2;
    }

    enemy1[0x108u] = UINT8_C(1);
    if (!stf_attack_hit_abort_cleanup_apply_model2(
            attacker, sizeof(attacker),
            enemy0, sizeof(enemy0),
            enemy1, sizeof(enemy1),
            &cleanup
        )) {
        return 3;
    }

    if (cleanup.next_attacker_counter_1234 != 0u ||
        read_le32(attacker + 0x1234u) != 0u ||
        enemy0[0x108u] != 0u ||
        enemy1[0x108u] != 0u ||
        read_le16(defender + 0x1ACu) != 0u) {
        return 4;
    }

    return 0;
}

int main(void)
{
    const int accepted = run_accepted_hit();
    const int rejected = run_rejected_hit();

    if (accepted != 0) {
        return accepted;
    }
    if (rejected != 0) {
        return 100 + rejected;
    }
    return 0;
}
