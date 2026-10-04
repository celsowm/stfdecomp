#include <stdint.h>

#include "skill_accounting.h"

int main(void)
{
    stf_skill_accounting_result out;

    if (!stf_total_skill_add(
            0u, UINT32_C(4), UINT32_C(4), 0u,
            UINT32_C(20), UINT32_C(100), &out
        ) ||
        out.applied || out.total_skill_after != UINT32_C(100)) {
        return 1;
    }

    if (!stf_total_skill_add(
            UINT16_C(0x81), UINT32_C(4), UINT32_C(4), 0u,
            UINT32_C(20), UINT32_C(100), &out
        ) ||
        out.applied) {
        return 2;
    }

    if (!stf_total_skill_add(
            UINT16_C(1), 0u, UINT32_C(4), 0u,
            UINT32_C(20), UINT32_C(100), &out
        ) ||
        out.applied || out.selected_flag != 0u) {
        return 3;
    }

    if (!stf_total_skill_add(
            UINT16_C(1), UINT32_C(4), 0u, 0u,
            UINT32_C(20), UINT32_C(100), &out
        ) ||
        !out.applied ||
        out.selected_flag != UINT32_C(4) ||
        out.total_skill_after != UINT32_C(120)) {
        return 4;
    }

    if (!stf_total_skill_add(
            UINT16_C(1), 0u, UINT32_C(4), 1u,
            UINT32_C(30), UINT32_C(100), &out
        ) ||
        !out.applied ||
        out.selected_flag != UINT32_C(4) ||
        out.total_skill_after != UINT32_C(130)) {
        return 5;
    }

    if (!stf_total_skill_add(
            UINT16_C(1), UINT32_C(4), 0u, 0u,
            UINT32_C(2), UINT32_C(0xFFFFFFFF), &out
        ) ||
        !out.applied ||
        out.total_skill_after != UINT32_C(1)) {
        return 6;
    }

    return 0;
}
