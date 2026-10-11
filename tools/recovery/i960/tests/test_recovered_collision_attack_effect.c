#include <stdint.h>
#include <string.h>

#include "collision_attack_effect.h"

int main(void)
{
    stf_collision_attack_result hit;
    uint8_t defender[0x2B4u];
    uint8_t source[12u];
    uint8_t effect[STF_COLLISION_ATTACK_EFFECT_MODEL2_SIZE];
    uint8_t global[12u];
    uint8_t expected[12u];
    unsigned i;

    memset(&hit, 0, sizeof(hit));
    memset(defender, 0, sizeof(defender));
    memset(effect, 0xCD, sizeof(effect));
    memset(global, 0xCD, sizeof(global));
    memset(source, 0xEF, sizeof(source));
    if (!stf_collision_attack_effect_apply_model2(
            &hit, NULL, 0u, NULL, 0u, NULL, 0u, NULL, 0u)) return 1;
    if (stf_collision_attack_effect_apply_model2(
            NULL, defender, sizeof(defender), source, sizeof(source),
            effect, sizeof(effect), global, sizeof(global))) return 2;
    if (effect[0x78u] != 0xCDu || global[0] != 0xCDu) return 3;

    hit.hit = true;
    hit.selected_unit = 15u;
    for (i = 0u; i < 12u; ++i) {
        defender[0x1F4u + 15u*12u + i] = (uint8_t)(0x30u+i);
        source[i] = (uint8_t)(0x70u+i);
        expected[i] = (uint8_t)(0x30u+i);
    }
    /* Preflight: no buffer should have been changed on invalid arguments. */
    if (stf_collision_attack_effect_apply_model2(
            &hit, defender, sizeof(defender)-1u,
            source, sizeof(source), effect, sizeof(effect),
            global, sizeof(global))) return 4;
    if (stf_collision_attack_effect_apply_model2(
            &hit, defender, sizeof(defender),
            source, sizeof(source)-1u, effect, sizeof(effect),
            global, sizeof(global))) return 5;
    if (stf_collision_attack_effect_apply_model2(
            &hit, defender, sizeof(defender),
            source, sizeof(source), effect, sizeof(effect)-1u,
            global, sizeof(global))) return 6;
    if (effect[0x78u] != 0xCDu || global[0] != 0xCDu) return 7;

    if (!stf_collision_attack_effect_apply_model2(
            &hit, defender, sizeof(defender),
            source, sizeof(source), effect, sizeof(effect),
            global, sizeof(global))) return 8;
    if (effect[0x78u] != 10u || effect[0x79u] != 0u ||
        effect[0x7Au] != 0u || effect[0x7Bu] != 0u ||
        memcmp(effect+0x7Cu, expected, 12u) != 0 ||
        memcmp(effect+0x8ECu, expected, 12u) != 0 ||
        memcmp(global, expected, 12u) != 0 ||
        memcmp(effect+0x8F8u, source, 12u) != 0) return 9;

    hit.selected_unit = 16u;
    if (stf_collision_attack_effect_apply_model2(
            &hit, defender, sizeof(defender),
            source, sizeof(source), effect, sizeof(effect),
            global, sizeof(global))) return 10;
    return 0;
}
