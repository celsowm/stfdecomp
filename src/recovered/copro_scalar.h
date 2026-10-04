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

#endif
