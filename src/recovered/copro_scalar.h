#ifndef STF_RECOVERED_COPRO_SCALAR_H
#define STF_RECOVERED_COPRO_SCALAR_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Recovered scalar coprocessor operation used by command 0x0D001A1A.
 *
 * Evidence:
 * - six STF call sites use one float input and consume one float output;
 * - calc_land_time forms v^2 + 2gh before the command and then uses the
 *   returned value as sqrt(v^2 + 2gh);
 * - attack_hit applies the command to a non-negative strength scalar.
 *
 * This models the recovered semantic operation, not the complete Model 2B
 * coprocessor transport/protocol.
 */
bool stf_copro_scalar_sqrt_bits(uint32_t input_bits, uint32_t *output_bits);

/*
 * Recovered cpres1 scaled trigonometric operations:
 *
 *   0x12002424: sin(angle) * scale
 *   0x12802525: cos(angle) * scale
 *
 * The low 16 bits of angle_word are one full turn over 0x10000 units.
 * These helpers model the semantic operation portably with libm. They do not
 * claim bit-identical reproduction of the cpres1 lookup-table implementation.
 */
bool stf_copro_scalar_sin_scale_bits(
    uint32_t angle_word,
    uint32_t scale_bits,
    uint32_t *output_bits
);

bool stf_copro_scalar_cos_scale_bits(
    uint32_t angle_word,
    uint32_t scale_bits,
    uint32_t *output_bits
);

/*
 * Ring-scatter semantic helpers for the cpres1 0x27/0x29 path.
 *
 * ring_tobitiri_set feeds the horizontal fighter delta to command 0x27,
 * installs the resulting angle as a Y rotation, then sends local X/Z vectors
 * through command 0x29.  These helpers recover that observable behavior
 * without pretending to emulate the complete coprocessor matrix stack.
 */
bool stf_copro_scalar_atan2_angle_bits(
    uint32_t x_bits,
    uint32_t z_bits,
    uint16_t *angle_word
);

bool stf_copro_scalar_rotate_y_xz_bits(
    uint16_t angle_word,
    uint32_t x_bits,
    uint32_t z_bits,
    uint32_t *out_x_bits,
    uint32_t *out_z_bits
);

/*
 * Recovered cpres1 command 0x29 point transform.
 *
 * Matrix layout matches Model 2's 3x4 affine transform:
 *   out_x = x*m[0] + y*m[3] + z*m[6] + m[9]
 *   out_y = x*m[1] + y*m[4] + z*m[7] + m[10]
 *   out_z = x*m[2] + y*m[5] + z*m[8] + m[11]
 *
 * Values are IEEE-754 bit patterns. This models the semantic transform while
 * keeping matrix-stack transport/state outside the portable helper.
 */
bool stf_copro_scalar_transform_point_bits(
    const uint32_t matrix_bits[12],
    const uint32_t input_bits[3],
    uint32_t output_bits[3]
);

/*
 * Recovered cpres1 command-0x77 fine-phase sphere overlap predicate.
 *
 * The SHARC handler scans 32 collision balls per fighter. For each non-zero
 * ball radius it computes 3D center distance and compares it against
 * query_radius + ball_radius. This helper exposes that proven predicate and
 * penetration depth without claiming the still-unresolved packet metadata or
 * aggregate push-out accumulation.
 */
bool stf_copro_collision_sphere_overlap_bits(
    const uint32_t query_xyz_bits[3],
    uint32_t query_radius_bits,
    const uint32_t ball_xyz_bits[3],
    uint32_t ball_radius_bits,
    bool *overlap,
    uint32_t *distance_bits,
    uint32_t *penetration_bits
);

enum {
    STF_COPRO_COMMAND77_FIGHTERS = 2u,
    STF_COPRO_COMMAND77_BALLS = 32u,
    STF_COPRO_COMMAND77_WORDS = 9u
};

typedef struct stf_copro_command77_ball {
    uint32_t position_bits[3];
    uint32_t radius_bits;
    uint8_t unit_index;
} stf_copro_command77_ball;

typedef struct stf_copro_command77_collision_state {
    stf_copro_command77_ball
        balls[STF_COPRO_COMMAND77_FIGHTERS][STF_COPRO_COMMAND77_BALLS];
} stf_copro_command77_collision_state;

/*
 * Semantic recovery of cpres1 command 0x77 (Fn_parts_oidasi).
 *
 * Inputs are the loose sphere XYZ/radius plus the current 32 world-space
 * collision balls for each fighter. Outputs preserve the firmware FIFO order:
 *   [0] push X (float bits)
 *   [1] push Z (float bits)
 *   [2] last fighter hit (0/1, UINT32_MAX when none)
 *   [3] last ball hit
 *   [4] last unit hit
 *   [5] P0 ball mask
 *   [6] P0 unit mask
 *   [7] P1 ball mask
 *   [8] P1 unit mask
 *
 * The firmware's non-obvious push factor is semantically
 * 1 - 2*distance/(query_radius+ball_radius), accumulated for every overlap.
 * The real SHARC obtains the ratio through RECIPS/Newton steps; this helper
 * models the recovered operation with host IEEE arithmetic and therefore does
 * not claim bit-identical SHARC rounding.
 */
bool stf_copro_command77_semantic_bits(
    const uint32_t query_xyz_bits[3],
    uint32_t query_radius_bits,
    const stf_copro_command77_collision_state *state,
    uint32_t output_words[STF_COPRO_COMMAND77_WORDS]
);

#endif
