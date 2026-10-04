#include "attack_hit_motion_runtime.h"

#include <string.h>


bool stf_attack_hit_motion_prefix_resolve_rom(
    uint32_t selected_motion,
    const stf_motion_prefix_inputs *prefix_inputs,
    const stf_motion_hit_rom_view *rom_view,
    const stf_motion_fallback_profile *fallback_profile,
    stf_attack_hit_motion_runtime_result *result
)
{
    stf_attack_hit_motion_runtime_result local;
    uint32_t record_address = 0u;

    if (prefix_inputs == NULL || rom_view == NULL || result == NULL) {
        return false;
    }

    memset(&local, 0, sizeof(local));
    local.lookup_status = stf_motion_hit_rom_find(
        rom_view,
        selected_motion,
        UINT8_C(0x11),
        &record_address
    );

    if (local.lookup_status == STF_MOTION_HIT_INVALID) {
        return false;
    }

    if (local.lookup_status == STF_MOTION_HIT_FOUND) {
        size_t record_offset = 0u;

        if (record_address < rom_view->base_address) {
            return false;
        }

        record_offset = (size_t)(record_address - rom_view->base_address);
        if (record_offset >= rom_view->image_size ||
            7u > rom_view->image_size - record_offset) {
            return false;
        }

        local.record_offset = (uint32_t)record_offset;
        local.used_mht_record = true;

        if (!stf_attack_hit_motion_prefix_compute(
                prefix_inputs,
                rom_view->image + record_offset,
                rom_view->image_size - record_offset,
                NULL,
                &local.prefix
            )) {
            return false;
        }
    } else {
        if (fallback_profile == NULL ||
            !stf_attack_hit_motion_prefix_compute(
                prefix_inputs,
                NULL,
                0u,
                fallback_profile,
                &local.prefix
            )) {
            return false;
        }
    }

    *result = local;
    return true;
}

bool stf_attack_hit_motion_prefix_resolve(
    uint32_t selected_motion,
    const stf_motion_prefix_inputs *prefix_inputs,
    const uint32_t *animation_offsets,
    size_t animation_count,
    const uint8_t *motion_blob,
    size_t motion_blob_size,
    const uint8_t *record_size_by_tag,
    size_t record_size_count,
    const stf_motion_fallback_profile *fallback_profile,
    stf_attack_hit_motion_runtime_result *result
)
{
    stf_attack_hit_motion_runtime_result local;
    uint32_t record_offset = 0u;
    const uint8_t *sizes = record_size_by_tag;
    size_t size_count = record_size_count;

    if (prefix_inputs == NULL || result == NULL) {
        return false;
    }

    memset(&local, 0, sizeof(local));

    if (sizes == NULL) {
        sizes = stf_motion_hit_default_record_sizes(&size_count);
    }

    local.lookup_status = stf_motion_hit_table_find_offset(
        selected_motion,
        UINT8_C(0x11),
        animation_offsets,
        animation_count,
        motion_blob,
        motion_blob_size,
        sizes,
        size_count,
        &record_offset
    );

    if (local.lookup_status == STF_MOTION_HIT_INVALID) {
        return false;
    }

    if (local.lookup_status == STF_MOTION_HIT_FOUND) {
        if ((size_t)record_offset >= motion_blob_size ||
            7u > motion_blob_size - (size_t)record_offset) {
            return false;
        }

        local.record_offset = record_offset;
        local.used_mht_record = true;

        if (!stf_attack_hit_motion_prefix_compute(
                prefix_inputs,
                motion_blob + record_offset,
                motion_blob_size - (size_t)record_offset,
                NULL,
                &local.prefix
            )) {
            return false;
        }
    } else {
        if (fallback_profile == NULL ||
            !stf_attack_hit_motion_prefix_compute(
                prefix_inputs,
                NULL,
                0u,
                fallback_profile,
                &local.prefix
            )) {
            return false;
        }
    }

    *result = local;
    return true;
}
