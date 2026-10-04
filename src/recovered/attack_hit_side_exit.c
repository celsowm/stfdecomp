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

static bool bit32(uint32_t value, unsigned bit)
{
    return (value & (UINT32_C(1) << bit)) != 0u;
}

bool stf_attack_hit_side_exit_apply_model2(
    stf_attack_side_exit_phase phase,
    const uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    stf_attack_side_exit_result *result
)
{
    stf_attack_side_exit_result local = {
        STF_ATTACK_SIDE_EXIT_CONTINUE,
        false,
        false
    };
    uint32_t flags_860 = 0u;

    if (attacker == NULL || defender == NULL || result == NULL ||
        attacker_size < STF_ATTACK_SIDE_EXIT_ATTACKER_MIN_SIZE ||
        defender_size < STF_ATTACK_SIDE_EXIT_DEFENDER_MIN_SIZE) {
        return false;
    }

    flags_860 = read_le32(attacker + 0x860u);

    if (phase == STF_ATTACK_SIDE_EXIT_GUARD_2AC74) {
        if (bit32(flags_860, 19u) && attacker[0x822u] == 0u) {
            local.path = STF_ATTACK_SIDE_EXIT_ABORT_2B8C8;
        }
        *result = local;
        return true;
    }

    if (phase != STF_ATTACK_SIDE_EXIT_NORMAL_2AE40) {
        return false;
    }

    if (!bit32(read_le32(attacker + 0x70Cu), 20u) &&
        bit32(read_le32(attacker + 0x5B8u), 0u)) {
        local.path = STF_ATTACK_SIDE_EXIT_ABORT_2B8C8;
        *result = local;
        return true;
    }

    {
        uint32_t defender_flags = read_le32(defender);

        if (bit32(defender_flags, 29u)) {
            if (bit32(flags_860, 19u)) {
                local.path = STF_ATTACK_SIDE_EXIT_ABORT_2B8C8;
                *result = local;
                return true;
            }

            defender_flags &= ~(UINT32_C(1) << 29u);
            write_le32(defender, defender_flags);
            local.cleared_defender_bit29 = true;
            local.request_set_kamae = true;
        }
    }

    if (bit32(flags_860, 19u)) {
        const uint32_t flags_720 = read_le32(attacker + 0x720u);
        const uint32_t motion_19c = read_le32(attacker + 0x19Cu);

        if (bit32(flags_720, 12u)) {
            local.path = STF_ATTACK_SIDE_EXIT_ABORT_2B8C8;
            *result = local;
            return true;
        }

        if (bit32(flags_720, 6u)) {
            if (bit32(motion_19c, 16u)) {
                local.path = STF_ATTACK_SIDE_EXIT_ABORT_2B8C8;
                *result = local;
                return true;
            }
        } else if (bit32(flags_720, 7u) && bit32(motion_19c, 17u)) {
            local.path = STF_ATTACK_SIDE_EXIT_ABORT_2B8C8;
            *result = local;
            return true;
        }

        if (attacker[0x822u] == 0u) {
            local.path = STF_ATTACK_SIDE_EXIT_ABORT_2B8C8;
        }
    }

    *result = local;
    return true;
}

bool stf_attack_hit_abort_cleanup_apply_model2(
    uint8_t *attacker,
    size_t attacker_size,
    uint8_t *enemy0,
    size_t enemy0_size,
    uint8_t *enemy1,
    size_t enemy1_size,
    stf_attack_abort_cleanup_result *result
)
{
    stf_attack_abort_cleanup_result local;
    uint32_t counter = 0u;

    if (attacker == NULL || enemy0 == NULL || enemy1 == NULL ||
        attacker_size < STF_ATTACK_SIDE_EXIT_ATTACKER_MIN_SIZE ||
        enemy0_size < STF_ATTACK_SIDE_EXIT_ENEMY_MIN_SIZE ||
        enemy1_size < STF_ATTACK_SIDE_EXIT_ENEMY_MIN_SIZE) {
        return false;
    }

    counter = read_le32(attacker + 0x1234u) - UINT32_C(1);
    write_le32(attacker + 0x1234u, counter);

    enemy0[0x108u] = 0u;
    enemy1[0x108u] = 0u;

    local.next_attacker_counter_1234 = counter;
    local.enemy0_108 = 0u;
    local.enemy1_108 = 0u;

    if (result != NULL) {
        *result = local;
    }
    return true;
}
