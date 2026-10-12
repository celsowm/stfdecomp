#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "tgp_command_trace.h"

int main(void)
{
    const uint32_t reference[3] = {
        UINT32_C(0x3F800000), UINT32_C(0xC0000000),
        UINT32_C(0x80000000)
    };
    uint32_t candidate[3];
    uint32_t invalid[3];
    stf_tgp_vector_difference diff;

    memcpy(candidate, reference, sizeof(candidate));
    if (!stf_tgp_command_trace_compare_vectors(
            reference, candidate, &diff) ||
        diff.mismatch_mask != 0u ||
        diff.ulp_distance[0] != 0u ||
        diff.ulp_distance[1] != 0u ||
        diff.ulp_distance[2] != 0u) return 1;

    /* One ULP in +X and -Y; distinguish bitwise signed-zero from
     * floating-point numerical equality in Z. */
    candidate[0] = UINT32_C(0x3F800001);
    candidate[1] = UINT32_C(0xBFFFFFFF);
    candidate[2] = 0u;
    if (!stf_tgp_command_trace_compare_vectors(
            reference, candidate, &diff) ||
        diff.mismatch_mask != 7u ||
        diff.ulp_distance[0] != 1u ||
        diff.ulp_distance[1] != 1u ||
        diff.ulp_distance[2] != 1u) return 2;

    /* Crossing zero has finite, correctly ordered ULP distance. */
    candidate[0] = UINT32_C(0x80000001);
    candidate[1] = reference[1];
    candidate[2] = reference[2];
    if (!stf_tgp_command_trace_compare_vectors(
            reference, candidate, &diff) ||
        (diff.mismatch_mask & 1u) == 0u ||
        diff.ulp_distance[0] != UINT32_C(0x7F000002)) return 3;

    /* Bad reference and candidate values cannot be treated as an
     * equal-comparison success even when identical NaNs are supplied. */
    memcpy(invalid, reference, sizeof(invalid));
    invalid[0] = UINT32_C(0x7FC00000);
    if (stf_tgp_command_trace_compare_vectors(
            invalid, invalid, &diff) ||
        stf_tgp_command_trace_compare_vectors(
            reference, invalid, &diff)) return 4;
    invalid[0] = UINT32_C(0x7F800000);
    if (stf_tgp_command_trace_compare_vectors(
            invalid, reference, &diff)) return 5;
    if (stf_tgp_command_trace_compare_vectors(
            NULL, candidate, &diff) ||
        stf_tgp_command_trace_compare_vectors(
            reference, NULL, &diff) ||
        stf_tgp_command_trace_compare_vectors(
            reference, candidate, NULL)) return 6;
    return 0;
}
