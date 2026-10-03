#ifndef STF_STATUS_H
#define STF_STATUS_H

typedef enum stf_status {
    STF_OK = 0,
    STF_ERROR_INVALID_ARGUMENT,
    STF_ERROR_IO,
    STF_ERROR_OUT_OF_MEMORY,
    STF_ERROR_BAD_SIZE,
    STF_ERROR_BAD_CRC32,
    STF_ERROR_BAD_SHA1,
    STF_ERROR_OUT_OF_BOUNDS,
    STF_ERROR_UNSUPPORTED
} stf_status;

const char *stf_status_string(stf_status status);

#endif
