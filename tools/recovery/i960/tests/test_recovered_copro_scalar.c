#include <stdint.h>

#include "copro_scalar.h"

int main(void)
{
    uint32_t out = 0u;

    if (!stf_copro_scalar_sqrt_bits(UINT32_C(0x40800000), &out) ||
        out != UINT32_C(0x40000000)) { /* sqrt(4.0) = 2.0 */
        return 1;
    }

    if (!stf_copro_scalar_sqrt_bits(UINT32_C(0x3F800000), &out) ||
        out != UINT32_C(0x3F800000)) {
        return 2;
    }

    if (!stf_copro_scalar_sqrt_bits(UINT32_C(0x00000000), &out) ||
        out != UINT32_C(0x00000000)) {
        return 3;
    }

    if (stf_copro_scalar_sqrt_bits(UINT32_C(0xBF800000), &out)) {
        return 4;
    }

    return 0;
}
