#ifndef STF_RECOVERED_COLI_INIT_H
#define STF_RECOVERED_COLI_INIT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    STF_COLI_TASK_MODEL2_SIZE = 0x400u
};

/*
 * Evidence-preserving names are intentionally offset based until later
 * collision recovery establishes their gameplay meaning.
 *
 * update_address is part of the original Model 2 task ABI. A Saturn/portable
 * runtime should replace that mechanism with its native task dispatch rather
 * than treating this value as a host function pointer.
 */
typedef struct stf_coli_init_values {
    uint32_t update_address;
    uint32_t field_090;
    uint16_t field_158;
    uint32_t flags_1b0;
    uint32_t field_1d0_bits;
    uint32_t field_1d4_bits;
    uint32_t field_1d8_bits;
    uint16_t field_20c;
    uint32_t field_278_bits;
} stf_coli_init_values;

void stf_coli_init_values_build(
    stf_coli_init_values *values,
    uint32_t collision_address,
    uint32_t previous_flags_1b0
);

/*
 * Apply exactly the currently recovered coli_init writes to a Model 2 task
 * workspace. Unknown bytes are preserved.
 */
bool stf_coli_init_apply_model2(
    uint8_t *workspace,
    size_t workspace_size,
    uint32_t collision_address
);

#endif
