#include <stdint.h>
#include <string.h>

#include "attack_hit.h"
#include "attack_hit_combo.h"
#include "attack_hit_damage.h"
#include "attack_hit_finish.h"
#include "attack_hit_motion_prefix.h"
#include "attack_hit_motion_runtime.h"
#include "attack_hit_motion_vector.h"
#include "attack_hit_normal_reaction.h"
#include "attack_hit_normal_reaction_runtime.h"
#include "attack_hit_reaction.h"
#include "attack_hit_side_exit.h"
#include "attack_hit_stance.h"
#include "attack_hit_state_prelude.h"
#include "attack_hit_sound.h"
#include "attack_hit_sound_runtime.h"
#include "attack_hit_sound_rom_view.h"
#include "attack_hit_strength.h"
#include "attack_hit_profile.h"
#include "damage_calculation.h"
#include "damage_unit.h"
#include "skill_accounting.h"
#include "ring_scatter_damage_flow.h"
#include "kamae_motion_rom_view.h"
#include "hit_motion_rom_view.h"

enum {
    FLOW_ATTACKER_SIZE = STF_DAMAGE_DEALER_MIN_SIZE,
    FLOW_DEFENDER_SIZE = STF_DAMAGE_UNIT_DEFENDER_MIN_SIZE
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



static uint32_t float_bits(float value)
{
    uint32_t bits = 0u;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
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
    stf_attack_hit_state_prelude_result state_prelude;
    uint8_t selector_block[STF_SET_KAMAE_FULL_SELECTOR_MIN_SIZE];
    uint8_t stance_ram[2048];
    uint8_t kamae_image[4096];
    stf_kamae_motion_rom_view kamae_rom;
    stf_attack_hit_combo_result combo;
    stf_attack_damage_result damage_transform;
    stf_attack_hit_sound_plan sound;
    stf_attack_hit_sound_resolved sound_resolved;
    uint8_t sound_image[0x400];
    stf_attack_hit_sound_rom_view sound_rom;
    stf_attack_finish_result finish;
    stf_attack_reaction_inputs reaction_in;
    stf_attack_reaction_path reaction_path;
    stf_normal_reaction_result normal;
    stf_normal_reaction_runtime_result normal_runtime;
    uint8_t hit_motion_image[1024];
    stf_hit_motion_rom_view hit_motion_rom;
    stf_motion_prefix_inputs motion_in;
    stf_attack_hit_profile hit_profile;
    stf_attack_hit_motion_runtime_result motion_runtime;
    uint8_t motion_image[256];
    stf_motion_hit_rom_view motion_rom;
    uint8_t hit_profile_table[STF_ATTACK_HIT_PROFILE_RECORD_SIZE];
    stf_motion_prefix_result motion_prefix;
    stf_motion_vector_inputs vector_in;
    stf_motion_vector_result vector_out;
    stf_damage_calculation_result damage;
    stf_damage_unit_effect_state damage_unit_effect;
    stf_damage_unit_result damage_unit;
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
    memset(hit_motion_image, 0, sizeof(hit_motion_image));
    memset(&hit_motion_rom, 0, sizeof(hit_motion_rom));
    memset(&motion_in, 0, sizeof(motion_in));
    memset(&hit_profile, 0, sizeof(hit_profile));
    memset(&motion_runtime, 0, sizeof(motion_runtime));
    memset(motion_image, 0, sizeof(motion_image));
    memset(&motion_rom, 0, sizeof(motion_rom));
    memset(hit_profile_table, 0, sizeof(hit_profile_table));
    memset(sound_image, 0, sizeof(sound_image));
    memset(&sound_rom, 0, sizeof(sound_rom));
    memset(&vector_in, 0, sizeof(vector_in));
    memset(&ring_inputs, 0, sizeof(ring_inputs));
    memset(&damage_unit_effect, 0, sizeof(damage_unit_effect));
    memset(selector_block, 0, sizeof(selector_block));
    memset(stance_ram, 0, sizeof(stance_ram));
    memset(kamae_image, 0, sizeof(kamae_image));
    memset(&kamae_rom, 0, sizeof(kamae_rom));
    memset(ring_slots, 0, sizeof(ring_slots));
    stf_ring_pool_init(&ring_pool);

    attacker[0x822u] = UINT8_C(20);
    attacker[0x843u] = UINT8_C(0);

    write_le32(hit_profile_table + 0x00u, UINT32_C(0x3F800000));
    write_le32(hit_profile_table + 0x04u, UINT32_C(0x3F800000));
    write_le32(hit_profile_table + 0x08u, UINT32_C(0x3F800000));
    write_le32(hit_profile_table + 0x10u, UINT32_C(0x3F800000));
    if (!stf_attack_hit_profile_decode(
            hit_profile_table,
            sizeof(hit_profile_table),
            attacker[0x843u],
            &hit_profile
        )) {
        return 22;
    }
    write_le16(attacker + 0x1ACu, UINT16_C(100));
    write_le16(defender + 0x1ACu, UINT16_C(100));
    write_le32(defender + 0x1F4u, UINT32_C(0x3F800000));
    write_le32(defender + 0x1FCu, UINT32_C(0x00000000));
    write_le32(attacker + 0x1F4u, UINT32_C(0x00000000));
    write_le32(attacker + 0x1FCu, UINT32_C(0x00000000));
    write_le32(defender + 0x20Cu, UINT32_C(0x41200000));
    write_le32(defender + 0x210u, UINT32_C(0x40000000));
    write_le32(defender + 0x214u, UINT32_C(0x41A00000));
    defender[0x1B1u] = UINT8_C(0);
    hit_motion_rom.image = hit_motion_image;
    hit_motion_rom.image_size = sizeof(hit_motion_image);
    hit_motion_rom.base_address = UINT32_C(0x00300000);
    hit_motion_rom.character_table_address = UINT32_C(0x00300020);
    hit_motion_rom.character_count = 4u;
    hit_motion_rom.selector_count = 8u;
    hit_motion_rom.character = defender[0x1B1u];
    write_le32(
        hit_motion_image + 0x20u,
        UINT32_C(0x00300080)
    );
    write_le32(
        hit_motion_image + 0x80u,
        UINT32_C(0x00300180)
    );
    write_le32(
        hit_motion_image + 0x180u + 18u * 4u,
        UINT32_C(0x10)
    );

    motion_rom.image = motion_image;
    motion_rom.image_size = sizeof(motion_image);
    motion_rom.base_address = UINT32_C(0x00100000);
    motion_rom.animation_related_address = UINT32_C(0x00100020);
    motion_rom.animation_count = 64u;
    write_le32(
        motion_image + 0x20u + 0x10u * 4u,
        UINT32_C(0x00100080)
    );
    motion_image[0x80u + 0x0Du] = UINT8_C(0x11);
    write_le16(motion_image + 0x80u + 0x0Eu, UINT16_C(0));
    write_le32(motion_image + 0x80u + 0x10u, UINT32_C(0x3F800000));
    write_le32(defender, UINT32_C(1) << 29u);

    kamae_rom.image = kamae_image;
    kamae_rom.image_size = sizeof(kamae_image);
    kamae_rom.base_address = UINT32_C(0x00200000);
    kamae_rom.offset_list_address = UINT32_C(0x00200020);
    kamae_rom.motion_count = 8u;

    write_le16(selector_block + 0x00u, UINT16_C(1));
    write_le16(selector_block + 0x08u, UINT16_C(2));
    write_le16(selector_block + 0x0Au, UINT16_C(3));
    write_le16(selector_block + 0x50u, UINT16_C(4));

    write_le32(kamae_image + 0x20u + 1u * 4u, UINT32_C(0x00200400));
    write_le32(kamae_image + 0x20u + 2u * 4u, UINT32_C(0x00200600));
    write_le32(kamae_image + 0x20u + 3u * 4u, UINT32_C(0x00200800));
    write_le32(kamae_image + 0x20u + 4u * 4u, UINT32_C(0x00200A00));

    build_flow_motion_record(kamae_image + 0x400u, 10.0f);
    build_flow_motion_record(kamae_image + 0x600u, 20.0f);
    build_flow_motion_record(kamae_image + 0x800u, 30.0f);
    build_flow_motion_record(kamae_image + 0xA00u, 40.0f);

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
            hit_profile.strength_scale_bits,
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
            stf_kamae_motion_resolve_rom,
            &kamae_rom,
            &stance_result
        ) ||
        !stance_result.executed ||
        stance_result.kamae.request_count != 4u ||
        stance_result.kamae.selectors[0] != UINT16_C(1) ||
        stance_result.kamae.selectors[3] != UINT16_C(4) ||
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

    write_le16(attacker + 0x1224u, UINT16_C(1));
    write_le16(attacker + 0x1226u, UINT16_C(0xFFF6));
    write_le32(attacker + 0x1228u, UINT32_C(0xCAFEBABE));
    write_le16(attacker + 0x1248u, UINT16_C(1));
    write_le16(attacker + 0x124Au, UINT16_C(0xFFEC));

    if (!stf_attack_hit_state_prelude_apply_model2(
            attacker, sizeof(attacker), &state_prelude
        ) ||
        state_prelude.attacker_194 != UINT32_C(0x10000001) ||
        !state_prelude.copied_122x ||
        !state_prelude.copied_124x ||
        read_le32(attacker + 0x121Cu) != UINT32_C(0xCAFEBABE) ||
        (int16_t)read_le16(attacker + 0x1220u) != INT16_C(-10) ||
        (int16_t)read_le16(attacker + 0x1244u) != INT16_C(-20)) {
        return 20;
    }

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

    sound_rom.image = sound_image;
    sound_rom.image_size = sizeof(sound_image);
    sound_rom.base_address = UINT32_C(0x000DB600);
    sound_rom.use_schamp_addresses = false;
    write_le32(
        sound_image + (0xDB6F4u - 0xDB600u) + 0u,
        UINT32_C(0x111)
    );
    write_le32(
        sound_image + (0xDB6F4u - 0xDB600u) + 4u,
        UINT32_C(0x222)
    );
    write_le32(
        sound_image + (0xDB6F4u - 0xDB600u) + 8u,
        UINT32_C(0x333)
    );
    if (!stf_attack_hit_sound_resolve_rom(
            &sound, &sound_rom, &sound_resolved
        ) ||
        sound_resolved.id_count != 1u ||
        sound_resolved.ids[0] != UINT32_C(0x222)) {
        return 21;
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

    if (!stf_attack_hit_normal_reaction_apply_resolved_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            UINT16_C(0),
            UINT8_C(0),
            damage_transform.damage,
            damage_transform.hit_mode,
            stf_hit_motion_resolve_rom,
            &hit_motion_rom,
            &normal_runtime
        ) ||
        normal_runtime.motion.table_index != UINT32_C(18) ||
        normal_runtime.motion.motion != UINT32_C(0x10) ||
        normal_runtime.reaction.defender_198 != UINT32_C(0x0B000010) ||
        normal_runtime.reaction.defender_5de != INT16_C(6) ||
        normal_runtime.reaction.reaction_argument != 1 ||
        normal_runtime.reaction.requires_sub_2b94c) {
        return 11;
    }
    normal = normal_runtime.reaction;

    motion_in.initial_r9_bits = strength_bits;
    motion_in.limit_xang = INT16_C(100);
    motion_in.defender_5d8_bits = UINT32_C(0x42700000);

    if (!stf_attack_hit_motion_prefix_resolve_rom(
            normal_runtime.motion.motion,
            &motion_in,
            &motion_rom,
            &hit_profile.fallback,
            &motion_runtime
        ) ||
        motion_runtime.lookup_status != STF_MOTION_HIT_FOUND ||
        !motion_runtime.used_mht_record ||
        motion_runtime.record_offset != UINT32_C(0x8D) ||
        motion_runtime.prefix.source != STF_MOTION_PREFIX_RECORD ||
        motion_runtime.prefix.angle_r6 != 0 ||
        motion_runtime.prefix.sqrt_r4_bits == 0u) {
        return 12;
    }
    motion_prefix = motion_runtime.prefix;

    vector_in.hit_mode = damage_transform.hit_mode;
    vector_in.profile_horizontal_scale_bits =
        hit_profile.horizontal_scale_bits;
    vector_in.profile_vertical_scale_bits =
        hit_profile.vertical_scale_bits;

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

    /*
     * collision calls damage_unit immediately after attack_hit.  Drive the
     * recovered CPU prefix with a category-0 part bit and the same workspace.
     */
    write_le16(defender + 0x6F0u, UINT16_C(1) << 4u);
    write_le32(defender + 0xAF0u, UINT32_C(20));
    write_le32(defender + 0xAF4u, UINT32_C(0x1000));
    if (!stf_damage_unit_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            UINT8_C(2),
            &damage_unit_effect,
            &damage_unit
        ) ||
        damage_unit.skipped ||
        !damage_unit.matched_slot ||
        damage_unit.selected_slot != UINT8_C(4) ||
        damage_unit.accumulator_after != UINT16_C(20) ||
        damage_unit.up_total_1f74 != UINT32_C(20) ||
        damage_unit.down_total_1f78 != UINT32_C(0) ||
        (damage_unit.flags_7f0 & UINT32_C(1)) == 0u) {
        return 23;
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
