#include "attack_hit_sound_runtime.h"

#include <string.h>

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8u) |
           ((uint32_t)data[2] << 16u) |
           ((uint32_t)data[3] << 24u);
}

bool stf_attack_hit_sound_resolve(
    const stf_attack_hit_sound_plan *plan,
    const uint8_t *table_bytes,
    size_t table_size,
    stf_attack_hit_sound_resolved *result
)
{
    stf_attack_hit_sound_resolved local;
    size_t offset = 0u;

    if (plan == NULL || result == NULL) {
        return false;
    }

    memset(&local, 0, sizeof(local));

    if (plan->kind == STF_ATTACK_HIT_SOUND_NONE) {
        *result = local;
        return true;
    }

    if (table_bytes == NULL) {
        return false;
    }

    if (plan->kind == STF_ATTACK_HIT_SOUND_SINGLE) {
        if (plan->source_index > SIZE_MAX / 4u) {
            return false;
        }
        offset = (size_t)plan->source_index * 4u;
        if (offset > table_size || 4u > table_size - offset) {
            return false;
        }

        local.ids[0] = read_le32(table_bytes + offset);
        if (local.ids[0] == 0u) {
            return false;
        }
        local.id_count = 1u;
        *result = local;
        return true;
    }

    if (plan->kind != STF_ATTACK_HIT_SOUND_LIST ||
        !plan->zero_terminated_list) {
        return false;
    }

    offset = (size_t)plan->source_index;
    while (local.id_count < STF_ATTACK_HIT_SOUND_MAX_IDS) {
        uint32_t id = 0u;

        if (offset > table_size || 4u > table_size - offset) {
            return false;
        }

        id = read_le32(table_bytes + offset);
        offset += 4u;

        if (id == 0u) {
            *result = local;
            return true;
        }

        local.ids[local.id_count++] = id;
    }

    /* Missing terminator inside the bounded portable contract. */
    return false;
}
