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

typedef struct stf_crush_part_physics_env {
    uint32_t gravity_bits;
    uint32_t stage_x_bits;
    uint32_t stage_floor_bits;
    uint32_t cage_height_bits;
    uint32_t finish_wall_flags;
    uint32_t effect_active_914;
    uint32_t visibility_mask;
} stf_crush_part_physics_env;

typedef struct stf_crush_part_physics_result {
    bool deactivated;
    bool floor_hit;
    bool request_floor_effect;
    bool stopped_bouncing;
    bool hit_x_wall;
    bool hit_z_wall;
    uint32_t ground_contacts;
    uint32_t flags;
} stf_crush_part_physics_result;

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

/*
 * Recover the CPU-visible body of epc_parts_pos_calc.
 *
 * visibility_mask is the already-computed result of sub_3464C used by the
 * bit-7/bit-3 dormant-part path. request_floor_effect records the original
 * sub_3FA78 call site without invoking its external effect backend.
 */
bool stf_crush_part_update_position_model2(
    uint8_t *slot,
    size_t slot_size,
    const stf_crush_part_physics_env *env,
    stf_crush_part_physics_result *result
);

#endif
