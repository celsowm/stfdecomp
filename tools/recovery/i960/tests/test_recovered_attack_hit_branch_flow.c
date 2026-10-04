#include <stdint.h>
#include <string.h>

#include "attack_hit_down_reaction_runtime.h"
#include "attack_hit_guard.h"
#include "attack_hit_guard_common_runtime.h"
#include "attack_hit_guard_state_runtime.h"
#include "attack_hit_reaction.h"

typedef struct fixture {
    uint8_t selector;
    uint32_t words[64];
} fixture;

static bool resolve_table(
    uint8_t selector,
    const uint32_t **table_words,
    size_t *table_word_count,
    void *user_data
)
{
    fixture *fx = (fixture *)user_data;
    if (fx == NULL || table_words == NULL || table_word_count == NULL ||
        selector != fx->selector) {
        return false;
    }
    *table_words = fx->words;
    *table_word_count = sizeof(fx->words) / sizeof(fx->words[0]);
    return true;
}

static void write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

static int run_down_branch(void)
{
    uint8_t attacker[0x82Cu];
    uint8_t defender[STF_DOWN_REACTION_DEFENDER_MIN_SIZE];
    fixture fx;
    stf_attack_reaction_inputs reaction;
    stf_attack_reaction_path path;
    stf_down_motion_runtime_result down;
    stf_motion_prefix_inputs prefix_inputs;
    stf_motion_vector_inputs vector_inputs;
    stf_motion_fallback_profile fallback;
    stf_motion_hit_rom_view rom;
    uint8_t image[1024];

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(&fx, 0, sizeof(fx));
    memset(&reaction, 0, sizeof(reaction));
    memset(&prefix_inputs, 0, sizeof(prefix_inputs));
    memset(&vector_inputs, 0, sizeof(vector_inputs));
    memset(&fallback, 0, sizeof(fallback));
    memset(&rom, 0, sizeof(rom));
    memset(image, 0, sizeof(image));

    fx.selector = UINT8_C(0);
    fx.words[34] = UINT32_C(0x44);

    rom.image = image;
    rom.image_size = sizeof(image);
    rom.base_address = UINT32_C(0x00100000);
    rom.animation_related_address = UINT32_C(0x00100020);
    rom.animation_count = 0x100u;
    write_le32(
        image + 0x20u + 0x44u * 4u,
        UINT32_C(0x00100300)
    );
    image[0x300u + 0x0Du] = UINT8_C(0x11);
    image[0x300u + 0x0Eu] = 0u;
    image[0x300u + 0x0Fu] = 0u;
    write_le32(image + 0x300u + 0x10u, UINT32_C(0x3F800000));

    prefix_inputs.initial_r9_bits = UINT32_C(0x3F800000);
    prefix_inputs.limit_xang = INT16_C(100);
    prefix_inputs.defender_5d8_bits = UINT32_C(0x42700000);
    fallback.scale_normal_bits = UINT32_C(0x3F800000);

    vector_inputs.hit_mode = UINT32_C(2);
    vector_inputs.profile_horizontal_scale_bits = UINT32_C(0x3F800000);
    vector_inputs.profile_vertical_scale_bits = UINT32_C(0x3F800000);

    reaction.defender_flags_1a4 = UINT32_C(1) << 14u;
    reaction.damage = UINT32_C(41);
    reaction.scaled_damage = UINT32_C(41);
    reaction.defender_energy_1ac = INT16_C(100);

    if (!stf_attack_hit_reaction_classify(&reaction, &path) ||
        path != STF_ATTACK_REACTION_DOWN_COMBO) {
        return 1;
    }

    if (!stf_attack_hit_generic_down_motion_apply_resolved_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            UINT16_C(0), UINT8_C(0),
            reaction.damage, true,
            resolve_table, &fx,
            &prefix_inputs, &rom, &fallback, &vector_inputs,
            &down
        ) ||
        down.down.motion.motion != UINT32_C(0x44) ||
        down.down.reaction.selected_motion != UINT32_C(0x44) ||
        down.down.reaction.defender_198 != UINT32_C(0x08000044) ||
        down.down.reaction.requires_sub_2b94c ||
        down.down.reaction.requires_calc_mht ||
        down.prefix.lookup_status != STF_MOTION_HIT_FOUND ||
        !down.prefix.used_mht_record ||
        down.vector.z_5e8_bits == 0u) {
        return 2;
    }

    return 0;
}

static int run_guard_branch(void)
{
    uint8_t attacker[STF_GUARD_COMMON_ATTACKER_MIN_SIZE];
    uint8_t defender[STF_GUARD_COMMON_DEFENDER_MIN_SIZE];
    uint8_t workspace[STF_GUARD_COMMON_WORKSPACE_MIN_SIZE];
    uint8_t enemy[STF_GUARD_COMMON_ENEMY_MIN_SIZE];
    fixture fx;
    stf_attack_guard_inputs guard_in;
    stf_attack_guard_path guard_path;
    stf_guard_common_transaction_result guard;

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    memset(enemy, 0, sizeof(enemy));
    memset(&fx, 0, sizeof(fx));
    memset(&guard_in, 0, sizeof(guard_in));

    fx.selector = UINT8_C(0);
    fx.words[2] = UINT32_C(0x222);

    guard_in.opponent_flags_1a4 = UINT32_C(1) << 13u;
    guard_in.hit_flags_50fe00 = UINT16_C(0);

    if (!stf_attack_hit_guard_classify(&guard_in, &guard_path) ||
        guard_path != STF_ATTACK_GUARD_BRANCH_2AC74) {
        return 1;
    }

    attacker[0x822u] = UINT8_C(15);
    write_le32(attacker + 0x1234u, UINT32_C(5));
    write_le32(attacker + 0x1238u, UINT32_C(9));

    if (!stf_attack_hit_guard_common_apply_transaction_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            enemy, sizeof(enemy),
            guard_in.hit_flags_50fe00,
            UINT8_C(0), UINT8_C(20),
            resolve_table, &fx,
            UINT16_C(1),
            UINT32_C(1) << 2u,
            UINT32_C(0),
            UINT32_C(100),
            &guard
        ) ||
        guard.runtime.motion.motion != UINT32_C(0x222) ||
        guard.runtime.guard.requires_sub_2b94c ||
        guard.runtime.guard.defender_198 != UINT32_C(0x0A000222) ||
        !guard.runtime.guard.request_sound_cane_2d ||
        guard.runtime.guard.skill_amount != UINT32_C(7) ||
        !guard.skill.applied ||
        guard.skill.total_skill_after != UINT32_C(107)) {
        return 2;
    }

    return 0;
}


static int run_guard_block_b(void)
{
    uint8_t attacker[STF_GUARD_STATE_ATTACKER_MIN_SIZE];
    uint8_t defender[STF_GUARD_STATE_DEFENDER_MIN_SIZE];
    uint8_t workspace[STF_GUARD_STATE_WORKSPACE_MIN_SIZE];
    fixture fx;
    stf_attack_guard_inputs guard_in;
    stf_attack_guard_path guard_path;
    stf_guard_state_runtime_result guard;

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    memset(&fx, 0, sizeof(fx));
    memset(&guard_in, 0, sizeof(guard_in));

    fx.selector = UINT8_C(0);
    fx.words[10] = UINT32_C(0x123);

    guard_in.opponent_flags_1a4 = UINT32_C(1) << 13u;
    guard_in.opponent_field_c70 = 2;
    guard_in.hit_flags_50fe00 = UINT16_C(1) << 13u;

    if (!stf_attack_hit_guard_classify(&guard_in, &guard_path) ||
        guard_path != STF_ATTACK_GUARD_BLOCK_B_2AB54) {
        return 1;
    }

    if (!stf_attack_hit_guard_apply_resolved_model2(
            STF_GUARD_BLOCK_B,
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            guard_in.hit_flags_50fe00,
            resolve_table, &fx,
            &guard
        ) ||
        !guard.resolved_motion ||
        guard.motion.motion != UINT32_C(0x123) ||
        guard.guard.requires_sub_2b94c ||
        guard.guard.defender_198 != UINT32_C(0x0B000123)) {
        return 2;
    }

    return 0;
}


static int run_special_bit16_branch(void)
{
    uint8_t attacker[0x82Cu];
    uint8_t defender[STF_DOWN_REACTION_DEFENDER_MIN_SIZE];
    fixture fx;
    stf_attack_reaction_inputs reaction;
    stf_attack_reaction_path path;
    stf_down_reaction_runtime_result special;

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(&fx, 0, sizeof(fx));
    memset(&reaction, 0, sizeof(reaction));

    fx.selector = UINT8_C(0);
    fx.words[40] = UINT32_C(0x55);

    reaction.defender_flags_1a4 = UINT32_C(1) << 16u;
    reaction.damage = UINT32_C(40);
    reaction.scaled_damage = UINT32_C(40);
    reaction.defender_energy_1ac = INT16_C(100);

    if (!stf_attack_hit_reaction_classify(&reaction, &path) ||
        path != STF_ATTACK_REACTION_SPECIAL_BIT16) {
        return 1;
    }

    if (!stf_attack_hit_special_bit16_reaction_apply_resolved_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            UINT16_C(0),
            reaction.damage,
            resolve_table, &fx,
            &special
        ) ||
        special.motion.table_index != UINT32_C(40) ||
        special.motion.motion != UINT32_C(0x55) ||
        special.reaction.requires_sub_2b94c ||
        special.reaction.selected_motion != UINT32_C(0x55) ||
        special.reaction.defender_198 != UINT32_C(0x08010055)) {
        return 2;
    }

    return 0;
}

static int run_guard_block_a(void)
{
    uint8_t attacker[STF_GUARD_STATE_ATTACKER_MIN_SIZE];
    uint8_t defender[STF_GUARD_STATE_DEFENDER_MIN_SIZE];
    uint8_t workspace[STF_GUARD_STATE_WORKSPACE_MIN_SIZE];
    stf_attack_guard_inputs guard_in;
    stf_attack_guard_path guard_path;
    stf_guard_state_runtime_result guard;

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    memset(&guard_in, 0, sizeof(guard_in));

    guard_in.opponent_flags_1a4 = UINT32_C(1) << 13u;
    guard_in.opponent_field_c70 = 2;
    guard_in.hit_flags_50fe00 = UINT16_C(1) << 12u;

    if (!stf_attack_hit_guard_classify(&guard_in, &guard_path) ||
        guard_path != STF_ATTACK_GUARD_BLOCK_A_2AA70) {
        return 1;
    }

    if (!stf_attack_hit_guard_apply_resolved_model2(
            STF_GUARD_BLOCK_A,
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            workspace, sizeof(workspace),
            guard_in.hit_flags_50fe00,
            NULL, NULL,
            &guard
        ) ||
        guard.resolved_motion ||
        guard.guard.requires_sub_2b94c ||
        guard.guard.defender_198 != UINT32_C(0x0A00013D)) {
        return 2;
    }

    return 0;
}

int main(void)
{
    const int down = run_down_branch();
    const int special = run_special_bit16_branch();
    const int guard = run_guard_branch();
    const int guard_a = run_guard_block_a();
    const int guard_b = run_guard_block_b();

    if (down != 0) {
        return down;
    }
    if (special != 0) {
        return 100 + special;
    }
    if (guard != 0) {
        return 200 + guard;
    }
    if (guard_a != 0) {
        return 300 + guard_a;
    }
    if (guard_b != 0) {
        return 400 + guard_b;
    }
    return 0;
}
