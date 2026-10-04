#include "attack_hit_motion_select.h"

#include <math.h>
#include <string.h>

static float bits_to_float(uint32_t bits)
{
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

bool stf_attack_hit_motion_select(
    const stf_hit_motion_select_inputs *inputs,
    stf_hit_motion_table_resolver resolver,
    void *resolver_user_data,
    stf_hit_motion_select_result *result
)
{
    stf_hit_motion_select_result local;
    const uint32_t *table = NULL;
    size_t table_count = 0u;
    uint32_t index = 0u;

    if (inputs == NULL || resolver == NULL || result == NULL) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    local.table_selector = (uint8_t)inputs->hit_flags_50fe00;

    if (!resolver(
            local.table_selector,
            &table,
            &table_count,
            resolver_user_data
        ) ||
        table == NULL) {
        return false;
    }

    if (inputs->motion_group == UINT32_C(5)) {
        index = UINT32_C(40);
        local.used_special_group5_index = true;
    } else {
        uint32_t variant = 0u;
        uint32_t parity =
            ((uint32_t)inputs->hit_flags_50fe00 >> 15u) ^
            ((inputs->attacker_flags_0 ^ inputs->defender_flags_0) >> 6u) ^
            (inputs->defender_flags_1a4 >> 21u);

        variant = parity & UINT32_C(1);

        {
            const int32_t distance_test =
                (int32_t)inputs->attacker_82a +
                (int32_t)inputs->attacker_26 -
                (int32_t)inputs->defender_5b4 +
                INT32_C(0x4000);

            if ((distance_test & INT32_C(0x8000)) == 0) {
                variant += UINT32_C(2);
                local.used_distance_variant = true;
            }
        }

        if (inputs->motion_group > (UINT32_MAX - variant) / UINT32_C(8)) {
            return false;
        }
        index = variant + inputs->motion_group * UINT32_C(8);
    }

    if ((size_t)index >= table_count) {
        return false;
    }

    local.table_index = index;
    local.motion = table[index];

    if (local.motion == UINT32_C(225)) {
        const float y = bits_to_float(inputs->defender_1f8_bits);
        if (!isfinite(y)) {
            return false;
        }
        if (y <= 0.9f) {
            local.motion = UINT32_C(0x106);
            local.used_down_override = true;
        }
    }

    *result = local;
    return true;
}
