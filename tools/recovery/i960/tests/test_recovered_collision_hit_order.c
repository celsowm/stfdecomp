#include <stdint.h>
#include <string.h>

#include "collision_hit_order.h"

static void write_le16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8u);
}

static void write_le32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8u);
    p[2] = (uint8_t)(v >> 16u);
    p[3] = (uint8_t)(v >> 24u);
}

static int expect(
    const stf_collision_hit_order_input *input,
    uint8_t count,
    uint8_t first,
    uint8_t second
)
{
    stf_collision_hit_order order;
    return stf_collision_hit_order_resolve(input, &order) &&
           order.count == count &&
           (count == 0u || order.attacker[0] == first) &&
           (count < 2u || order.attacker[1] == second);
}

int main(void)
{
    stf_collision_hit_order_input in;
    stf_collision_hit_order out;
    uint8_t f0[STF_COLLISION_HIT_ORDER_FIGHTER_MIN_SIZE];
    uint8_t f1[STF_COLLISION_HIT_ORDER_FIGHTER_MIN_SIZE];
    uint8_t ram[STF_COLLISION_HIT_ORDER_WORKSPACE_MIN_SIZE];

    memset(&in, 0, sizeof(in));
    if (!expect(&in, 0u, 0u, 0u)) return 1;
    in.hit_latch[0] = 1u;
    if (!expect(&in, 1u, 0u, 0u)) return 2;
    in.hit_latch[0] = 0u;
    in.hit_latch[1] = 1u;
    if (!expect(&in, 1u, 1u, 0u)) return 3;

    in.hit_latch[0] = 1u;
    /* Both hit: a lone 0x804 bit 15 wins. */
    in.flags_804[0] = UINT32_C(1) << 15u;
    if (!expect(&in, 1u, 0u, 0u)) return 4;
    in.flags_804[0] = 0u;
    in.flags_804[1] = UINT32_C(1) << 15u;
    if (!expect(&in, 1u, 1u, 0u)) return 5;

    /* Matching flags: 0x822 (unsigned byte) wins before 0x808. */
    in.flags_804[0] = in.flags_804[1];
    in.strength_822[0] = 25u;
    in.strength_822[1] = 31u;
    if (!expect(&in, 1u, 1u, 0u)) return 6;
    in.strength_822[1] = 12u;
    if (!expect(&in, 1u, 0u, 0u)) return 7;

    in.strength_822[1] = 25u;
    in.frame_808[0] = 15u;
    in.frame_808[1] = 9u;
    if (!expect(&in, 1u, 1u, 0u)) return 8;
    in.frame_808[1] = 19u;
    if (!expect(&in, 1u, 0u, 0u)) return 9;

    /* Equal strength and frame: game processes fighter 1, then fighter 0. */
    in.frame_808[1] = 15u;
    if (!expect(&in, 2u, 1u, 0u)) return 10;
    in.flags_804[0] = in.flags_804[1] = 0u;
    if (!expect(&in, 2u, 1u, 0u)) return 11;

    /* Nonzero latches are boolean gates, not equality tests against one. */
    in.hit_latch[0] = UINT16_C(0x8000);
    in.hit_latch[1] = UINT16_C(0xFFFF);
    if (!expect(&in, 2u, 1u, 0u)) return 12;

    if (stf_collision_hit_order_resolve(NULL, &out) ||
        stf_collision_hit_order_resolve(&in, NULL)) return 13;

    memset(f0, 0, sizeof(f0));
    memset(f1, 0, sizeof(f1));
    memset(ram, 0, sizeof(ram));
    write_le16(ram + 0x274u, 1u);
    write_le16(ram + 0x276u, 1u);
    write_le32(f1 + 0x804u, UINT32_C(1) << 15u);
    f0[0x822u] = UINT8_C(200);
    f1[0x822u] = UINT8_C(1);
    write_le16(f0 + 0x808u, 1u);
    write_le16(f1 + 0x808u, 100u);
    if (!stf_collision_hit_order_read_model2(
            f0, sizeof(f0), f1, sizeof(f1), ram, sizeof(ram), &out) ||
        out.count != 1u || out.attacker[0] != 1u) return 14;

    write_le32(f1 + 0x804u, 0u);
    f1[0x822u] = UINT8_C(201);
    if (!stf_collision_hit_order_read_model2(
            f0, sizeof(f0), f1, sizeof(f1), ram, sizeof(ram), &out) ||
        out.count != 1u || out.attacker[0] != 1u) return 15;

    if (stf_collision_hit_order_read_model2(
            f0, sizeof(f0)-1u, f1, sizeof(f1), ram, sizeof(ram), &out) ||
        stf_collision_hit_order_read_model2(
            f0, sizeof(f0), f1, sizeof(f1), ram, sizeof(ram)-1u, &out) ||
        stf_collision_hit_order_read_model2(
            NULL, sizeof(f0), f1, sizeof(f1), ram, sizeof(ram), &out)) return 16;
    return 0;
}
