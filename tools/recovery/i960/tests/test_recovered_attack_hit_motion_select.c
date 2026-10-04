#include <stdint.h>
#include <string.h>

#include "attack_hit_motion_select.h"

typedef struct fixture {
    uint8_t selector;
    uint32_t words[64];
} fixture;

static uint32_t float_bits(float value)
{
    uint32_t bits = 0u;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static bool resolve_table(
    uint8_t selector,
    const uint32_t **table_words,
    size_t *table_word_count,
    void *user_data
)
{
    fixture *fx = (fixture *)user_data;

    if (fx == NULL || table_words == NULL || table_word_count == NULL ||
        selector != fx->selector) {
        return false;
    }

    *table_words = fx->words;
    *table_word_count = sizeof(fx->words) / sizeof(fx->words[0]);
    return true;
}

int main(void)
{
    fixture fx;
    stf_hit_motion_select_inputs inputs;
    stf_hit_motion_select_result result;
    size_t index = 0u;

    memset(&fx, 0, sizeof(fx));
    memset(&inputs, 0, sizeof(inputs));

    fx.selector = UINT8_C(2);
    for (index = 0u; index < 64u; ++index) {
        fx.words[index] = UINT32_C(1000) + (uint32_t)index;
    }

    inputs.hit_flags_50fe00 = UINT16_C(2);
    inputs.motion_group = UINT32_C(1);
    inputs.defender_1f8_bits = float_bits(2.0f);

    if (!stf_attack_hit_motion_select(
            &inputs, resolve_table, &fx, &result
        ) ||
        result.table_selector != UINT8_C(2) ||
        result.table_index != UINT32_C(10) ||
        result.motion != UINT32_C(1010) ||
        !result.used_distance_variant ||
        result.used_special_group5_index ||
        result.used_down_override) {
        return 1;
    }

    inputs.motion_group = UINT32_C(5);
    if (!stf_attack_hit_motion_select(
            &inputs, resolve_table, &fx, &result
        ) ||
        result.table_index != UINT32_C(40) ||
        result.motion != UINT32_C(1040) ||
        !result.used_special_group5_index) {
        return 2;
    }

    inputs.motion_group = UINT32_C(4);
    inputs.attacker_flags_0 = UINT32_C(1) << 6u;
    inputs.defender_flags_0 = 0u;
    inputs.defender_flags_1a4 = 0u;
    inputs.attacker_82a = INT16_C(0);
    inputs.attacker_26 = INT16_C(0);
    inputs.defender_5b4 = INT16_C(0x5000);
    if (!stf_attack_hit_motion_select(
            &inputs, resolve_table, &fx, &result
        ) ||
        result.table_index != UINT32_C(33) ||
        result.motion != UINT32_C(1033) ||
        result.used_distance_variant) {
        return 3;
    }

    memset(&inputs, 0, sizeof(inputs));
    inputs.hit_flags_50fe00 = UINT16_C(2);
    inputs.motion_group = UINT32_C(4);
    inputs.defender_1f8_bits = float_bits(0.9f);
    fx.words[34] = UINT32_C(225);

    if (!stf_attack_hit_motion_select(
            &inputs, resolve_table, &fx, &result
        ) ||
        result.table_index != UINT32_C(34) ||
        result.motion != UINT32_C(0x106) ||
        !result.used_down_override) {
        return 4;
    }

    inputs.defender_1f8_bits = float_bits(1.0f);
    if (!stf_attack_hit_motion_select(
            &inputs, resolve_table, &fx, &result
        ) ||
        result.motion != UINT32_C(225) ||
        result.used_down_override) {
        return 5;
    }

    fx.selector = UINT8_C(9);
    if (stf_attack_hit_motion_select(
            &inputs, resolve_table, &fx, &result
        )) {
        return 6;
    }

    return 0;
}
