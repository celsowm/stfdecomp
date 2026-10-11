#include "collision_attack_effect.h"

#include <string.h>

bool stf_collision_attack_effect_apply_model2(
    const stf_collision_attack_result *hit,
    const uint8_t *defender,
    size_t defender_size,
    const uint8_t *r5_selected_xyz,
    size_t r5_selected_xyz_size,
    uint8_t *effect,
    size_t effect_size,
    uint8_t *copro_hit_xyz,
    size_t copro_hit_xyz_size
)
{
    size_t xyz_offset;
    const uint8_t *defender_xyz;

    if (hit == NULL) {
        return false;
    }
    if (!hit->hit) {
        return true;
    }
    if (hit->selected_unit >= STF_COLLISION_ATTACK_MAPPING_COUNT) {
        return false;
    }
    xyz_offset = 0x1F4u + (size_t)hit->selected_unit * 12u;
    if (defender == NULL || r5_selected_xyz == NULL ||
        effect == NULL || copro_hit_xyz == NULL ||
        defender_size < xyz_offset + STF_COLLISION_ATTACK_EFFECT_XYZ_SIZE ||
        r5_selected_xyz_size < STF_COLLISION_ATTACK_EFFECT_XYZ_SIZE ||
        effect_size < STF_COLLISION_ATTACK_EFFECT_MODEL2_SIZE ||
        copro_hit_xyz_size < STF_COLLISION_ATTACK_EFFECT_XYZ_SIZE) {
        return false;
    }

    defender_xyz = defender + xyz_offset;
    /* Original store order: triple, countdown, global, second triple. */
    memcpy(effect + 0x7Cu, defender_xyz, STF_COLLISION_ATTACK_EFFECT_XYZ_SIZE);
    effect[0x78u] = 10u;
    effect[0x79u] = 0u;
    effect[0x7Au] = 0u;
    effect[0x7Bu] = 0u;
    memcpy(copro_hit_xyz, defender_xyz, STF_COLLISION_ATTACK_EFFECT_XYZ_SIZE);
    memcpy(effect + 0x8ECu, defender_xyz, STF_COLLISION_ATTACK_EFFECT_XYZ_SIZE);
    memcpy(effect + 0x8F8u, r5_selected_xyz, STF_COLLISION_ATTACK_EFFECT_XYZ_SIZE);
    return true;
}
