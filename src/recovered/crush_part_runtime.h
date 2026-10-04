#ifndef STF_RECOVERED_CRUSH_PART_RUNTIME_H
#define STF_RECOVERED_CRUSH_PART_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_CRUSH_PART_SLOT_SIZE = 0x48u
};

typedef struct stf_crush_part_angle_result {
    int16_t angle_x;
    int16_t angle_y;
    int16_t angle_z;
    int16_t step;
} stf_crush_part_angle_result;

typedef struct stf_crush_part_draw {
    bool active;
    bool use_saved_graphics_state;
    uint8_t owner_index;
    uint16_t object_id;
    uint32_t position[3];
    int16_t angle_x;
    int16_t angle_y;
    int16_t angle_z;
} stf_crush_part_draw;

/*
 * Recover epc_parts_ang_calc for the 0x48-byte part slot at
 * mod_fa_effect+0x88.
 *
 * +0x1C/+0x1E/+0x20 are current XYZ angles.
 * +0x28/+0x2A/+0x2C are target XYZ angles.
 * +0x2E is the free-spin increment.
 * +0x3C is the ground-contact counter.
 * +0x24 contains mode flags.
 */
bool stf_crush_part_update_angles_model2(
    uint8_t *slot,
    size_t slot_size,
    stf_crush_part_angle_result *result
);

/*
 * Recover the matching efc_disp draw extraction.
 *
 * +0x22 is the object id. +0x24 bit 0 selects the owner fighter and bit 19
 * wraps set_obj in the original graphics-state save/restore sequence.
 */
bool stf_crush_part_build_draw_model2(
    const uint8_t *slot,
    size_t slot_size,
    stf_crush_part_draw *draw
);

#endif
