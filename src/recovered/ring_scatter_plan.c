#include "ring_scatter_plan.h"

#include <stddef.h>
#include <string.h>

static bool is_metal_variant(uint8_t character)
{
    return character == UINT8_C(3) || character == UINT8_C(14);
}

static bool is_egg_variant(uint8_t character)
{
    return character == UINT8_C(11) || character == UINT8_C(13);
}

bool stf_ring_scatter_plan_compute(
    const stf_ring_scatter_inputs *inputs,
    stf_ring_scatter_plan *plan
)
{
    stf_ring_scatter_plan local;

    if (inputs == NULL || plan == NULL) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    local.profile = STF_RING_SCATTER_NONE;
    local.drop_mode = STF_RING_DROP_RING;

    if (is_metal_variant(inputs->defender_character)) {
        local.suppressed = true;
        *plan = local;
        return true;
    }

    local.request_ring_sound = !is_egg_variant(inputs->defender_character);

    if (inputs->defender_motion_1a8 == STF_RING_SPECIAL_MOTION) {
        local.ring_count =
            inputs->stage_num == STF_RING_SPECIAL_STAGE ? UINT8_C(4) : UINT8_C(8);
        local.visible_from_frame = UINT16_C(90);
        local.expire_at_frame = UINT16_C(140);
        local.profile = STF_RING_SCATTER_SPECIAL_MOTION;
        local.drop_mode = STF_RING_DROP_FIXED_5;
    } else if (inputs->damage <= UINT32_C(20)) {
        local.ring_count = UINT8_C(2);
        local.visible_from_frame = UINT16_C(60);
        local.expire_at_frame = UINT16_C(90);
        local.profile = STF_RING_SCATTER_DAMAGE_2;
    } else if (inputs->damage <= UINT32_C(30)) {
        local.ring_count = UINT8_C(4);
        local.visible_from_frame = UINT16_C(90);
        local.expire_at_frame = UINT16_C(140);
        local.profile = STF_RING_SCATTER_DAMAGE_4;
    } else if (inputs->damage <= UINT32_C(60)) {
        local.ring_count = UINT8_C(8);
        local.visible_from_frame = UINT16_C(120);
        local.expire_at_frame = UINT16_C(180);
        local.profile = STF_RING_SCATTER_DAMAGE_8;
    } else {
        local.ring_count = UINT8_C(16);
        local.visible_from_frame = UINT16_C(150);
        local.expire_at_frame = UINT16_C(210);
        local.profile = STF_RING_SCATTER_DAMAGE_16;
    }

    if (is_egg_variant(inputs->defender_character)) {
        local.drop_mode = STF_RING_DROP_RANDOM_1_TO_4;
    }

    *plan = local;
    return true;
}

void stf_ring_pool_init(stf_ring_pool *pool)
{
    if (pool == NULL) {
        return;
    }
    pool->occupied_mask = 0u;
    pool->head = STF_RING_POOL_EMPTY;
}

bool stf_ring_pool_allocate(stf_ring_pool *pool, uint8_t *slot)
{
    unsigned index = 0u;

    if (pool == NULL || slot == NULL) {
        return false;
    }

    for (index = 0u; index < STF_RING_POOL_SLOT_COUNT; ++index) {
        const uint32_t mask = UINT32_C(1) << index;
        if ((pool->occupied_mask & mask) == 0u) {
            pool->occupied_mask |= mask;
            *slot = (uint8_t)index;
            return true;
        }
    }
    return false;
}

void stf_ring_pool_release(stf_ring_pool *pool, uint8_t slot)
{
    if (pool == NULL || slot >= STF_RING_POOL_SLOT_COUNT) {
        return;
    }
    pool->occupied_mask &= ~(UINT32_C(1) << slot);
}
