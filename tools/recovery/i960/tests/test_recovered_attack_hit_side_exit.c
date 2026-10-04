#include <stdint.h>
#include <string.h>

#include "attack_hit.h"
#include "attack_hit_side_exit.h"

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8u) |
           ((uint32_t)data[2] << 16u) |
           ((uint32_t)data[3] << 24u);
}

static void write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

int main(void)
{
    uint8_t attacker[STF_ATTACK_SIDE_EXIT_ATTACKER_MIN_SIZE];
    uint8_t defender[STF_ATTACK_HIT_OPPONENT_MODEL2_MIN_SIZE];
    uint8_t workspace[STF_ATTACK_HIT_WORKSPACE_MODEL2_MIN_SIZE];
    uint8_t enemy0[STF_ATTACK_SIDE_EXIT_ENEMY_MIN_SIZE];
    uint8_t enemy1[STF_ATTACK_SIDE_EXIT_ENEMY_MIN_SIZE];
    stf_attack_side_exit_result side;
    stf_attack_abort_cleanup_result cleanup;
    stf_attack_hit_prefix_result prefix;

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    memset(workspace, 0, sizeof(workspace));
    memset(enemy0, 0, sizeof(enemy0));
    memset(enemy1, 0, sizeof(enemy1));

    if (!stf_attack_hit_side_exit_apply_model2(
            STF_ATTACK_SIDE_EXIT_GUARD_2AC74,
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            &side
        ) || side.path != STF_ATTACK_SIDE_EXIT_CONTINUE) {
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

    memset(attacker, 0, sizeof(attacker));
    write_le32(attacker + 0x5B8u, UINT32_C(1));
    if (!stf_attack_hit_side_exit_apply_model2(
            STF_ATTACK_SIDE_EXIT_NORMAL_2AE40,
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            &side
        ) || side.path != STF_ATTACK_SIDE_EXIT_ABORT_2B8C8) {
        return 3;
    }

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    write_le32(attacker + 0x70Cu, UINT32_C(1) << 20u);
    write_le32(attacker + 0x860u, UINT32_C(1) << 19u);
    write_le32(defender, UINT32_C(1) << 29u);
    if (!stf_attack_hit_side_exit_apply_model2(
            STF_ATTACK_SIDE_EXIT_NORMAL_2AE40,
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            &side
        ) || side.path != STF_ATTACK_SIDE_EXIT_ABORT_2B8C8 ||
        (read_le32(defender) & (UINT32_C(1) << 29u)) == 0u) {
        return 4;
    }

    write_le32(attacker + 0x860u, 0u);
    if (!stf_attack_hit_side_exit_apply_model2(
            STF_ATTACK_SIDE_EXIT_NORMAL_2AE40,
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            &side
        ) || side.path != STF_ATTACK_SIDE_EXIT_CONTINUE ||
        !side.cleared_defender_bit29 || !side.request_set_kamae ||
        (read_le32(defender) & (UINT32_C(1) << 29u)) != 0u) {
        return 5;
    }

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    write_le32(attacker + 0x70Cu, UINT32_C(1) << 20u);
    write_le32(attacker + 0x860u, UINT32_C(1) << 19u);
    write_le32(attacker + 0x720u, UINT32_C(1) << 12u);
    attacker[0x822u] = 1u;
    if (!stf_attack_hit_side_exit_apply_model2(
            STF_ATTACK_SIDE_EXIT_NORMAL_2AE40,
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            &side
        ) || side.path != STF_ATTACK_SIDE_EXIT_ABORT_2B8C8) {
        return 6;
    }

    write_le32(attacker + 0x720u, UINT32_C(1) << 6u);
    write_le32(attacker + 0x19Cu, UINT32_C(1) << 16u);
    if (!stf_attack_hit_side_exit_apply_model2(
            STF_ATTACK_SIDE_EXIT_NORMAL_2AE40,
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            &side
        ) || side.path != STF_ATTACK_SIDE_EXIT_ABORT_2B8C8) {
        return 7;
    }

    write_le32(attacker + 0x720u, UINT32_C(1) << 7u);
    write_le32(attacker + 0x19Cu, UINT32_C(1) << 17u);
    if (!stf_attack_hit_side_exit_apply_model2(
            STF_ATTACK_SIDE_EXIT_NORMAL_2AE40,
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            &side
        ) || side.path != STF_ATTACK_SIDE_EXIT_ABORT_2B8C8) {
        return 8;
    }

    write_le32(attacker + 0x720u, 0u);
    write_le32(attacker + 0x19Cu, 0u);
    attacker[0x822u] = 0u;
    if (!stf_attack_hit_side_exit_apply_model2(
            STF_ATTACK_SIDE_EXIT_NORMAL_2AE40,
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            &side
        ) || side.path != STF_ATTACK_SIDE_EXIT_ABORT_2B8C8) {
        return 9;
    }

    attacker[0x822u] = 1u;
    if (!stf_attack_hit_side_exit_apply_model2(
            STF_ATTACK_SIDE_EXIT_NORMAL_2AE40,
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            &side
        ) || side.path != STF_ATTACK_SIDE_EXIT_CONTINUE) {
        return 10;
    }

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
        ) ||
        read_le32(attacker + 0x1234u) != 1u ||
        enemy0[0x108u] != 1u) {
        return 11;
    }

    write_le32(attacker + 0x860u, UINT32_C(1) << 19u);
    if (!stf_attack_hit_side_exit_apply_model2(
            STF_ATTACK_SIDE_EXIT_GUARD_2AC74,
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            &side
        ) || side.path != STF_ATTACK_SIDE_EXIT_ABORT_2B8C8) {
        return 12;
    }

    enemy1[0x108u] = 1u;
    if (!stf_attack_hit_abort_cleanup_apply_model2(
            attacker, sizeof(attacker),
            enemy0, sizeof(enemy0),
            enemy1, sizeof(enemy1),
            &cleanup
        ) ||
        cleanup.next_attacker_counter_1234 != 0u ||
        read_le32(attacker + 0x1234u) != 0u ||
        enemy0[0x108u] != 0u ||
        enemy1[0x108u] != 0u) {
        return 13;
    }

    return 0;
}
