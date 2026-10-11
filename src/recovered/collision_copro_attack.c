#include "collision_copro_attack.h"

#include "collision_ram_init.h"

#include <string.h>

static uint16_t read_u16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8u));
}

static uint32_t read_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) |
           ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}

static void write_u32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8u);
    p[2] = (uint8_t)(v >> 16u);
    p[3] = (uint8_t)(v >> 24u);
}

static bool read_image_word(
    const stf_copro_attack_rom_view *view,
    uint64_t address,
    uint32_t *value
)
{
    uint64_t offset;
    if (view == NULL || value == NULL || view->image == NULL ||
        address < (uint64_t)view->image_base) {
        return false;
    }
    offset = address - (uint64_t)view->image_base;
    if (offset > (uint64_t)view->image_size ||
        (uint64_t)view->image_size - offset < 4u) {
        return false;
    }
    *value = read_u32(view->image + (size_t)offset);
    return true;
}

bool stf_calc_attack_flag_read_model2(
    const uint8_t *fighter,
    size_t fighter_size,
    const stf_copro_attack_rom_view *view,
    stf_copro_attack_flag_result *result
)
{
    stf_copro_attack_flag_result local;
    unsigned i;
    uint32_t profile = 0u;
    uint32_t selector_address;

    if (fighter == NULL || view == NULL || result == NULL ||
        fighter_size < STF_COPRO_ATTACK_FLAG_FIGHTER_MIN_SIZE) {
        return false;
    }
    memset(&local, 0, sizeof(local));
    local.kind_0820 = fighter[0x820u];

    if ((read_u32(fighter + 0x1A4u) & (UINT32_C(1) << 8u)) == 0u ||
        read_u16(fighter + 0x1AAu) < read_u16(fighter + 0x808u) ||
        local.kind_0820 == STF_COPRO_ATTACK_INACTIVE_KIND) {
        *result = local;
        return true;
    }

    if (local.kind_0820 >= view->profile_count ||
        !read_image_word(
            view,
            (uint64_t)view->profile_table_address +
                (uint64_t)local.kind_0820 * 4u,
            &profile)) {
        return false;
    }
    selector_address = read_u32(fighter + 0x1D18u);

    /* i960 bbc r7,r3 tests bit (r7 & 31), then sets bit r6 in g3. */
    for (i = 0u; i < STF_COPRO_ATTACK_SELECTOR_COUNT; ++i) {
        uint32_t selector = 0u;
        if (!read_image_word(
                view, (uint64_t)selector_address + (uint64_t)i * 4u,
                &selector)) {
            return false;
        }
        if ((profile & (UINT32_C(1) << (selector & 31u))) != 0u) {
            local.attack_units_0768 |= UINT32_C(1) << i;
        }
    }

    local.eligible = true;
    local.attack_profile_0ac8 = profile;
    *result = local;
    return true;
}

bool stf_calc_attack_flag_apply_model2(
    uint8_t *fighter,
    size_t fighter_size,
    const stf_copro_attack_rom_view *view,
    stf_copro_attack_flag_result *result
)
{
    stf_copro_attack_flag_result local;
    if (!stf_calc_attack_flag_read_model2(
            fighter, fighter_size, view, &local)) {
        return false;
    }
    write_u32(fighter + 0xAC8u, local.attack_profile_0ac8);
    write_u32(fighter + 0x768u, local.attack_units_0768);
    if (result != NULL) {
        *result = local;
    }
    return true;
}

bool stf_collision_copro_attack_prepare_model2(
    uint8_t *workspace,
    size_t workspace_size,
    uint8_t *fighter0,
    size_t fighter0_size,
    uint8_t *fighter1,
    size_t fighter1_size,
    const stf_copro_attack_rom_view *view,
    stf_copro_attack_flag_result results[2]
)
{
    stf_copro_attack_flag_result local[2];

    if (workspace == NULL || fighter0 == NULL || fighter1 == NULL ||
        view == NULL || results == NULL ||
        workspace_size < STF_COLLISION_RAM_INIT_WORKSPACE_MIN_SIZE ||
        fighter0_size < STF_COPRO_ATTACK_FLAG_FIGHTER_MIN_SIZE ||
        fighter1_size < STF_COPRO_ATTACK_FLAG_FIGHTER_MIN_SIZE ||
        fighter0[4u] != 0u || fighter1[4u] != 1u) {
        return false;
    }
    if (!stf_calc_attack_flag_read_model2(
            fighter0, fighter0_size, view, &local[0]) ||
        !stf_calc_attack_flag_read_model2(
            fighter1, fighter1_size, view, &local[1])) {
        return false;
    }
    write_u32(fighter0 + 0xAC8u, local[0].attack_profile_0ac8);
    write_u32(fighter0 + 0x768u, local[0].attack_units_0768);
    write_u32(fighter1 + 0xAC8u, local[1].attack_profile_0ac8);
    write_u32(fighter1 + 0x768u, local[1].attack_units_0768);
    /* Sizes were checked before writes, so this cannot fail. */
    if (!stf_collision_ram_init_apply_model2(
            workspace, workspace_size,
            fighter0, fighter0_size, fighter1, fighter1_size)) {
        return false;
    }
    results[0] = local[0];
    results[1] = local[1];
    return true;
}
