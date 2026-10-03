#include "stf/recovery/model2b_map.h"

#include <stddef.h>

const char *stf_model2b_symbol_hint(uint32_t address)
{
    switch (address) {
    case STF_MODEL2B_GEO_START:
        return "GEO_START";
    case STF_MODEL2B_GEO_PROGRAM_START:
        return "GEO_PROGRAM_START";
    case STF_MODEL2B_COPRO_SHARC_IOP_START:
        return "COPRO_SHARC_IOP_START";
    case STF_MODEL2B_BUFF_RAM_START:
        return "BUFF_RAM_START";
    case STF_MODEL2B_BUFF_RAM_01:
        return "BUFF_RAM_01";
    case STF_MODEL2B_BUFF_RAM_02:
        return "BUFF_RAM_02";
    case STF_MODEL2B_BUFF_RAM_03:
        return "BUFF_RAM_03";
    case STF_MODEL2B_COPRO_CONTROL1_START:
        return "COPRO_CONTROL1_START";
    case STF_MODEL2B_GEO_CTL1_START:
        return "GEO_CTL1_START";
    case STF_MODEL2B_COPRO_STATUS_START:
        return "COPRO_STATUS_START";
    case STF_MODEL2B_MIDI_START:
        return "MIDI_START";
    case STF_MODEL2B_CPU_CONTROL_START:
        return "CPU_CONTROL_START";
    case STF_MODEL2B_IRQ_REQUEST_START:
        return "IRQ_REQUEST_START";
    case STF_MODEL2B_IRQ_ENABLE_START:
        return "IRQ_ENABLE_START";
    case STF_MODEL2B_TIMERS_START:
        return "TIMERS_START";
    case STF_MODEL2B_TIMER_02:
        return "TIMER_02";
    case STF_MODEL2B_TIMER_03:
        return "TIMER_03";
    case STF_MODEL2B_TIMER_04:
        return "TIMER_04";
    case STF_MODEL2B_TILE_DATA_START:
        return "TILE_DATA_START";
    case STF_MODEL2B_CG_DATA_START:
        return "CG_DATA_START";
    case STF_MODEL2B_SCRB_H_PAGE:
        return "scrB_H_page";
    case STF_MODEL2B_SCRB_V_PAGE:
        return "scrB_V_page";
    case STF_MODEL2B_HSYNC_START:
        return "HSYNC_START";
    case STF_MODEL2B_VSYNC_START:
        return "VSYNC_START";
    case STF_MODEL2B_TMAPGFXBASE_START:
        return "TMAPGFXBASE_START";
    case STF_MODEL2B_STAGE_PALETTE_DATA:
        return "STAGE_PALETTE_DATA";
    case STF_MODEL2B_POLY_PALETTE_DATA:
        return "POLY_PALETTE_DATA";
    case STF_MODEL2B_COLORXLAT_START:
        return "COLORXLAT_START";
    case STF_MODEL2B_3D_ZCLIP_START:
        return "_3D_ZCLIP_START";
    case STF_MODEL2B_IO_PORTS:
        return "IO_PORTS";
    case STF_MODEL2B_IO_BILLBOARD:
        return "IO_BILLBOARD";
    case STF_MODEL2B_ANALOG_TO_DIGITAL:
        return "analog_to_digital";
    case STF_MODEL2B_SERIAL_START:
        return "SERIAL_START";
    case STF_MODEL2B_BACKUP_RAM_START:
        return "BACKUP_RAM_START";
    default:
        return NULL;
    }
}

const char *stf_model2b_region_hint(uint32_t address)
{
    if (address >= STF_MODEL2B_GEO_START &&
        address < STF_MODEL2B_GEO_PROGRAM_START) {
        return "geometry-ram";
    }
    if (address >= STF_MODEL2B_BUFF_RAM_START &&
        address < UINT32_C(0x00920000)) {
        return "geometry-buffer-ram";
    }
    if (address >= STF_MODEL2B_COPRO_CONTROL1_START &&
        address < UINT32_C(0x00980018)) {
        return "coprocessor-control";
    }
    if (address >= STF_MODEL2B_IRQ_REQUEST_START &&
        address < UINT32_C(0x00E80008)) {
        return "irq-control";
    }
    if (address >= STF_MODEL2B_TIMERS_START &&
        address < UINT32_C(0x00F00010)) {
        return "timers";
    }
    if (address >= STF_MODEL2B_TILE_DATA_START &&
        address < STF_MODEL2B_CG_DATA_START) {
        return "tile-data";
    }
    if (address >= STF_MODEL2B_STAGE_PALETTE_DATA &&
        address < STF_MODEL2B_POLY_PALETTE_DATA) {
        return "stage-palette";
    }
    if (address >= STF_MODEL2B_POLY_PALETTE_DATA &&
        address < STF_MODEL2B_COLORXLAT_START) {
        return "polygon-palette";
    }
    if (address >= STF_MODEL2B_COLORXLAT_START &&
        address < STF_MODEL2B_3D_ZCLIP_START) {
        return "color-translation";
    }
    if (address >= STF_MODEL2B_IO_PORTS &&
        address < UINT32_C(0x01C00020)) {
        return "io-ports";
    }
    if (address >= STF_MODEL2B_BACKUP_RAM_START &&
        address < UINT32_C(0x01D10000)) {
        return "backup-ram";
    }

    switch (address) {
    case STF_MODEL2B_GEO_PROGRAM_START:
        return "geometry-program";
    case STF_MODEL2B_COPRO_SHARC_IOP_START:
        return "coprocessor-iop";
    case STF_MODEL2B_MIDI_START:
        return "midi";
    case STF_MODEL2B_CPU_CONTROL_START:
        return "cpu-control";
    case STF_MODEL2B_CG_DATA_START:
    case STF_MODEL2B_SCRB_H_PAGE:
    case STF_MODEL2B_SCRB_V_PAGE:
        return "tile-video";
    case STF_MODEL2B_HSYNC_START:
        return "hsync";
    case STF_MODEL2B_VSYNC_START:
        return "vsync";
    case STF_MODEL2B_TMAPGFXBASE_START:
        return "tilemap-gfx-base";
    case STF_MODEL2B_3D_ZCLIP_START:
        return "3d-zclip";
    case STF_MODEL2B_SERIAL_START:
        return "serial";
    default:
        return "unknown";
    }
}
