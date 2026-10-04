#include <stdint.h>
#include <string.h>

#include "attack_hit.h"

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

int main(void)
{
    uint8_t fighter[STF_ATTACK_HIT_FIGHTER_MODEL2_MIN_SIZE];
    uint8_t opponent[STF_ATTACK_HIT_OPPONENT_MODEL2_MIN_SIZE];
    uint8_t workspace[STF_ATTACK_HIT_WORKSPACE_MODEL2_MIN_SIZE];
    uint8_t enemy[STF_ATTACK_HIT_ENEMY_MODEL2_MIN_SIZE];
    stf_attack_hit_prefix_result result;

    memset(fighter, 0, sizeof(fighter));
    memset(opponent, 0, sizeof(opponent));
    memset(workspace, 0xAA, sizeof(workspace));
    memset(enemy, 0, sizeof(enemy));

    fighter[0x843u] = 7u;
    fighter[0x83Eu] = 3u;
    write_le16(fighter + 0x828u, UINT16_C(0x1234));
    write_le32(fighter + 0x1234u, UINT32_C(41));
    opponent[0x19Fu] = 0u;

    if (!stf_attack_hit_prefix_apply_model2(
            fighter, sizeof(fighter),
            opponent, sizeof(opponent),
            workspace, sizeof(workspace),
            enemy, sizeof(enemy),
            &result
        )) {
        return 1;
    }

    if (result.hit_kind_843 != 7 ||
        result.hit_kind_83e != 3 ||
        result.hit_flags_828 != (int16_t)UINT16_C(0x1234) ||
        result.opponent_override ||
        result.special_opponent_path ||
        result.next_workspace_26c != UINT32_C(1) ||
        result.next_fighter_counter_1234 != UINT32_C(42) ||
        read_le32(workspace + 0x26Cu) != UINT32_C(1) ||
        read_le32(fighter + 0x1234u) != UINT32_C(42) ||
        enemy[0x108u] != UINT8_C(1)) {
        return 2;
    }

    memset(fighter, 0, sizeof(fighter));
    memset(opponent, 0, sizeof(opponent));
    memset(workspace, 0xAA, sizeof(workspace));
    memset(enemy, 0, sizeof(enemy));

    write_le32(opponent + 0x000u, UINT32_C(1) << 29u);
    opponent[0x19Fu] = UINT8_C(0x12);

    if (!stf_attack_hit_prefix_apply_model2(
            fighter, sizeof(fighter),
            opponent, sizeof(opponent),
            workspace, sizeof(workspace),
            enemy, sizeof(enemy),
            &result
        )) {
        return 3;
    }

    if (!result.opponent_override ||
        !result.special_opponent_path ||
        result.hit_kind_843 != 4 ||
        result.hit_kind_83e != 2 ||
        result.hit_flags_828 != (int16_t)UINT16_C(0x4006) ||
        result.next_workspace_26c != 0u ||
        read_le32(workspace + 0x26Cu) != 0u) {
        return 4;
    }

    if (stf_attack_hit_prefix_apply_model2(
            fighter, STF_ATTACK_HIT_FIGHTER_MODEL2_MIN_SIZE - 1u,
            opponent, sizeof(opponent),
            workspace, sizeof(workspace),
            enemy, sizeof(enemy),
            &result
        )) {
        return 5;
    }

    return 0;
}
