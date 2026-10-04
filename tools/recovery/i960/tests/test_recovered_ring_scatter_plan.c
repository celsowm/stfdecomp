#include <stdint.h>

#include "ring_scatter_plan.h"

static int expect_plan(
    uint32_t damage,
    uint8_t character,
    uint16_t motion,
    uint8_t stage,
    uint8_t rings,
    uint16_t visible,
    uint16_t expire,
    stf_ring_scatter_profile profile,
    stf_ring_drop_mode drop,
    bool sound
)
{
    stf_ring_scatter_inputs inputs;
    stf_ring_scatter_plan plan;

    inputs.damage = damage;
    inputs.defender_character = character;
    inputs.defender_motion_1a8 = motion;
    inputs.stage_num = stage;

    if (!stf_ring_scatter_plan_compute(&inputs, &plan) ||
        plan.suppressed ||
        plan.ring_count != rings ||
        plan.visible_from_frame != visible ||
        plan.expire_at_frame != expire ||
        plan.profile != profile ||
        plan.drop_mode != drop ||
        plan.request_ring_sound != sound) {
        return 1;
    }
    return 0;
}

int main(void)
{
    stf_ring_scatter_inputs inputs;
    stf_ring_scatter_plan plan;
    stf_ring_pool pool;
    uint8_t slot = 0u;
    unsigned index = 0u;

    if (expect_plan(20u, 0u, 0u, 0u, 2u, 60u, 90u,
                    STF_RING_SCATTER_DAMAGE_2, STF_RING_DROP_RING, true) ||
        expect_plan(21u, 0u, 0u, 0u, 4u, 90u, 140u,
                    STF_RING_SCATTER_DAMAGE_4, STF_RING_DROP_RING, true) ||
        expect_plan(31u, 0u, 0u, 0u, 8u, 120u, 180u,
                    STF_RING_SCATTER_DAMAGE_8, STF_RING_DROP_RING, true) ||
        expect_plan(61u, 0u, 0u, 0u, 16u, 150u, 210u,
                    STF_RING_SCATTER_DAMAGE_16, STF_RING_DROP_RING, true)) {
        return 1;
    }

    if (expect_plan(999u, 0u, STF_RING_SPECIAL_MOTION, 0u,
                    8u, 90u, 140u, STF_RING_SCATTER_SPECIAL_MOTION,
                    STF_RING_DROP_FIXED_5, true) ||
        expect_plan(999u, 0u, STF_RING_SPECIAL_MOTION, STF_RING_SPECIAL_STAGE,
                    4u, 90u, 140u, STF_RING_SCATTER_SPECIAL_MOTION,
                    STF_RING_DROP_FIXED_5, true)) {
        return 2;
    }

    if (expect_plan(40u, 11u, 0u, 0u, 8u, 120u, 180u,
                    STF_RING_SCATTER_DAMAGE_8, STF_RING_DROP_RANDOM_1_TO_4,
                    false) ||
        expect_plan(40u, 13u, 0u, 0u, 8u, 120u, 180u,
                    STF_RING_SCATTER_DAMAGE_8, STF_RING_DROP_RANDOM_1_TO_4,
                    false)) {
        return 3;
    }

    inputs.damage = 40u;
    inputs.defender_character = 3u;
    inputs.defender_motion_1a8 = 0u;
    inputs.stage_num = 0u;
    if (!stf_ring_scatter_plan_compute(&inputs, &plan) ||
        !plan.suppressed || plan.ring_count != 0u || plan.request_ring_sound) {
        return 4;
    }
    inputs.defender_character = 14u;
    if (!stf_ring_scatter_plan_compute(&inputs, &plan) || !plan.suppressed) {
        return 5;
    }

    stf_ring_pool_init(&pool);
    if (pool.occupied_mask != 0u || pool.head != STF_RING_POOL_EMPTY) {
        return 6;
    }

    for (index = 0u; index < STF_RING_POOL_SLOT_COUNT; ++index) {
        if (!stf_ring_pool_allocate(&pool, &slot) || slot != index) {
            return 7;
        }
    }
    if (stf_ring_pool_allocate(&pool, &slot) ||
        pool.occupied_mask != UINT32_C(0x00FFFFFF)) {
        return 8;
    }

    stf_ring_pool_release(&pool, 7u);
    if (!stf_ring_pool_allocate(&pool, &slot) || slot != 7u) {
        return 9;
    }

    return 0;
}
