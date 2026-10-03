/*
 * Derived from celsowm/vf2-decomp i960 recovery code.
 * Copyright (c) 2026, vf2-decomp contributors.
 * BSD-3-Clause terms are retained in LICENSE.vf2-decomp in this directory.
 */
#include "stf/recovery/status.h"

const char *stf_status_string(stf_status status)
{
    switch (status) {
    case STF_OK:
        return "ok";
    case STF_ERROR_INVALID_ARGUMENT:
        return "invalid argument";
    case STF_ERROR_IO:
        return "I/O error";
    case STF_ERROR_OUT_OF_MEMORY:
        return "out of memory";
    case STF_ERROR_BAD_SIZE:
        return "unexpected file size";
    case STF_ERROR_BAD_CRC32:
        return "CRC-32 mismatch";
    case STF_ERROR_BAD_SHA1:
        return "SHA-1 mismatch";
    case STF_ERROR_OUT_OF_BOUNDS:
        return "out of bounds";
    case STF_ERROR_UNSUPPORTED:
        return "unsupported operation";
    default:
        return "unknown error";
    }
}
