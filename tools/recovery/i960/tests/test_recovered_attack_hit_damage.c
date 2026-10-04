#include <stdint.h>
#include <string.h>

#include "attack_hit_damage.h"

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
    uint8_t attacker[STF_ATTACK_DAMAGE_ATTACKER_MIN_SIZE];
    uint8_t defender[STF_ATTACK_DAMAGE_DEFENDER_MIN_SIZE];
    stf_attack_damage_result result;

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    write_le32(attacker + 0x194u, UINT32_C(0x10000001));

    if (!stf_attack_hit_damage_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            UINT32_C(40), &result
        ) ||
        result.damage != UINT32_C(40) ||
        result.reason != STF_ATTACK_DAMAGE_HIT ||
        result.hit_mode != UINT32_C(2) ||
        result.attacker_194 != UINT32_C(0x10000001) ||
        (read_le32(attacker + 0x000u) & (UINT32_C(1) << 16u)) == 0u) {
        return 1;
    }

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    write_le32(attacker + 0x194u, UINT32_C(0x10000001));
    write_le32(defender + 0x1A4u, UINT32_C(1) << 8u);
    defender[0x822u] = UINT8_C(20);

    if (!stf_attack_hit_damage_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            UINT32_C(40), &result
        ) ||
        result.damage != UINT32_C(67) ||
        result.reason != STF_ATTACK_DAMAGE_COUNTER ||
        result.hit_mode != UINT32_C(3) ||
        result.attacker_194 != UINT32_C(0x10000004) ||
        read_le32(attacker + 0x194u) != UINT32_C(0x10000004)) {
        return 2;
    }

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    write_le32(defender + 0x1A4u, UINT32_C(1) << 8u);
    write_le16(defender + 0x6F0u, UINT16_C(0x0010));
    defender[0x822u] = UINT8_C(16);

    if (!stf_attack_hit_damage_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            UINT32_C(40), &result
        ) ||
        result.damage != UINT32_C(52) ||
        result.reason != STF_ATTACK_DAMAGE_SMALL_COUNTER) {
        return 3;
    }

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    write_le32(attacker + 0x000u, UINT32_C(1) << 18u);
    write_le16(attacker + 0xC7Cu, UINT16_C(100));
    write_le32(defender + 0x1A4u, UINT32_C(1) << 4u);
    defender[0x6F4u] = UINT8_C(4);

    if (!stf_attack_hit_damage_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            UINT32_C(100), &result
        ) ||
        result.damage != UINT32_C(30) ||
        result.reason != STF_ATTACK_DAMAGE_OC_DOWN) {
        return 4;
    }

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    write_le32(defender + 0x000u, UINT32_C(1) << 29u);

    if (!stf_attack_hit_damage_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            UINT32_C(50), &result
        ) ||
        result.damage != UINT32_C(25) ||
        result.reason != STF_ATTACK_DAMAGE_UKEMI) {
        return 5;
    }

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    write_le32(defender + 0x1A4u, UINT32_C(1) << 14u);

    if (!stf_attack_hit_damage_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            UINT32_C(40), &result
        ) ||
        result.damage != UINT32_C(30) ||
        result.reason != STF_ATTACK_DAMAGE_DOWN) {
        return 6;
    }

    memset(attacker, 0, sizeof(attacker));
    memset(defender, 0, sizeof(defender));
    write_le32(attacker + 0x000u, UINT32_C(1) << 16u);

    if (!stf_attack_hit_damage_apply_model2(
            attacker, sizeof(attacker),
            defender, sizeof(defender),
            UINT32_C(40), &result
        ) ||
        result.damage != UINT32_C(20) ||
        result.reason != STF_ATTACK_DAMAGE_MULTI) {
        return 7;
    }

    return 0;
}
