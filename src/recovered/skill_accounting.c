#include "skill_accounting.h"

#include <stddef.h>

bool stf_total_skill_add(
    uint16_t rank_mode,
    uint32_t select0_flag,
    uint32_t select1_flag,
    uint8_t fighter_slot,
    uint32_t amount,
    uint32_t total_skill_before,
    stf_skill_accounting_result *result
)
{
    stf_skill_accounting_result local;
    const uint32_t selected =
        fighter_slot == UINT8_C(0) ? select0_flag : select1_flag;

    if (result == NULL) {
        return false;
    }

    local.applied = false;
    local.selected_flag = selected;
    local.total_skill_after = total_skill_before;

    if ((rank_mode & UINT16_C(1)) == 0u ||
        (rank_mode & (UINT16_C(1) << 7u)) != 0u ||
        (selected & (UINT32_C(1) << 2u)) == 0u) {
        *result = local;
        return true;
    }

    local.applied = true;
    local.total_skill_after = total_skill_before + amount;
    *result = local;
    return true;
}
