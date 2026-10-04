#include "attack_hit_damage.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

static const float k_combo_scale[9] = {
    1.0f, 0.80000001f, 0.60000002f, 0.40000001f, 0.30000001f,
    0.25f, 0.2f, 0.15000001f, 0.1f
};

static const float k_combo_down_scale[9] = {
    1.0f, 1.0f, 1.0f, 0.80000001f, 0.60000002f,
    0.40000001f, 0.30000001f, 0.2f, 0.1f
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

static int32_t round_nearest_even(float value)
{
    const double x = (double)value;
    const double lower = floor(x);
    const double fraction = x - lower;
    double rounded = 0.0;

    if (fraction < 0.5) {
        rounded = lower;
    } else if (fraction > 0.5) {
        rounded = lower + 1.0;
    } else {
        rounded = fmod(fabs(lower), 2.0) == 0.0 ? lower : lower + 1.0;
    }
    return (int32_t)rounded;
}

static uint32_t scale_damage(uint32_t damage, float scale)
{
    const float converted = (float)(int32_t)damage;
    const float scaled = converted * scale;
    return (uint32_t)round_nearest_even(scaled);
}

bool stf_attack_hit_damage_apply_model2(
    uint8_t *attacker,
    size_t attacker_size,
    const uint8_t *defender,
    size_t defender_size,
    uint32_t initial_damage,
    stf_attack_damage_result *result
)
{
    stf_attack_damage_result local;
    uint32_t damage = initial_damage;
    uint32_t attacker_flags = 0u;
    uint32_t defender_flags = 0u;
    uint32_t defender_state = 0u;
    uint16_t overlap = 0u;
    unsigned combo_index = 0u;

    if (attacker == NULL || defender == NULL || result == NULL ||
        attacker_size < STF_ATTACK_DAMAGE_ATTACKER_MIN_SIZE ||
        defender_size < STF_ATTACK_DAMAGE_DEFENDER_MIN_SIZE) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    local.reason = STF_ATTACK_DAMAGE_HIT;

    attacker_flags = read_le32(attacker + 0x000u);
    defender_flags = read_le32(defender + 0x1A4u);

    if (bit32(attacker_flags, 18u) &&
        read_le16(attacker + 0xC7Cu) < UINT16_C(640)) {
        damage >>= 1u;

        if (bit32(defender_flags, 16u)) {
            damage >>= 2u;
            local.reason = STF_ATTACK_DAMAGE_OC_DP;
            goto finalize;
        }

        combo_index = defender[0x6F4u];
        if (combo_index > 8u) {
            combo_index = 8u;
        }

        if (bit32(defender_flags, 4u)) {
            damage = scale_damage(damage, k_combo_down_scale[combo_index]);
            local.reason = STF_ATTACK_DAMAGE_OC_DOWN;
            goto finalize;
        }

        damage = scale_damage(damage, k_combo_scale[combo_index]);
        local.reason = STF_ATTACK_DAMAGE_ORG_COMBO;
    }

    defender_state = read_le32(defender + 0x000u);
    if (bit32(defender_state, 29u)) {
        damage >>= 1u;
        local.reason = STF_ATTACK_DAMAGE_UKEMI;
        goto finalize;
    }

    if (bit32(defender_flags, 8u)) {
        overlap = read_le16(defender + 0x6F0u);
        if ((overlap & UINT16_C(0xD9B0)) == 0u) {
            damage += (uint32_t)(defender[0x822u] >> 2u);
            damage = (damage * UINT32_C(3)) >> 1u;
            local.reason = STF_ATTACK_DAMAGE_COUNTER;
        } else {
            uint32_t quarter = 0u;
            damage += (uint32_t)(defender[0x822u] >> 3u);
            quarter = damage >> 2u;
            damage += quarter;
            local.reason = STF_ATTACK_DAMAGE_SMALL_COUNTER;
        }
        local.hit_mode = UINT32_C(3);
        local.attacker_194 = UINT32_C(0x10000004);
        write_le32(attacker + 0x194u, local.attacker_194);
        goto finalize;
    }

    if (bit32(read_le32(defender + 0x70Cu), 2u)) {
        damage >>= 1u;
        local.reason = STF_ATTACK_DAMAGE_UKEMI;
        goto finalize;
    }

    if (bit32(defender_flags, 14u) && !bit32(defender_flags, 16u)) {
        damage -= damage >> 2u;
        local.reason = STF_ATTACK_DAMAGE_DOWN;
        goto finalize;
    }

    if (attacker[0x821u] == UINT8_C(8) && bit32(defender_flags, 16u)) {
        damage >>= 1u;
        local.reason = STF_ATTACK_DAMAGE_TETSU;
        goto finalize;
    }

    if ((defender_flags & UINT32_C(0x4010)) == UINT32_C(0x0010)) {
        damage = (damage * UINT32_C(3)) >> 1u;
        local.reason = STF_ATTACK_DAMAGE_AIR;
        goto finalize;
    }

    overlap = read_le16(defender + 0x6F0u);
    if (bit32(defender_flags, 16u) || bit32(defender_flags, 14u)) {
        if ((overlap & UINT16_C(0x264F)) == 0u) {
            damage -= damage >> 2u;
            local.reason = STF_ATTACK_DAMAGE_TENDER;
        }
        goto finalize;
    }

    if (bit32(read_le32(defender + 0x804u), 8u) &&
        bit32(defender_flags, 0u)) {
        damage = damage + (damage >> 3u) + UINT32_C(1);
        local.hit_mode = UINT32_C(34);
        local.reason = STF_ATTACK_DAMAGE_HIT_ALT;
    }

finalize:
    attacker_flags = read_le32(attacker + 0x000u);
    if (bit32(attacker_flags, 16u)) {
        damage >>= 1u;
        local.reason = STF_ATTACK_DAMAGE_MULTI;
    }
    attacker_flags |= UINT32_C(1) << 16u;
    write_le32(attacker + 0x000u, attacker_flags);

    local.damage = damage;
    local.attacker_flags_000 = attacker_flags;
    *result = local;
    return true;
}
