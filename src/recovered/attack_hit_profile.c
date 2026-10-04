#include "attack_hit_profile.h"

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

bool stf_attack_hit_profile_decode(
    const uint8_t *table_bytes,
    size_t table_size,
    uint8_t hit_kind,
    stf_attack_hit_profile *result
)
{
    size_t offset = 0u;
    const uint8_t *record = NULL;
    stf_attack_hit_profile local;

    if (table_bytes == NULL || result == NULL) {
        return false;
    }

    offset = (size_t)hit_kind * (size_t)STF_ATTACK_HIT_PROFILE_RECORD_SIZE;
    if (offset > table_size ||
        STF_ATTACK_HIT_PROFILE_RECORD_SIZE > table_size - offset) {
        return false;
    }

    record = table_bytes + offset;
    memset(&local, 0, sizeof(local));

    local.horizontal_scale_bits = read_le32(record + 0x00u);
    local.vertical_scale_bits = read_le32(record + 0x04u);
    local.strength_scale_bits = read_le32(record + 0x08u);
    local.unknown_0c = read_le32(record + 0x0Cu);

    local.fallback.scale_normal_bits = read_le32(record + 0x10u);
    local.fallback.scale_mode3_bits = read_le32(record + 0x14u);
    local.fallback.scale_down_bits = read_le32(record + 0x18u);
    local.fallback.scale_down_mode3_bits = read_le32(record + 0x1Cu);

    local.fallback.angle_normal = (int16_t)read_le16(record + 0x20u);
    local.fallback.angle_mode3 = (int16_t)read_le16(record + 0x22u);
    local.fallback.angle_down = (int16_t)read_le16(record + 0x24u);
    local.fallback.angle_down_mode3 = (int16_t)read_le16(record + 0x26u);

    *result = local;
    return true;
}
