#include <stdint.h>
#include <string.h>

#include "attack_hit.h"
#include "attack_hit_motion_prefix.h"
#include "attack_hit_motion_vector.h"
#include "attack_hit_side_exit.h"
#include "damage_calculation.h"

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

static int run_accepted_hit(void)
{
    uint8_t attacker[FLOW_ATTACKER_SIZE];
    uint8_t defender[FLOW_DEFENDER_SIZE];
    uint8_t workspace[STF_ATTACK_HIT_WORKSPACE_MODEL2_MIN_SIZE];
    uint8_t enemy0[STF_ATTACK_SIDE_EXIT_ENEMY_MIN_SIZE];
    stf_attack_hit_prefix_result prefix;
    stf_attack_side_exit_result side;
    stf_motion_prefix_inputs motion_in;
    stf_motion_fallback_profile profile;
    stf_motion_prefix_result motion_prefix;
    stf_motion_vector_inputs vector_in;
    stf_motion_vector_result vector_out;
    stf_damage_calculation_result damage;

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    memset(enemy0, 0, sizeof(enemy0));
    memset(&motion_in, 0, sizeof(motion_in));
    memset(&profile, 0, sizeof(profile));
    memset(&vector_in, 0, sizeof(vector_in));

    write_le16(attacker + 0x1ACu, UINT16_C(100));
    write_le16(defender + 0x1ACu, UINT16_C(100));

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

    if (!stf_attack_hit_side_exit_apply_model2(
            STF_ATTACK_SIDE_EXIT_NORMAL_2AE40,
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            &side
        ) || side.path != STF_ATTACK_SIDE_EXIT_CONTINUE) {
        return 3;
    }

    motion_in.initial_r9_bits = UINT32_C(0x41700000); /* 15.0 */
    motion_in.limit_xang = INT16_C(100);
    motion_in.defender_5d8_bits = UINT32_C(0x42700000); /* 60.0 */
    profile.scale_normal_bits = UINT32_C(0x3F800000); /* 1.0 */
    profile.angle_normal = 0;

    if (!stf_attack_hit_motion_prefix_compute(
            &motion_in, NULL, 0u, &profile, &motion_prefix
        ) ||
        motion_prefix.angle_r6 != 0 ||
        motion_prefix.sqrt_r4_bits != UINT32_C(0x3F800000)) {
        return 4;
    }

    if (!stf_attack_hit_motion_vector_compute(
            &motion_prefix, &vector_in, &vector_out
        ) ||
        vector_out.x_5e0_bits != UINT32_C(0x00000000) ||
        vector_out.y_5e4_bits != UINT32_C(0x00000000) ||
        vector_out.z_5e8_bits != UINT32_C(0x3F800000)) {
        return 5;
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
            UINT32_C(20),
            &damage
        )) {
        return 6;
    }

    if (!damage.energy_applied ||
        damage.scaled_damage != 20u ||
        damage.receiver_energy_after != 80 ||
        read_le16(defender + 0x1ACu) != 80u ||
        read_le16(defender + 0x0C54u) != 20u ||
        read_le16(attacker + 0x1F70u) != 20u ||
        read_le32(defender + 0x5E8u) != UINT32_C(0x3F800000) ||
        read_le32(attacker + 0x1234u) != 1u ||
        enemy0[0x108u] != 1u) {
        return 7;
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

    enemy1[0x108u] = 1u;
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
