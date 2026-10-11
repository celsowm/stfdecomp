#include "collision_ram_init.h"

#include <string.h>

bool stf_collision_ram_init_apply_model2(
    uint8_t *workspace,
    size_t workspace_size,
    uint8_t *fighter0,
    size_t fighter0_size,
    uint8_t *fighter1,
    size_t fighter1_size
)
{
    if (workspace == NULL || fighter0 == NULL || fighter1 == NULL ||
        workspace_size < STF_COLLISION_RAM_INIT_WORKSPACE_MIN_SIZE ||
        fighter0_size < STF_COLLISION_RAM_INIT_FIGHTER_MIN_SIZE ||
        fighter1_size < STF_COLLISION_RAM_INIT_FIGHTER_MIN_SIZE) {
        return false;
    }

    /* i960 movq 0,r12; stq r12,+0x170,+0x180,+0x190,+0x1A0 */
    memset(workspace + 0x170u, 0, 0x40u);

    /* i960 stores at +0x75C,+0x75E and stl at +0xA68. */
    memset(fighter0 + 0x75Cu, 0, 4u);
    memset(fighter0 + 0xA68u, 0, 8u);
    memset(fighter1 + 0x75Cu, 0, 4u);
    memset(fighter1 + 0xA68u, 0, 8u);

    /* Five scalar st instructions at +0x98,+0x9C,+0xA0,+0xA4,+0xA8. */
    memset(workspace + 0x98u, 0, 20u);
    return true;
}
