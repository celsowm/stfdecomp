#include <stdint.h>
#include <string.h>
#include "collision_combat_runtime.h"

static void wr16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8u);
}
static void wr32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8u);
    p[2] = (uint8_t)(v >> 16u);
    p[3] = (uint8_t)(v >> 24u);
}
static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8u));
}
static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) |
           ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}
typedef struct hook_data {
    uint8_t attacker[2];
    uint8_t count;
    uint8_t fail_at;
    uint8_t clear_damage_gate_at;
    uint8_t special_seen;
} hook_data;

static bool attack_body(
    void *user,
    uint8_t attacker_index,
    uint8_t defender_index,
    uint8_t *attacker,
    size_t attacker_size,
    uint8_t *defender,
    size_t defender_size,
    uint8_t *workspace,
    size_t workspace_size,
    const stf_attack_hit_prefix_result *prefix
)
{
    hook_data *data = (hook_data *)user;
    if (attacker == NULL || defender == NULL || prefix == NULL ||
        attacker_size < STF_DAMAGE_UNIT_ATTACKER_MIN_SIZE ||
        defender_size < STF_DAMAGE_UNIT_DEFENDER_MIN_SIZE ||
        workspace_size < STF_DAMAGE_UNIT_WORKSPACE_MIN_SIZE ||
        attacker_index == defender_index ||
        data->count >= 2u) return false;
    data->attacker[data->count] = attacker_index;
    ++data->count;
    if (prefix->special_opponent_path) ++data->special_seen;
    if (data->clear_damage_gate_at == data->count) {
        wr32(workspace+0x26Cu, 0u);
    }
    return data->fail_at != data->count;
}

int main(void)
{
    uint8_t fighters[2][STF_DAMAGE_UNIT_DEFENDER_MIN_SIZE];
    uint8_t enemies[2][STF_ATTACK_HIT_ENEMY_MODEL2_MIN_SIZE];
    uint8_t workspace[STF_COLLISION_ATTACK_WORKSPACE_MODEL2_MIN_SIZE];
    stf_damage_unit_effect_state effect;
    stf_collision_combat_model2 model;
    stf_collision_combat_result result;
    stf_collision_hit_order order;
    hook_data hook;

    memset(&model, 0, sizeof(model));
    memset(fighters, 0, sizeof(fighters));
    memset(enemies, 0, sizeof(enemies));
    memset(workspace, 0, sizeof(workspace));
    memset(&effect, 0, sizeof(effect));
    memset(&hook, 0, sizeof(hook));
    model.fighters[0] = fighters[0];
    model.fighters[1] = fighters[1];
    model.fighter_sizes[0] = sizeof(fighters[0]);
    model.fighter_sizes[1] = sizeof(fighters[1]);
    model.enemy_slots[0] = enemies[0];
    model.enemy_slots[1] = enemies[1];
    model.enemy_slot_sizes[0] = sizeof(enemies[0]);
    model.enemy_slot_sizes[1] = sizeof(enemies[1]);
    model.workspace = workspace;
    model.workspace_size = sizeof(workspace);
    model.effect = &effect;
    model.num_rounds_to_win = 2u;
    model.attack_body = attack_body;
    model.attack_body_user_data = &hook;

    fighters[1][4u] = 1u;
    fighters[0][0x821u] = fighters[1][0x821u] = 0u;
    fighters[0][0x822u] = 7u; fighters[1][0x822u] = 11u;
    wr16(fighters[0]+0x6F0u, UINT16_C(1)<<1u);
    wr16(fighters[1]+0x6F0u, UINT16_C(1)<<0u);
    wr32(fighters[0]+0xAF0u, UINT32_MAX);
    wr32(fighters[0]+0xAF4u, UINT32_MAX);
    wr32(fighters[1]+0xAF0u, UINT32_MAX);
    wr32(fighters[1]+0xAF4u, UINT32_MAX);
    wr32(fighters[0]+0x1234u, 20u);
    wr32(fighters[1]+0x1234u, 30u);

    memset(&order, 0, sizeof(order));
    order.count=2u; order.attacker[0]=1u; order.attacker[1]=0u;

    /* Before side effects, validate every target buffer and hook. */
    model.enemy_slot_sizes[1] -= 1u;
    if (stf_collision_combat_apply_model2(&model,&order,&result) ||
        rd32(fighters[1]+0x1234u)!=30u) return 1;
    model.enemy_slot_sizes[1] += 1u;
    model.attack_body=NULL;
    if (stf_collision_combat_apply_model2(&model,&order,&result) ||
        rd32(fighters[1]+0x1234u)!=30u) return 2;
    model.attack_body=attack_body;
    order.attacker[0]=0u; order.attacker[1]=1u;
    if (stf_collision_combat_apply_model2(&model,&order,&result) ||
        rd32(fighters[1]+0x1234u)!=30u) return 3;
    order.attacker[0]=1u; order.attacker[1]=0u;

    if (!stf_collision_combat_apply_model2(&model,&order,&result)) return 4;
    if (result.attempted!=2u || result.completed!=2u ||
        result.pairs[0].attacker_index!=1u ||
        result.pairs[1].attacker_index!=0u ||
        !result.pairs[0].prefix_applied || !result.pairs[0].body_completed ||
        !result.pairs[0].damage_completed ||
        hook.count!=2u || hook.attacker[0]!=1u || hook.attacker[1]!=0u ||
        rd32(fighters[1]+0x1234u)!=31u ||
        rd32(fighters[0]+0x1234u)!=21u ||
        enemies[0][0x108u]!=1u || enemies[1][0x108u]!=1u) return 5;
    if (rd16(fighters[0]+0x1F00u+2u)!=11u ||
        rd16(fighters[1]+0x1F00u)!=7u ||
        result.pairs[0].damage.damage.skipped ||
        result.pairs[1].damage.damage.skipped ||
        !result.pairs[0].damage.post_applied ||
        !result.pairs[1].damage.post_applied ||
        rd16(fighters[0]+0x75Cu)!=(UINT16_C(1)<<1u) ||
        rd16(fighters[1]+0x75Cu)!=1u) return 6;

    /* A special defender routes through sub_19888 and leaves bit0 clear;
       damage_unit must skip without applying more accumulator damage. */
    memset(&hook, 0, sizeof(hook));
    fighters[0][0x19Fu] = UINT8_C(0x12);
    wr16(fighters[0]+0x1F02u, 0u);
    if (!stf_collision_combat_apply_model2(&model,&order,&result) ||
        hook.special_seen!=1u ||
        !result.pairs[0].prefix.special_opponent_path ||
        !result.pairs[0].damage.damage.skipped ||
        rd16(fighters[0]+0x1F02u)!=0u ||
        rd16(fighters[1]+0x1F00u)!=14u) return 7;
    fighters[0][0x19Fu]=0u;

    /* Caller-provided recovered body may reject damage by clearing bit0. */
    memset(&hook, 0, sizeof(hook));
    hook.clear_damage_gate_at=1u;
    wr16(fighters[0]+0x1F02u, 0u);
    if (!stf_collision_combat_apply_model2(&model,&order,&result) ||
        !result.pairs[0].damage.damage.skipped ||
        result.pairs[1].damage.damage.skipped ||
        rd16(fighters[0]+0x1F02u)!=0u) return 8;

    /* Partial progress is explicit when a continuation fails. */
    memset(&hook, 0, sizeof(hook));
    hook.fail_at=2u;
    if (stf_collision_combat_apply_model2(&model,&order,&result) ||
        result.attempted!=2u || result.completed!=1u ||
        !result.pairs[0].damage_completed ||
        result.pairs[1].body_completed!=false ||
        result.pairs[1].damage_completed) return 9;

    /* Empty order never invokes body. */
    memset(&hook, 0, sizeof(hook));
    memset(&order, 0, sizeof(order));
    if (!stf_collision_combat_apply_model2(&model,&order,&result) ||
        result.completed!=0u || hook.count!=0u) return 10;
    return 0;
}
