#ifndef STF_RECOVERED_COLLISION_HIT_ORDER_H
#define STF_RECOVERED_COLLISION_HIT_ORDER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_COLLISION_HIT_ORDER_FIGHTER_MIN_SIZE = 0x823u,
    STF_COLLISION_HIT_ORDER_WORKSPACE_MIN_SIZE = 0x278u
};

typedef struct stf_collision_hit_order_input {
    uint16_t hit_latch[2];
    uint32_t flags_804[2];
    uint8_t strength_822[2];
    uint16_t frame_808[2];
} stf_collision_hit_order_input;

typedef struct stf_collision_hit_order {
    uint8_t count;
    uint8_t attacker[2];
} stf_collision_hit_order;

/*
 * Recover collision's post-coli_attack_chk priority flow at 0x294D4..0x29534.
 * Each attacker is 0 or 1, in the order attack_hit/damage_unit are called.
 * When both priority fields tie, the original processes fighter 1 then 0.
 * This is a decision only: callers still own the attack/damage side effects.
 */
bool stf_collision_hit_order_resolve(
    const stf_collision_hit_order_input *input,
    stf_collision_hit_order *order
);

/* Read Model 2 fighter/workspace fields without modifying memory. */
bool stf_collision_hit_order_read_model2(
    const uint8_t *fighter0,
    size_t fighter0_size,
    const uint8_t *fighter1,
    size_t fighter1_size,
    const uint8_t *workspace,
    size_t workspace_size,
    stf_collision_hit_order *order
);

#endif
