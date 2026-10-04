#include "attack_hit.h"

#include <string.h>

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

static void write_le32(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8u);
    data[2] = (uint8_t)(value >> 16u);
    data[3] = (uint8_t)(value >> 24u);
}

bool stf_attack_hit_prefix_apply_model2(
    uint8_t *fighter,
    size_t fighter_size,
    const uint8_t *opponent,
    size_t opponent_size,
    uint8_t *workspace,
    size_t workspace_size,
    uint8_t *enemy_slot,
    size_t enemy_slot_size,
    stf_attack_hit_prefix_result *result
)
{
    stf_attack_hit_prefix_result local;
    uint32_t opponent_flags = 0u;
    uint32_t counter = 0u;

    if (fighter == NULL || opponent == NULL || workspace == NULL ||
        enemy_slot == NULL ||
        fighter_size < STF_ATTACK_HIT_FIGHTER_MODEL2_MIN_SIZE ||
        opponent_size < STF_ATTACK_HIT_OPPONENT_MODEL2_MIN_SIZE ||
        workspace_size < STF_ATTACK_HIT_WORKSPACE_MODEL2_MIN_SIZE ||
        enemy_slot_size < STF_ATTACK_HIT_ENEMY_MODEL2_MIN_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));

    local.hit_kind_843 = (int8_t)fighter[0x843u];
    local.hit_kind_83e = (int8_t)fighter[0x83Eu];
    local.hit_flags_828 = (int16_t)read_le16(fighter + 0x828u);

    opponent_flags = read_le32(opponent + 0x000u);
    if ((opponent_flags & (UINT32_C(1) << 29u)) != 0u) {
        local.hit_kind_843 = 4;
        local.hit_flags_828 = (int16_t)UINT16_C(0x4006);
        local.hit_kind_83e = 2;
        local.opponent_override = true;
    }

    local.next_workspace_26c = 0u;
    write_le32(workspace + 0x26Cu, 0u);

    counter = read_le32(fighter + 0x1234u) + UINT32_C(1);
    local.next_fighter_counter_1234 = counter;
    write_le32(fighter + 0x1234u, counter);

    enemy_slot[0x108u] = UINT8_C(1);
    local.enemy_slot_108 = UINT8_C(1);

    if (opponent[0x19Fu] == UINT8_C(0x12)) {
        local.special_opponent_path = true;
    } else {
        local.next_workspace_26c = UINT32_C(1);
        write_le32(workspace + 0x26Cu, local.next_workspace_26c);
    }

    if (result != NULL) {
        *result = local;
    }
    return true;
}
