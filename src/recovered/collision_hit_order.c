#include "collision_hit_order.h"

#include <string.h>

static uint16_t read_le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8u));
}

static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) |
           ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}

bool stf_collision_hit_order_resolve(
    const stf_collision_hit_order_input *input,
    stf_collision_hit_order *order
)
{
    const uint32_t priority_bit = UINT32_C(1) << 15u;
    bool priority0;
    bool priority1;

    if (input == NULL || order == NULL) {
        return false;
    }

    memset(order, 0, sizeof(*order));
    if (input->hit_latch[1] == 0u) {
        if (input->hit_latch[0] != 0u) {
            order->attacker[0] = 0u;
            order->count = 1u;
        }
        return true;
    }
    if (input->hit_latch[0] == 0u) {
        order->attacker[0] = 1u;
        order->count = 1u;
        return true;
    }

    priority0 = (input->flags_804[0] & priority_bit) != 0u;
    priority1 = (input->flags_804[1] & priority_bit) != 0u;
    order->count = 1u;
    if (priority0 != priority1) {
        order->attacker[0] = priority1 ? 1u : 0u;
    } else if (input->strength_822[1] != input->strength_822[0]) {
        order->attacker[0] =
            input->strength_822[1] > input->strength_822[0] ? 1u : 0u;
    } else if (input->frame_808[1] != input->frame_808[0]) {
        order->attacker[0] =
            input->frame_808[1] < input->frame_808[0] ? 1u : 0u;
    } else {
        order->attacker[0] = 1u;
        order->attacker[1] = 0u;
        order->count = 2u;
    }
    return true;
}

bool stf_collision_hit_order_read_model2(
    const uint8_t *fighter0,
    size_t fighter0_size,
    const uint8_t *fighter1,
    size_t fighter1_size,
    const uint8_t *workspace,
    size_t workspace_size,
    stf_collision_hit_order *order
)
{
    stf_collision_hit_order_input input;
    const uint8_t *fighters[2];
    unsigned i;

    if (fighter0 == NULL || fighter1 == NULL || workspace == NULL ||
        order == NULL ||
        fighter0_size < STF_COLLISION_HIT_ORDER_FIGHTER_MIN_SIZE ||
        fighter1_size < STF_COLLISION_HIT_ORDER_FIGHTER_MIN_SIZE ||
        workspace_size < STF_COLLISION_HIT_ORDER_WORKSPACE_MIN_SIZE) {
        return false;
    }

    fighters[0] = fighter0;
    fighters[1] = fighter1;
    input.hit_latch[0] = read_le16(workspace + 0x274u);
    input.hit_latch[1] = read_le16(workspace + 0x276u);
    for (i = 0u; i < 2u; ++i) {
        input.flags_804[i] = read_le32(fighters[i] + 0x804u);
        input.strength_822[i] = fighters[i][0x822u];
        input.frame_808[i] = read_le16(fighters[i] + 0x808u);
    }
    return stf_collision_hit_order_resolve(&input, order);
}
