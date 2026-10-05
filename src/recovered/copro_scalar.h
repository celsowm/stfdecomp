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

#endif
