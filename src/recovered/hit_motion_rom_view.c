#include "hit_motion_rom_view.h"

static uint32_t read_le32(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8u) |
           ((uint32_t)data[2] << 16u) |
           ((uint32_t)data[3] << 24u);
}

bool stf_hit_motion_resolve_rom(
    uint8_t table_selector,
    const uint32_t **table_words,
    size_t *table_word_count,
    void *user_data
)
{
    stf_hit_motion_rom_view *view = (stf_hit_motion_rom_view *)user_data;
    uint64_t character_table_offset = 0u;
    uint64_t character_entry_offset = 0u;
    uint32_t selector_table_address = 0u;
    uint64_t selector_table_offset = 0u;
    uint64_t selector_entry_offset = 0u;
    uint32_t motion_table_address = 0u;
    uint64_t motion_table_offset = 0u;
    size_t index = 0u;

    if (view == NULL || view->image == NULL ||
        table_words == NULL || table_word_count == NULL ||
        (size_t)view->character >= view->character_count ||
        (size_t)table_selector >= view->selector_count ||
        view->character_table_address < view->base_address) {
        return false;
    }

    character_table_offset =
        (uint64_t)view->character_table_address - view->base_address;
    character_entry_offset =
        character_table_offset + (uint64_t)view->character * 4u;

    if (character_entry_offset > view->image_size ||
        4u > view->image_size - (size_t)character_entry_offset) {
        return false;
    }

    selector_table_address =
        read_le32(view->image + (size_t)character_entry_offset);
    if (selector_table_address < view->base_address) {
        return false;
    }

    selector_table_offset =
        (uint64_t)selector_table_address - view->base_address;
    selector_entry_offset =
        selector_table_offset + (uint64_t)table_selector * 4u;

    if (selector_entry_offset > view->image_size ||
        4u > view->image_size - (size_t)selector_entry_offset) {
        return false;
    }

    motion_table_address =
        read_le32(view->image + (size_t)selector_entry_offset);
    if (motion_table_address < view->base_address) {
        return false;
    }

    motion_table_offset =
        (uint64_t)motion_table_address - view->base_address;

    if (motion_table_offset > view->image_size ||
        (size_t)STF_HIT_MOTION_TABLE_WORD_COUNT * 4u >
            view->image_size - (size_t)motion_table_offset) {
        return false;
    }

    for (index = 0u; index < STF_HIT_MOTION_TABLE_WORD_COUNT; ++index) {
        view->scratch_words[index] = read_le32(
            view->image + (size_t)motion_table_offset + index * 4u
        );
    }

    *table_words = view->scratch_words;
    *table_word_count = STF_HIT_MOTION_TABLE_WORD_COUNT;
    return true;
}
