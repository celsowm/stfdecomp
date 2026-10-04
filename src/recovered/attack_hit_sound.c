#include "attack_hit_sound.h"

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8u) |
           ((uint32_t)data[2] << 16u) |
           ((uint32_t)data[3] << 24u);
}

static bool bit32(uint32_t value, unsigned bit)
{
    return (value & (UINT32_C(1) << bit)) != 0u;
}

static bool uses_db964(uint8_t attack_kind)
{
    return attack_kind == UINT8_C(3) ||
           attack_kind == UINT8_C(4) ||
           attack_kind == UINT8_C(5) ||
           attack_kind == UINT8_C(6) ||
           attack_kind == UINT8_C(0x1A) ||
           attack_kind == UINT8_C(0x1B);
}


bool stf_attack_hit_sound_source_info_get(
    stf_attack_hit_sound_source source,
    stf_attack_hit_sound_source_info *info
)
{
    stf_attack_hit_sound_source_info local;

    if (info == NULL) {
        return false;
    }

    local.sfight_base_address = 0u;
    local.schamp_base_address = 0u;
    local.source_index_is_byte_offset = false;

    switch (source) {
    case STF_ATTACK_HIT_SOUND_SOURCE_OFF_DBE44:
        local.sfight_base_address = UINT32_C(0x000DBE44);
        local.schamp_base_address = UINT32_C(0x000DBF7C);
        local.source_index_is_byte_offset = true;
        break;
    case STF_ATTACK_HIT_SOUND_SOURCE_AUDIO_LIST:
        local.sfight_base_address = UINT32_C(0x000DBECC);
        local.schamp_base_address = UINT32_C(0x000DC004);
        local.source_index_is_byte_offset = true;
        break;
    case STF_ATTACK_HIT_SOUND_SOURCE_OFF_DBF4C:
        local.sfight_base_address = UINT32_C(0x000DBF4C);
        local.schamp_base_address = UINT32_C(0x000DC084);
        local.source_index_is_byte_offset = true;
        break;
    case STF_ATTACK_HIT_SOUND_SOURCE_DWORD_DB6F4:
        local.sfight_base_address = UINT32_C(0x000DB6F4);
        local.schamp_base_address = UINT32_C(0x000DB82C);
        break;
    case STF_ATTACK_HIT_SOUND_SOURCE_DWORD_DB964:
        local.sfight_base_address = UINT32_C(0x000DB964);
        local.schamp_base_address = UINT32_C(0x000DBA9C);
        break;
    case STF_ATTACK_HIT_SOUND_SOURCE_NONE:
    default:
        return false;
    }

    *info = local;
    return true;
}

bool stf_attack_hit_sound_plan_model2(
    const uint8_t *attacker,
    size_t attacker_size,
    const uint8_t *defender,
    size_t defender_size,
    uint32_t damage,
    stf_attack_hit_sound_plan *result
)
{
    stf_attack_hit_sound_plan local = {
        STF_ATTACK_HIT_SOUND_NONE,
        STF_ATTACK_HIT_SOUND_SOURCE_NONE,
        STF_ATTACK_HIT_SOUND_TIER_LIGHT,
        0u,
        false
    };
    const uint8_t voice_selector =
        attacker != NULL && attacker_size >= STF_ATTACK_HIT_SOUND_ATTACKER_MIN_SIZE
            ? attacker[0x823u] : 0u;

    if (attacker == NULL || defender == NULL || result == NULL ||
        attacker_size < STF_ATTACK_HIT_SOUND_ATTACKER_MIN_SIZE ||
        defender_size < STF_ATTACK_HIT_SOUND_DEFENDER_MIN_SIZE) {
        return false;
    }

    if (damage == 0u || bit32(read_le32(attacker + 0x1A4u), 18u)) {
        *result = local;
        return true;
    }

    if (voice_selector != 0u) {
        const uint8_t character = defender[0x1B1u];

        local.kind = STF_ATTACK_HIT_SOUND_LIST;
        local.zero_terminated_list = true;
        local.source_index = (uint32_t)voice_selector - UINT32_C(1);

        if (character == UINT8_C(3)) {
            local.source = STF_ATTACK_HIT_SOUND_SOURCE_OFF_DBF4C;
        } else if (character == UINT8_C(0x0B) ||
                   character == UINT8_C(0x0C) ||
                   character == UINT8_C(0x0D)) {
            local.source = STF_ATTACK_HIT_SOUND_SOURCE_AUDIO_LIST;
        } else {
            local.source = STF_ATTACK_HIT_SOUND_SOURCE_OFF_DBE44;
        }

        *result = local;
        return true;
    }

    local.kind = STF_ATTACK_HIT_SOUND_SINGLE;
    local.source = uses_db964(attacker[0x820u])
        ? STF_ATTACK_HIT_SOUND_SOURCE_DWORD_DB964
        : STF_ATTACK_HIT_SOUND_SOURCE_DWORD_DB6F4;

    if (damage < UINT32_C(15)) {
        local.tier = STF_ATTACK_HIT_SOUND_TIER_LIGHT;
    } else if (damage < UINT32_C(30)) {
        local.tier = STF_ATTACK_HIT_SOUND_TIER_MEDIUM;
    } else {
        local.tier = STF_ATTACK_HIT_SOUND_TIER_HEAVY;
    }

    local.source_index =
        (uint32_t)defender[0x1B0u] * UINT32_C(3) +
        (uint32_t)local.tier;

    *result = local;
    return true;
}
