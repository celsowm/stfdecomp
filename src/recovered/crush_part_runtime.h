#ifndef STF_RECOVERED_CRUSH_PART_RUNTIME_H
#define STF_RECOVERED_CRUSH_PART_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_CRUSH_PART_SLOT_SIZE = 0x48u,
    STF_CRUSH_PART_RECORD_SIZE = 0x28u,
    STF_CRUSH_PART_SPEED_SIZE = 0x0Cu,
    STF_CRUSH_PART_SPIN_TABLE_COUNT = 16u,
    STF_CRUSH_PART_MAX_SPEEDS = 4u
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

typedef struct stf_crush_part_visibility_input {
    uint32_t camera_x_bits;
    uint32_t camera_y_bits;
    uint32_t camera_z_bits;
    uint32_t radius_bits;
    uint32_t focus_distance_bits;
} stf_crush_part_visibility_input;

typedef struct stf_crush_part_visibility_result {
    uint32_t mask;
    uint32_t screen_x_bits;
    uint32_t screen_y_bits;
    uint32_t screen_radius_bits;
    bool behind_camera;
} stf_crush_part_visibility_result;

typedef struct stf_crush_part_spawn_input {
    const uint8_t *record;
    size_t record_size;
    uint32_t base_position[3];
    uint32_t velocity[3];
    uint8_t part_index;
    uint8_t record_index;
    uint8_t fighter_flags_byte;
    int16_t fighter_angle_y;
    const int16_t *spin_table;
    size_t spin_table_count;
} stf_crush_part_spawn_input;

typedef struct stf_crush_part_spawn_result {
    bool slot_was_free;
    bool spawned;
    uint8_t spin_index;
    int16_t spin_value;
    uint32_t flags;
    uint16_t object_id;
} stf_crush_part_spawn_result;

typedef struct stf_crush_part_bookkeeping_result {
    bool applied;
    uint8_t history_lane;
    uint32_t stored_record_word;
} stf_crush_part_bookkeeping_result;

typedef struct stf_crush_part_speed_tables {
    uint32_t radial_profile_bits[6];
    uint32_t vertical_profile_bits[6];
    int16_t angle_offsets[16];
    uint32_t jitter_bits[16];
} stf_crush_part_speed_tables;

typedef struct stf_crush_part_speed_input {
    uint8_t count;
    uint8_t body_height_83d;
    uint8_t profile_843;
    uint32_t effect_active_914;
    int16_t fighter_angle_26;
    int16_t fighter_angle_82a;
    uint8_t part_index;
    const uint8_t *records;
    size_t records_size;
    const stf_crush_part_speed_tables *tables;
} stf_crush_part_speed_input;

typedef struct stf_crush_part_speed_request {
    uint8_t jitter_index;
    uint16_t angle;
    uint32_t radial_speed_bits;
    uint32_t vertical_speed_bits;
} stf_crush_part_speed_request;

typedef struct stf_crush_part_speed_result {
    bool count_rejected;
    uint8_t generated;
} stf_crush_part_speed_result;

typedef struct stf_crush_part_set_input {
    const uint8_t *records;
    size_t records_size;
    const uint32_t (*velocities)[3];
    size_t velocity_count;
    uint8_t count;
    uint8_t part_index;
    uint8_t record_index;
    uint32_t effect_active_914;
    uint8_t also_mode;
    uint8_t also_sub_mode;
    const int16_t *spin_table;
    size_t spin_table_count;
} stf_crush_part_set_input;

typedef struct stf_crush_part_set_result {
    bool marked_part_1f40;
    bool spawn_gate_rejected;
    bool slot_occupied_break;
    uint8_t iterations_entered;
    uint8_t weights_applied;
    uint8_t spawned_count;
    stf_crush_part_spawn_result spawn;
    stf_crush_part_bookkeeping_result bookkeeping;
} stf_crush_part_set_result;

typedef struct stf_crush_part_put_input {
    stf_crush_part_speed_input speed;
    uint8_t record_index;
    uint8_t also_mode;
    uint8_t also_sub_mode;
    const int16_t *spin_table;
    size_t spin_table_count;
} stf_crush_part_put_input;

typedef struct stf_crush_part_put_result {
    stf_crush_part_speed_result speed;
    stf_crush_part_set_result set;
    uint32_t velocities[STF_CRUSH_PART_MAX_SPEEDS][3];
} stf_crush_part_put_result;

typedef struct stf_crush_part_floor_sound_result {
    bool request_sound;
    uint8_t table_index;
} stf_crush_part_floor_sound_result;

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

/*
 * Recover the arithmetic after sub_3464C's Model 2 transform command.
 *
 * The original first transforms 0x50A314 through the graphics command path;
 * this helper intentionally starts from the resulting camera-space XYZ. It
 * then applies focus-distance perspective projection and recreates the four
 * visibility bits written to 0x50A368.
 */
bool stf_crush_part_visibility_mask_model2(
    const stf_crush_part_visibility_input *input,
    stf_crush_part_visibility_result *result
);

/* Recover the deterministic slot population performed by efc_crush_parts_set. */
bool stf_crush_part_spawn_model2(
    uint8_t *slot,
    size_t slot_size,
    const stf_crush_part_spawn_input *input,
    stf_crush_part_spawn_result *result
);

/* Mode gate used by delete_parts_weight. */
bool stf_crush_part_should_delete_weight_model2(
    uint8_t also_mode,
    uint8_t also_sub_mode
);

/*
 * Apply delete_parts_weight using record +0x14 as a float value:
 * defender +0x7D8 -= value; defender +0x5D8 = +0x7DC + +0x7D8.
 */
bool stf_crush_part_delete_weight_model2(
    uint8_t *defender,
    size_t defender_size,
    const uint8_t *record,
    size_t record_size,
    uint8_t also_mode,
    uint8_t also_sub_mode,
    bool *applied
);

/*
 * Recover the post-spawn persistence bookkeeping from efc_crush_parts_set.
 * The mode gate stores signed record +0x04 at defender +0x40[part_index] and
 * inserts part_index into the first free lane among +0x1F60/62/64/66.
 */
bool stf_crush_part_bookkeeping_model2(
    uint8_t *defender,
    size_t defender_size,
    const uint8_t *record,
    size_t record_size,
    uint8_t part_index,
    uint8_t also_mode,
    uint8_t also_sub_mode,
    stf_crush_part_bookkeeping_result *result
);

/*
 * Recover the CPU side of efc_crushpts_speed_cont up to the Model 2 math
 * commands 0x24/0x25. Table data comes from the original program image via
 * stf_crush_part_speed_tables, rather than duplicated magic constants. The
 * resulting requests contain the exact angle, radial magnitude and CPU-computed
 * Y velocity sent/used for each record.
 */
bool stf_crush_part_build_speed_requests_model2(
    const stf_crush_part_speed_input *input,
    stf_crush_part_speed_request *requests,
    size_t request_capacity,
    stf_crush_part_speed_result *result
);

/*
 * Complete one speed triple from the two coprocessor outputs exactly as the
 * assembly does: command 0x24 output has its sign bit toggled for X, CPU Y is
 * copied unchanged, and command 0x25 output becomes Z.
 */
bool stf_crush_part_resolve_speed_model2(
    const stf_crush_part_speed_request *request,
    uint32_t command24_output_bits,
    uint32_t command25_output_bits,
    uint32_t velocity_bits[3]
);

/*
 * Resolve the same 0x24/0x25 pair through the already recovered cpres1
 * scaled-trigonometric semantics, removing the synthetic backend-output
 * dependency from normal portable gameplay execution.
 */
bool stf_crush_part_resolve_speed_semantic_model2(
    const stf_crush_part_speed_request *request,
    uint32_t velocity_bits[3]
);

/*
 * Compose efc_crush_parts_set over the single +0x88 slot. This includes the
 * pre-spawn +0x1F40 mark, per-record delete_parts_weight calls, the original
 * bit-3/bit-1 spawn gate, slot population, and post-loop bookkeeping.
 */
bool stf_crush_part_set_model2(
    uint8_t *defender,
    size_t defender_size,
    uint8_t *slot,
    size_t slot_size,
    const stf_crush_part_set_input *input,
    stf_crush_part_set_result *result
);


/*
 * Compose efc_crush_parts_put_cont: build each speed request, execute the
 * recovered 0x24/0x25 trig semantics, then feed the resulting triples into
 * efc_crush_parts_set as one portable transaction.
 */
bool stf_crush_part_put_model2(
    uint8_t *defender,
    size_t defender_size,
    uint8_t *slot,
    size_t slot_size,
    const stf_crush_part_put_input *input,
    stf_crush_part_put_result *result
);

/* Recover sub_3FA78's scanbit selection into no_sfx_or_sd_punch_k. */
bool stf_crush_part_floor_sound_select_model2(
    uint32_t slot_flags,
    stf_crush_part_floor_sound_result *result
);

#endif
