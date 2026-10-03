#ifndef STF_I960_DECODER_H
#define STF_I960_DECODER_H

#include <stddef.h>
#include <stdint.h>

#include "stf/recovery/i960/instruction.h"
#include "stf/recovery/status.h"

stf_status stf_i960_decode(
    const uint8_t *image,
    size_t image_size,
    uint32_t address,
    stf_i960_instruction *instruction
);

stf_status stf_i960_format_instruction(
    const stf_i960_instruction *instruction,
    char *output,
    size_t output_size
);

#endif
