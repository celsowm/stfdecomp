#ifndef STF_RECOVERED_ATTACK_HIT_STRENGTH_H
#define STF_RECOVERED_ATTACK_HIT_STRENGTH_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_ATTACK_HIT_STRENGTH_FIGHTER_MODEL2_MIN_SIZE = 0x82Cu,
    STF_ATTACK_HIT_STRENGTH_WORKSPACE_MODEL2_MIN_SIZE = 0x270u
};

typedef struct stf_attack_hit_strength_input {
    int32_t combined_82a_026;
    uint8_t raw_822;
    uint32_t raw_822_float_bits;
    uint32_t workspace_26c;
} stf_attack_hit_strength_input;

/*
 * Recover attack_hit 0x2A8F8.. just before the first coprocessor command.
 * This is exact CPU-side state preparation only; no TGP/GEO behavior is
 * synthesized.
 */
bool stf_attack_hit_strength_prepare_model2(
    const uint8_t *fighter,
    size_t fighter_size,
    uint8_t *workspace,
    size_t workspace_size,
    stf_attack_hit_strength_input *result
);

/*
 * Continue the ROM-observed strength formula after raw_822 conversion:
 *
 *   sqrt(raw_strength * 0.01f) * 50.0f * 0.0025f * hit_kind_scale
 *
 * hit_kind_scale is the float read from the per-kind table at
 * 0x50A800 + hit_kind * 40 + 8 in the original program.
 */
bool stf_attack_hit_strength_scale_bits(
    uint32_t raw_strength_float_bits,
    uint32_t hit_kind_scale_bits,
    uint32_t *result_bits
);

#endif
