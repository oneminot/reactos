/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            bios.c
 * PURPOSE:         VDM BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "bios.h"

#include "vga.h"
#include "pic.h"
#include "ps2.h"
#include "timer.h"

#include "int32.h"
#include "registers.h"

/* MACROS *********************************************************************/

//
// These macros are defined for ease-of-use of some VGA I/O ports
// whose addresses depend whether we are in Monochrome or Colour mode.
//
#define VGA_INSTAT1_READ    Bda->CrtBasePort + 6    // VGA_INSTAT1_READ_MONO or VGA_INSTAT1_READ_COLOR
#define VGA_CRTC_INDEX      Bda->CrtBasePort        // VGA_CRTC_INDEX_MONO   or VGA_CRTC_INDEX_COLOR
#define VGA_CRTC_DATA       Bda->CrtBasePort + 1    // VGA_CRTC_DATA_MONO    or VGA_CRTC_DATA_COLOR

/* PRIVATE VARIABLES **********************************************************/

PBIOS_DATA_AREA Bda;
static BYTE BiosKeyboardMap[256];
static HANDLE BiosConsoleInput  = INVALID_HANDLE_VALUE;
static HANDLE BiosConsoleOutput = INVALID_HANDLE_VALUE;
static DWORD BiosSavedConInMode, BiosSavedConOutMode;
static CONSOLE_CURSOR_INFO        BiosSavedCursorInfo;
static CONSOLE_SCREEN_BUFFER_INFO BiosSavedBufferInfo;

/*
 * VGA Register Configurations for BIOS Video Modes
 * The configurations come from DOSBox.
 */
static VGA_REGISTERS VideoMode_40x25_text =
{
    /* Miscellaneous Register */
    0x67,

    /* Sequencer Registers */
    {0x00, 0x08, 0x03, 0x00, 0x07},

    /* CRTC Registers */
    {0x2D, 0x27, 0x28, 0x90, 0x2B, 0xA0, 0xBF, 0x1F, 0x00, 0x4F, 0x0D, 0x0E,
     0x00, 0x00, 0x00, 0x00, 0x9C, 0x8E, 0x8F, 0x14, 0x1F, 0x96, 0xB9, 0xA3,
     0xFF},

    /* GC Registers */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x0F, 0xFF},

    /* AC Registers */
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07, 0x38, 0x39, 0x3A, 0x3B,
     0x3C, 0x3D, 0x3E, 0x3F, 0x0C, 0x00, 0x0F, 0x08, 0x00}
};

static VGA_REGISTERS VideoMode_80x25_text =
{
    /* Miscellaneous Register */
    0x67,

    /* Sequencer Registers */
    {0x00, 0x00, 0x03, 0x00, 0x07},

    /* CRTC Registers */
    {0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F, 0x00, 0x4F, 0x0D, 0x0E,
     0x00, 0x00, 0x00, 0x00, 0x9C, 0x8E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3,
     0xFF},

    /* GC Registers */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x0E, 0x0F, 0xFF},

    /* AC Registers */
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07, 0x38, 0x39, 0x3A, 0x3B,
     0x3C, 0x3D, 0x3E, 0x3F, 0x0C, 0x00, 0x0F, 0x08, 0x00}
};

static VGA_REGISTERS VideoMode_320x200_4color =
{
    /* Miscellaneous Register */
    0x63,

    /* Sequencer Registers */
    {0x00, 0x09, 0x03, 0x00, 0x02},

    /* CRTC Registers */
    {0x2D, 0x27, 0x28, 0x90, 0x2B, 0x80, 0xBF, 0x1F, 0x00, 0xC1, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x9C, 0x8E, 0x8F, 0x14, 0x00, 0x96, 0xB9, 0xA2,
     0xFF},

    /* GC Registers */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x0F, 0x0F, 0xFF},

    /* AC Registers */
    {0x00, 0x13, 0x15, 0x17, 0x02, 0x04, 0x06, 0x07, 0x10, 0x11, 0x12, 0x13,
     0x14, 0x15, 0x16, 0x17, 0x01, 0x00, 0x0F, 0x00, 0x00}
};

static VGA_REGISTERS VideoMode_640x200_2color =
{
    /* Miscellaneous Register */
    0x63,

    /* Sequencer Registers */
    {0x00, 0x09, 0x0F, 0x00, 0x02},

    /* CRTC Registers */
    {0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F, 0x00, 0xC1, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x9C, 0x8E, 0x8F, 0x28, 0x00, 0x96, 0xB9, 0xC2,
     0xFF},

    /* GC Registers */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0xFF},

    /* AC Registers */
    {0x00, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17,
     0x17, 0x17, 0x17, 0x17, 0x01, 0x00, 0x01, 0x00, 0x00}
};

static VGA_REGISTERS VideoMode_320x200_16color =
{
    /* Miscellaneous Register */
    0x63,

    /* Sequencer Registers */
    {0x00, 0x09, 0x0F, 0x00, 0x02},

    /* CRTC Registers */
    {0x2D, 0x27, 0x28, 0x90, 0x2B, 0x80, 0xBF, 0x1F, 0x00, 0xC0, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x9C, 0x8E, 0x8F, 0x14, 0x00, 0x96, 0xB9, 0xE3,
     0xFF},

    /* GC Registers */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0F, 0xFF},

    /* AC Registers */
//     {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07, 0x38, 0x39, 0x3A, 0x3B,
//      0x3C, 0x3D, 0x3E, 0x3F, 0x01, 0x00, 0x0F, 0x00, 0x00}
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x10, 0x11, 0x12, 0x13,
     0x14, 0x15, 0x16, 0x17, 0x01, 0x00, 0x0F, 0x00, 0x00}
};

static VGA_REGISTERS VideoMode_640x200_16color =
{
    /* Miscellaneous Register */
    0x63,

    /* Sequencer Registers */
    {0x00, 0x01, 0x0F, 0x00, 0x02},

    /* CRTC Registers */
    {0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F, 0x00, 0xC0, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x9C, 0x8E, 0x8F, 0x28, 0x00, 0x96, 0xB9, 0xE3,
     0xFF},

    /* GC Registers */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0F, 0xFF},

    /* AC Registers */
//     {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07, 0x38, 0x39, 0x3A, 0x3B,
//      0x3C, 0x3D, 0x3E, 0x3F, 0x01, 0x00, 0x0F, 0x00, 0x00}
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x10, 0x11, 0x12, 0x13,
     0x14, 0x15, 0x16, 0x17, 0x01, 0x00, 0x0F, 0x00, 0x00}
};

static VGA_REGISTERS VideoMode_640x350_16color =
{
    /* Miscellaneous Register */
    0xA3,

    /* Sequencer Registers */
    {0x00, 0x01, 0x0F, 0x00, 0x02},

    /* CRTC Registers */
    {0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F, 0x00, 0x40, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x83, 0x85, 0x5D, 0x28, 0x0F, 0x63, 0xBA, 0xE3,
     0xFF},

    /* GC Registers */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0F, 0xFF},

    /* AC Registers */
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07, 0x38, 0x39, 0x3A, 0x3B,
     0x3C, 0x3D, 0x3E, 0x3F, 0x01, 0x00, 0x0F, 0x00, 0x00}
};

static VGA_REGISTERS VideoMode_640x480_2color =
{
    /* Miscellaneous Register */
    0xE3,

    /* Sequencer Registers */
    {0x00, 0x01, 0x0F, 0x00, 0x02},

    /* CRTC Registers */
    {0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E, 0x00, 0x40, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0xEA, 0x8C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xC3,
     0xFF},

    /* GC Registers */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0F, 0xFF},

    /* AC Registers */
    {0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
     0x3F, 0x3F, 0x3F, 0x3F, 0x01, 0x00, 0x0F, 0x00, 0x00}
};

static VGA_REGISTERS VideoMode_640x480_16color =
{
    /* Miscellaneous Register */
    0xE3,

    /* Sequencer Registers */
    {0x00, 0x01, 0x0F, 0x00, 0x02},

    /* CRTC Registers */
    {0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E, 0x00, 0x40, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0xEA, 0x8C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xE3,
     0xFF},

    /* GC Registers */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x0F, 0xFF},

    /* AC Registers */
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07, 0x38, 0x39, 0x3A, 0x3B,
     0x3C, 0x3D, 0x3E, 0x3F, 0x01, 0x00, 0x0F, 0x00, 0x00}
};

static VGA_REGISTERS VideoMode_320x200_256color =
{
    /* Miscellaneous Register */
    0x63,

    /* Sequencer Registers */
    {0x00, 0x01, 0x0F, 0x00, 0x0E},

    /* CRTC Registers */
    {0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F, 0x00, 0x41, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x9C, 0x8E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
     0xFF},

    /* GC Registers */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF},

    /* AC Registers */
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B,
     0x0C, 0x0D, 0x0E, 0x0F, 0x41, 0x00, 0x0F, 0x00, 0x00}
};

/* See http://wiki.osdev.org/Drawing_In_Protected_Mode#Locating_Video_Memory */
static PVGA_REGISTERS VideoModes[BIOS_MAX_VIDEO_MODE + 1] =
{
    &VideoMode_40x25_text,          /* Mode 00h */      // 16 color (mono)
    &VideoMode_40x25_text,          /* Mode 01h */      // 16 color
    &VideoMode_80x25_text,          /* Mode 02h */      // 16 color (mono)
    &VideoMode_80x25_text,          /* Mode 03h */      // 16 color
    &VideoMode_320x200_4color,      /* Mode 04h */      // CGA 4 color
    &VideoMode_320x200_4color,      /* Mode 05h */      // CGA same (m)
    &VideoMode_640x200_2color,      /* Mode 06h */      // CGA 640*200 2 color
    NULL,                           /* Mode 07h */      // MDA monochrome text 80*25
    NULL,                           /* Mode 08h */      // PCjr
    NULL,                           /* Mode 09h */      // PCjr
    NULL,                           /* Mode 0Ah */      // PCjr
    NULL,                           /* Mode 0Bh */      // Reserved
    NULL,                           /* Mode 0Ch */      // Reserved
    &VideoMode_320x200_16color,     /* Mode 0Dh */      // EGA 320*200 16 color
    &VideoMode_640x200_16color,     /* Mode 0Eh */      // EGA 640*200 16 color
    NULL,                           /* Mode 0Fh */      // EGA 640*350 mono
    &VideoMode_640x350_16color,     /* Mode 10h */      // EGA 640*350 HiRes 16 color
    &VideoMode_640x480_2color,      /* Mode 11h */      // VGA 640*480 mono
    &VideoMode_640x480_16color,     /* Mode 12h */      // VGA
    &VideoMode_320x200_256color,    /* Mode 13h */      // VGA
};

// FIXME: Are they computable with the previous data ??
// Values taken from DOSBox.
static WORD VideoModePageSize[BIOS_MAX_VIDEO_MODE + 1] =
{
    0x0800, 0x0800, 0x1000, 0x1000,
    0x4000, 0x4000, 0x4000, 0x1000,
    0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x2000, 0x4000, 0x8000,
    0x8000, 0xA000, 0xA000, 0x2000
};

/*
 * BIOS Mode Palettes
 *
 * Many people have different versions of those palettes
 * (e.g. DOSBox, http://www.brokenthorn.com/Resources/OSDevVid2.html ,
 * etc...) A choice should be made at some point.
 */

// This is the same as EgaPalette__HiRes
static CONST COLORREF TextPalette[VGA_MAX_COLORS / 4] =
{
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0xAA), RGB(0x00, 0xAA, 0x00), RGB(0x00, 0xAA, 0xAA),
    RGB(0xAA, 0x00, 0x00), RGB(0xAA, 0x00, 0xAA), RGB(0xAA, 0xAA, 0x00), RGB(0xAA, 0xAA, 0xAA),
    RGB(0x00, 0x00, 0x55), RGB(0x00, 0x00, 0xFF), RGB(0x00, 0xAA, 0x55), RGB(0x00, 0xAA, 0xFF),
    RGB(0xAA, 0x00, 0x55), RGB(0xAA, 0x00, 0xFF), RGB(0xAA, 0xAA, 0x55), RGB(0xAA, 0xAA, 0xFF),

    RGB(0x00, 0x55, 0x00), RGB(0x00, 0x55, 0xAA), RGB(0x00, 0xFF, 0x00), RGB(0x00, 0xFF, 0xAA),
    RGB(0xAA, 0x55, 0x00), RGB(0xAA, 0x55, 0xAA), RGB(0xAA, 0xFF, 0x00), RGB(0xAA, 0xFF, 0xAA),
    RGB(0x00, 0x55, 0x55), RGB(0x00, 0x55, 0xFF), RGB(0x00, 0xFF, 0x55), RGB(0x00, 0xFF, 0xFF),
    RGB(0xAA, 0x55, 0x55), RGB(0xAA, 0x55, 0xFF), RGB(0xAA, 0xFF, 0x55), RGB(0xAA, 0xFF, 0xFF),


    RGB(0x55, 0x00, 0x00), RGB(0x55, 0x00, 0xAA), RGB(0x55, 0xAA, 0x00), RGB(0x55, 0xAA, 0xAA),
    RGB(0xFF, 0x00, 0x00), RGB(0xFF, 0x00, 0xAA), RGB(0xFF, 0xAA, 0x00), RGB(0xFF, 0xAA, 0xAA),
    RGB(0x55, 0x00, 0x55), RGB(0x55, 0x00, 0xFF), RGB(0x55, 0xAA, 0x55), RGB(0x55, 0xAA, 0xFF),
    RGB(0xFF, 0x00, 0x55), RGB(0xFF, 0x00, 0xFF), RGB(0xFF, 0xAA, 0x55), RGB(0xFF, 0xAA, 0xFF),

    RGB(0x55, 0x55, 0x00), RGB(0x55, 0x55, 0xAA), RGB(0x55, 0xFF, 0x00), RGB(0x55, 0xFF, 0xAA),
    RGB(0xFF, 0x55, 0x00), RGB(0xFF, 0x55, 0xAA), RGB(0xFF, 0xFF, 0x00), RGB(0xFF, 0xFF, 0xAA),
    RGB(0x55, 0x55, 0x55), RGB(0x55, 0x55, 0xFF), RGB(0x55, 0xFF, 0x55), RGB(0x55, 0xFF, 0xFF),
    RGB(0xFF, 0x55, 0x55), RGB(0xFF, 0x55, 0xFF), RGB(0xFF, 0xFF, 0x55), RGB(0xFF, 0xFF, 0xFF)
};

// Unused at the moment
static CONST COLORREF mtext_palette[64] =
{
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00),
    RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA),
    RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00),
    RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
    RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),

    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00),
    RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA),
    RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00),
    RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
    RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF)
};

// Unused at the moment
static CONST COLORREF mtext_s3_palette[64] =
{
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00),
    RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA),
    RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA),
    RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA),
    RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA),
    RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
    RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),

    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00),
    RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA),
    RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA),
    RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA),
    RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA), RGB(0xAA, 0xAA, 0xAA),
    RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF),
    RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF), RGB(0xFF, 0xFF, 0xFF)
};

// Unused at the moment
static CONST COLORREF CgaPalette[16] =
{
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0xAA), RGB(0x00, 0xAA, 0x00), RGB(0x00, 0xAA, 0xAA),
    RGB(0xAA, 0x00, 0x00), RGB(0xAA, 0x00, 0xAA), RGB(0xAA, 0x55, 0x00), RGB(0xAA, 0xAA, 0xAA),
    RGB(0x55, 0x55, 0x55), RGB(0x55, 0x55, 0xFF), RGB(0x55, 0xFF, 0x55), RGB(0x55, 0xFF, 0xFF),
    RGB(0xFF, 0x55, 0x55), RGB(0xFF, 0x55, 0xFF), RGB(0xFF, 0xFF, 0x55), RGB(0xFF, 0xFF, 0xFF)
};

// Unused at the moment
static CONST COLORREF CgaPalette2[VGA_MAX_COLORS / 4] =
{
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0xAA), RGB(0x00, 0xAA, 0x00), RGB(0x00, 0xAA, 0xAA),
    RGB(0xAA, 0x00, 0x00), RGB(0xAA, 0x00, 0xAA), RGB(0xAA, 0x55, 0x00), RGB(0xAA, 0xAA, 0xAA),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0xAA), RGB(0x00, 0xAA, 0x00), RGB(0x00, 0xAA, 0xAA),
    RGB(0xAA, 0x00, 0x00), RGB(0xAA, 0x00, 0xAA), RGB(0xAA, 0x55, 0x00), RGB(0xAA, 0xAA, 0xAA),
    RGB(0x55, 0x55, 0x55), RGB(0x55, 0x55, 0xFF), RGB(0x55, 0xFF, 0x55), RGB(0x55, 0xFF, 0xFF),
    RGB(0xFF, 0x55, 0x55), RGB(0xFF, 0x55, 0xFF), RGB(0xFF, 0xFF, 0x55), RGB(0xFF, 0xFF, 0xFF),
    RGB(0x55, 0x55, 0x55), RGB(0x55, 0x55, 0xFF), RGB(0x55, 0xFF, 0x55), RGB(0x55, 0xFF, 0xFF),
    RGB(0xFF, 0x55, 0x55), RGB(0xFF, 0x55, 0xFF), RGB(0xFF, 0xFF, 0x55), RGB(0xFF, 0xFF, 0xFF),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0xAA), RGB(0x00, 0xAA, 0x00), RGB(0x00, 0xAA, 0xAA),
    RGB(0xAA, 0x00, 0x00), RGB(0xAA, 0x00, 0xAA), RGB(0xAA, 0x55, 0x00), RGB(0xAA, 0xAA, 0xAA),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0xAA), RGB(0x00, 0xAA, 0x00), RGB(0x00, 0xAA, 0xAA),
    RGB(0xAA, 0x00, 0x00), RGB(0xAA, 0x00, 0xAA), RGB(0xAA, 0x55, 0x00), RGB(0xAA, 0xAA, 0xAA),
    RGB(0x55, 0x55, 0x55), RGB(0x55, 0x55, 0xFF), RGB(0x55, 0xFF, 0x55), RGB(0x55, 0xFF, 0xFF),
    RGB(0xFF, 0x55, 0x55), RGB(0xFF, 0x55, 0xFF), RGB(0xFF, 0xFF, 0x55), RGB(0xFF, 0xFF, 0xFF),
    RGB(0x55, 0x55, 0x55), RGB(0x55, 0x55, 0xFF), RGB(0x55, 0xFF, 0x55), RGB(0x55, 0xFF, 0xFF),
    RGB(0xFF, 0x55, 0x55), RGB(0xFF, 0x55, 0xFF), RGB(0xFF, 0xFF, 0x55), RGB(0xFF, 0xFF, 0xFF)
};

static CONST COLORREF EgaPalette___16ColorFixed_DOSBox[VGA_MAX_COLORS / 4] =
{
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0xAA), RGB(0x00, 0xAA, 0x00), RGB(0x00, 0xAA, 0xAA),
    RGB(0xAA, 0x00, 0x00), RGB(0xAA, 0x00, 0xAA), RGB(0xAA, 0x55, 0x00), RGB(0xAA, 0xAA, 0xAA),

    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0xAA), RGB(0x00, 0xAA, 0x00), RGB(0x00, 0xAA, 0xAA),
    RGB(0xAA, 0x00, 0x00), RGB(0xAA, 0x00, 0xAA), RGB(0xAA, 0x55, 0x00), RGB(0xAA, 0xAA, 0xAA),


    RGB(0x55, 0x55, 0x55), RGB(0x55, 0x55, 0xFF), RGB(0x55, 0xFF, 0x55), RGB(0x55, 0xFF, 0xFF),
    RGB(0xFF, 0x55, 0x55), RGB(0xFF, 0x55, 0xFF), RGB(0xFF, 0xFF, 0x55), RGB(0xFF, 0xFF, 0xFF),

    RGB(0x55, 0x55, 0x55), RGB(0x55, 0x55, 0xFF), RGB(0x55, 0xFF, 0x55), RGB(0x55, 0xFF, 0xFF),
    RGB(0xFF, 0x55, 0x55), RGB(0xFF, 0x55, 0xFF), RGB(0xFF, 0xFF, 0x55), RGB(0xFF, 0xFF, 0xFF),



    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0xAA), RGB(0x00, 0xAA, 0x00), RGB(0x00, 0xAA, 0xAA),
    RGB(0xAA, 0x00, 0x00), RGB(0xAA, 0x00, 0xAA), RGB(0xAA, 0x55, 0x00), RGB(0xAA, 0xAA, 0xAA),

    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0xAA), RGB(0x00, 0xAA, 0x00), RGB(0x00, 0xAA, 0xAA),
    RGB(0xAA, 0x00, 0x00), RGB(0xAA, 0x00, 0xAA), RGB(0xAA, 0x55, 0x00), RGB(0xAA, 0xAA, 0xAA),


    RGB(0x55, 0x55, 0x55), RGB(0x55, 0x55, 0xFF), RGB(0x55, 0xFF, 0x55), RGB(0x55, 0xFF, 0xFF),
    RGB(0xFF, 0x55, 0x55), RGB(0xFF, 0x55, 0xFF), RGB(0xFF, 0xFF, 0x55), RGB(0xFF, 0xFF, 0xFF),

    RGB(0x55, 0x55, 0x55), RGB(0x55, 0x55, 0xFF), RGB(0x55, 0xFF, 0x55), RGB(0x55, 0xFF, 0xFF),
    RGB(0xFF, 0x55, 0x55), RGB(0xFF, 0x55, 0xFF), RGB(0xFF, 0xFF, 0x55), RGB(0xFF, 0xFF, 0xFF)
};

// This is the same as TextPalette
static CONST COLORREF EgaPalette__HiRes[VGA_MAX_COLORS / 4] =
{
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0xAA), RGB(0x00, 0xAA, 0x00), RGB(0x00, 0xAA, 0xAA),
    RGB(0xAA, 0x00, 0x00), RGB(0xAA, 0x00, 0xAA), RGB(0xAA, 0xAA, 0x00), RGB(0xAA, 0xAA, 0xAA),
    RGB(0x00, 0x00, 0x55), RGB(0x00, 0x00, 0xFF), RGB(0x00, 0xAA, 0x55), RGB(0x00, 0xAA, 0xFF),
    RGB(0xAA, 0x00, 0x55), RGB(0xAA, 0x00, 0xFF), RGB(0xAA, 0xAA, 0x55), RGB(0xAA, 0xAA, 0xFF),

    RGB(0x00, 0x55, 0x00), RGB(0x00, 0x55, 0xAA), RGB(0x00, 0xFF, 0x00), RGB(0x00, 0xFF, 0xAA),
    RGB(0xAA, 0x55, 0x00), RGB(0xAA, 0x55, 0xAA), RGB(0xAA, 0xFF, 0x00), RGB(0xAA, 0xFF, 0xAA),
    RGB(0x00, 0x55, 0x55), RGB(0x00, 0x55, 0xFF), RGB(0x00, 0xFF, 0x55), RGB(0x00, 0xFF, 0xFF),
    RGB(0xAA, 0x55, 0x55), RGB(0xAA, 0x55, 0xFF), RGB(0xAA, 0xFF, 0x55), RGB(0xAA, 0xFF, 0xFF),


    RGB(0x55, 0x00, 0x00), RGB(0x55, 0x00, 0xAA), RGB(0x55, 0xAA, 0x00), RGB(0x55, 0xAA, 0xAA),
    RGB(0xFF, 0x00, 0x00), RGB(0xFF, 0x00, 0xAA), RGB(0xFF, 0xAA, 0x00), RGB(0xFF, 0xAA, 0xAA),
    RGB(0x55, 0x00, 0x55), RGB(0x55, 0x00, 0xFF), RGB(0x55, 0xAA, 0x55), RGB(0x55, 0xAA, 0xFF),
    RGB(0xFF, 0x00, 0x55), RGB(0xFF, 0x00, 0xFF), RGB(0xFF, 0xAA, 0x55), RGB(0xFF, 0xAA, 0xFF),

    RGB(0x55, 0x55, 0x00), RGB(0x55, 0x55, 0xAA), RGB(0x55, 0xFF, 0x00), RGB(0x55, 0xFF, 0xAA),
    RGB(0xFF, 0x55, 0x00), RGB(0xFF, 0x55, 0xAA), RGB(0xFF, 0xFF, 0x00), RGB(0xFF, 0xFF, 0xAA),
    RGB(0x55, 0x55, 0x55), RGB(0x55, 0x55, 0xFF), RGB(0x55, 0xFF, 0x55), RGB(0x55, 0xFF, 0xFF),
    RGB(0xFF, 0x55, 0x55), RGB(0xFF, 0x55, 0xFF), RGB(0xFF, 0xFF, 0x55), RGB(0xFF, 0xFF, 0xFF)
};

#define USE_REACTOS_COLORS
// #define USE_DOSBOX_COLORS

/*
 * Same palette as the default one 'VgaDefaultPalette' in vga.c
 */
#if defined(USE_REACTOS_COLORS)

// ReactOS colors
static CONST COLORREF VgaPalette[VGA_MAX_COLORS] =
{
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0xAA), RGB(0x00, 0xAA, 0x00), RGB(0x00, 0xAA, 0xAA),
    RGB(0xAA, 0x00, 0x00), RGB(0xAA, 0x00, 0xAA), RGB(0xAA, 0x55, 0x00), RGB(0xAA, 0xAA, 0xAA),
    RGB(0x55, 0x55, 0x55), RGB(0x55, 0x55, 0xFF), RGB(0x55, 0xFF, 0x55), RGB(0x55, 0xFF, 0xFF),
    RGB(0xFF, 0x55, 0x55), RGB(0xFF, 0x55, 0xFF), RGB(0xFF, 0xFF, 0x55), RGB(0xFF, 0xFF, 0xFF),
    RGB(0x00, 0x00, 0x00), RGB(0x10, 0x10, 0x10), RGB(0x20, 0x20, 0x20), RGB(0x35, 0x35, 0x35),
    RGB(0x45, 0x45, 0x45), RGB(0x55, 0x55, 0x55), RGB(0x65, 0x65, 0x65), RGB(0x75, 0x75, 0x75),
    RGB(0x8A, 0x8A, 0x8A), RGB(0x9A, 0x9A, 0x9A), RGB(0xAA, 0xAA, 0xAA), RGB(0xBA, 0xBA, 0xBA),
    RGB(0xCA, 0xCA, 0xCA), RGB(0xDF, 0xDF, 0xDF), RGB(0xEF, 0xEF, 0xEF), RGB(0xFF, 0xFF, 0xFF),
    RGB(0x00, 0x00, 0xFF), RGB(0x41, 0x00, 0xFF), RGB(0x82, 0x00, 0xFF), RGB(0xBE, 0x00, 0xFF),
    RGB(0xFF, 0x00, 0xFF), RGB(0xFF, 0x00, 0xBE), RGB(0xFF, 0x00, 0x82), RGB(0xFF, 0x00, 0x41),
    RGB(0xFF, 0x00, 0x00), RGB(0xFF, 0x41, 0x00), RGB(0xFF, 0x82, 0x00), RGB(0xFF, 0xBE, 0x00),
    RGB(0xFF, 0xFF, 0x00), RGB(0xBE, 0xFF, 0x00), RGB(0x82, 0xFF, 0x00), RGB(0x41, 0xFF, 0x00),
    RGB(0x00, 0xFF, 0x00), RGB(0x00, 0xFF, 0x41), RGB(0x00, 0xFF, 0x82), RGB(0x00, 0xFF, 0xBE),
    RGB(0x00, 0xFF, 0xFF), RGB(0x00, 0xBE, 0xFF), RGB(0x00, 0x82, 0xFF), RGB(0x00, 0x41, 0xFF),
    RGB(0x82, 0x82, 0xFF), RGB(0x9E, 0x82, 0xFF), RGB(0xBE, 0x82, 0xFF), RGB(0xDF, 0x82, 0xFF),
    RGB(0xFF, 0x82, 0xFF), RGB(0xFF, 0x82, 0xDF), RGB(0xFF, 0x82, 0xBE), RGB(0xFF, 0x82, 0x9E),
    RGB(0xFF, 0x82, 0x82), RGB(0xFF, 0x9E, 0x82), RGB(0xFF, 0xBE, 0x82), RGB(0xFF, 0xDF, 0x82),
    RGB(0xFF, 0xFF, 0x82), RGB(0xDF, 0xFF, 0x82), RGB(0xBE, 0xFF, 0x82), RGB(0x9E, 0xFF, 0x82),
    RGB(0x82, 0xFF, 0x82), RGB(0x82, 0xFF, 0x9E), RGB(0x82, 0xFF, 0xBE), RGB(0x82, 0xFF, 0xDF),
    RGB(0x82, 0xFF, 0xFF), RGB(0x82, 0xDF, 0xFF), RGB(0x82, 0xBE, 0xFF), RGB(0x82, 0x9E, 0xFF),
    RGB(0xBA, 0xBA, 0xFF), RGB(0xCA, 0xBA, 0xFF), RGB(0xDF, 0xBA, 0xFF), RGB(0xEF, 0xBA, 0xFF),
    RGB(0xFF, 0xBA, 0xFF), RGB(0xFF, 0xBA, 0xEF), RGB(0xFF, 0xBA, 0xDF), RGB(0xFF, 0xBA, 0xCA),
    RGB(0xFF, 0xBA, 0xBA), RGB(0xFF, 0xCA, 0xBA), RGB(0xFF, 0xDF, 0xBA), RGB(0xFF, 0xEF, 0xBA),
    RGB(0xFF, 0xFF, 0xBA), RGB(0xEF, 0xFF, 0xBA), RGB(0xDF, 0xFF, 0xBA), RGB(0xCA, 0xFF, 0xBA),
    RGB(0xBA, 0xFF, 0xBA), RGB(0xBA, 0xFF, 0xCA), RGB(0xBA, 0xFF, 0xDF), RGB(0xBA, 0xFF, 0xEF),
    RGB(0xBA, 0xFF, 0xFF), RGB(0xBA, 0xEF, 0xFF), RGB(0xBA, 0xDF, 0xFF), RGB(0xBA, 0xCA, 0xFF),
    RGB(0x00, 0x00, 0x71), RGB(0x1C, 0x00, 0x71), RGB(0x39, 0x00, 0x71), RGB(0x55, 0x00, 0x71),
    RGB(0x71, 0x00, 0x71), RGB(0x71, 0x00, 0x55), RGB(0x71, 0x00, 0x39), RGB(0x71, 0x00, 0x1C),
    RGB(0x71, 0x00, 0x00), RGB(0x71, 0x1C, 0x00), RGB(0x71, 0x39, 0x00), RGB(0x71, 0x55, 0x00),
    RGB(0x71, 0x71, 0x00), RGB(0x55, 0x71, 0x00), RGB(0x39, 0x71, 0x00), RGB(0x1C, 0x71, 0x00),
    RGB(0x00, 0x71, 0x00), RGB(0x00, 0x71, 0x1C), RGB(0x00, 0x71, 0x39), RGB(0x00, 0x71, 0x55),
    RGB(0x00, 0x71, 0x71), RGB(0x00, 0x55, 0x71), RGB(0x00, 0x39, 0x71), RGB(0x00, 0x1C, 0x71),
    RGB(0x39, 0x39, 0x71), RGB(0x45, 0x39, 0x71), RGB(0x55, 0x39, 0x71), RGB(0x61, 0x39, 0x71),
    RGB(0x71, 0x39, 0x71), RGB(0x71, 0x39, 0x61), RGB(0x71, 0x39, 0x55), RGB(0x71, 0x39, 0x45),
    RGB(0x71, 0x39, 0x39), RGB(0x71, 0x45, 0x39), RGB(0x71, 0x55, 0x39), RGB(0x71, 0x61, 0x39),
    RGB(0x71, 0x71, 0x39), RGB(0x61, 0x71, 0x39), RGB(0x55, 0x71, 0x39), RGB(0x45, 0x71, 0x39),
    RGB(0x39, 0x71, 0x39), RGB(0x39, 0x71, 0x45), RGB(0x39, 0x71, 0x55), RGB(0x39, 0x71, 0x61),
    RGB(0x39, 0x71, 0x71), RGB(0x39, 0x61, 0x71), RGB(0x39, 0x55, 0x71), RGB(0x39, 0x45, 0x71),
    RGB(0x51, 0x51, 0x71), RGB(0x59, 0x51, 0x71), RGB(0x61, 0x51, 0x71), RGB(0x69, 0x51, 0x71),
    RGB(0x71, 0x51, 0x71), RGB(0x71, 0x51, 0x69), RGB(0x71, 0x51, 0x61), RGB(0x71, 0x51, 0x59),
    RGB(0x71, 0x51, 0x51), RGB(0x71, 0x59, 0x51), RGB(0x71, 0x61, 0x51), RGB(0x71, 0x69, 0x51),
    RGB(0x71, 0x71, 0x51), RGB(0x69, 0x71, 0x51), RGB(0x61, 0x71, 0x51), RGB(0x59, 0x71, 0x51),
    RGB(0x51, 0x71, 0x51), RGB(0x51, 0x71, 0x59), RGB(0x51, 0x71, 0x61), RGB(0x51, 0x71, 0x69),
    RGB(0x51, 0x71, 0x71), RGB(0x51, 0x69, 0x71), RGB(0x51, 0x61, 0x71), RGB(0x51, 0x59, 0x71),
    RGB(0x00, 0x00, 0x41), RGB(0x10, 0x00, 0x41), RGB(0x20, 0x00, 0x41), RGB(0x31, 0x00, 0x41),
    RGB(0x41, 0x00, 0x41), RGB(0x41, 0x00, 0x31), RGB(0x41, 0x00, 0x20), RGB(0x41, 0x00, 0x10),
    RGB(0x41, 0x00, 0x00), RGB(0x41, 0x10, 0x00), RGB(0x41, 0x20, 0x00), RGB(0x41, 0x31, 0x00),
    RGB(0x41, 0x41, 0x00), RGB(0x31, 0x41, 0x00), RGB(0x20, 0x41, 0x00), RGB(0x10, 0x41, 0x00),
    RGB(0x00, 0x41, 0x00), RGB(0x00, 0x41, 0x10), RGB(0x00, 0x41, 0x20), RGB(0x00, 0x41, 0x31),
    RGB(0x00, 0x41, 0x41), RGB(0x00, 0x31, 0x41), RGB(0x00, 0x20, 0x41), RGB(0x00, 0x10, 0x41),
    RGB(0x20, 0x20, 0x41), RGB(0x28, 0x20, 0x41), RGB(0x31, 0x20, 0x41), RGB(0x39, 0x20, 0x41),
    RGB(0x41, 0x20, 0x41), RGB(0x41, 0x20, 0x39), RGB(0x41, 0x20, 0x31), RGB(0x41, 0x20, 0x28),
    RGB(0x41, 0x20, 0x20), RGB(0x41, 0x28, 0x20), RGB(0x41, 0x31, 0x20), RGB(0x41, 0x39, 0x20),
    RGB(0x41, 0x41, 0x20), RGB(0x39, 0x41, 0x20), RGB(0x31, 0x41, 0x20), RGB(0x28, 0x41, 0x20),
    RGB(0x20, 0x41, 0x20), RGB(0x20, 0x41, 0x28), RGB(0x20, 0x41, 0x31), RGB(0x20, 0x41, 0x39),
    RGB(0x20, 0x41, 0x41), RGB(0x20, 0x39, 0x41), RGB(0x20, 0x31, 0x41), RGB(0x20, 0x28, 0x41),
    RGB(0x2D, 0x2D, 0x41), RGB(0x31, 0x2D, 0x41), RGB(0x35, 0x2D, 0x41), RGB(0x3D, 0x2D, 0x41),
    RGB(0x41, 0x2D, 0x41), RGB(0x41, 0x2D, 0x3D), RGB(0x41, 0x2D, 0x35), RGB(0x41, 0x2D, 0x31),
    RGB(0x41, 0x2D, 0x2D), RGB(0x41, 0x31, 0x2D), RGB(0x41, 0x35, 0x2D), RGB(0x41, 0x3D, 0x2D),
    RGB(0x41, 0x41, 0x2D), RGB(0x3D, 0x41, 0x2D), RGB(0x35, 0x41, 0x2D), RGB(0x31, 0x41, 0x2D),
    RGB(0x2D, 0x41, 0x2D), RGB(0x2D, 0x41, 0x31), RGB(0x2D, 0x41, 0x35), RGB(0x2D, 0x41, 0x3D),
    RGB(0x2D, 0x41, 0x41), RGB(0x2D, 0x3D, 0x41), RGB(0x2D, 0x35, 0x41), RGB(0x2D, 0x31, 0x41),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00)
};

#elif defined(USE_DOSBOX_COLORS)

// DOSBox colors
static CONST COLORREF VgaPalette[VGA_MAX_COLORS] =
{
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0xAA), RGB(0x00, 0xAA, 0x00), RGB(0x00, 0xAA, 0xAA),
    RGB(0xAA, 0x00, 0x00), RGB(0xAA, 0x00, 0xAA), RGB(0xAA, 0x55, 0x00), RGB(0xAA, 0xAA, 0xAA),
    RGB(0x55, 0x55, 0x55), RGB(0x55, 0x55, 0xFF), RGB(0x55, 0xFF, 0x55), RGB(0x55, 0xFF, 0xFF),
    RGB(0xFF, 0x55, 0x55), RGB(0xFF, 0x55, 0xFF), RGB(0xFF, 0xFF, 0x55), RGB(0xFF, 0xFF, 0xFF),
    RGB(0x00, 0x00, 0x00), RGB(0x14, 0x14, 0x14), RGB(0x20, 0x20, 0x20), RGB(0x2C, 0x2C, 0x2C),
    RGB(0x38, 0x38, 0x38), RGB(0x45, 0x45, 0x45), RGB(0x51, 0x51, 0x51), RGB(0x61, 0x61, 0x61),
    RGB(0x71, 0x71, 0x71), RGB(0x82, 0x82, 0x82), RGB(0x92, 0x92, 0x92), RGB(0xA2, 0xA2, 0xA2),
    RGB(0xB6, 0xB6, 0xB6), RGB(0xCB, 0xCB, 0xCB), RGB(0xE3, 0xE3, 0xE3), RGB(0xFF, 0xFF, 0xFF),
    RGB(0x00, 0x00, 0xFF), RGB(0x41, 0x00, 0xFF), RGB(0x7D, 0x00, 0xFF), RGB(0xBE, 0x00, 0xFF),
    RGB(0xFF, 0x00, 0xFF), RGB(0xFF, 0x00, 0xBE), RGB(0xFF, 0x00, 0x7D), RGB(0xFF, 0x00, 0x41),
    RGB(0xFF, 0x00, 0x00), RGB(0xFF, 0x41, 0x00), RGB(0xFF, 0x7D, 0x00), RGB(0xFF, 0xBE, 0x00),
    RGB(0xFF, 0xFF, 0x00), RGB(0xBE, 0xFF, 0x00), RGB(0x7D, 0xFF, 0x00), RGB(0x41, 0xFF, 0x00),
    RGB(0x00, 0xFF, 0x00), RGB(0x00, 0xFF, 0x41), RGB(0x00, 0xFF, 0x7D), RGB(0x00, 0xFF, 0xBE),
    RGB(0x00, 0xFF, 0xFF), RGB(0x00, 0xBE, 0xFF), RGB(0x00, 0x7D, 0xFF), RGB(0x00, 0x41, 0xFF),
    RGB(0x7D, 0x7D, 0xFF), RGB(0x9E, 0x7D, 0xFF), RGB(0xBE, 0x7D, 0xFF), RGB(0xDF, 0x7D, 0xFF),
    RGB(0xFF, 0x7D, 0xFF), RGB(0xFF, 0x7D, 0xDF), RGB(0xFF, 0x7D, 0xBE), RGB(0xFF, 0x7D, 0x9E),

    RGB(0xFF, 0x7D, 0x7D), RGB(0xFF, 0x9E, 0x7D), RGB(0xFF, 0xBE, 0x7D), RGB(0xFF, 0xDF, 0x7D),
    RGB(0xFF, 0xFF, 0x7D), RGB(0xDF, 0xFF, 0x7D), RGB(0xBE, 0xFF, 0x7D), RGB(0x9E, 0xFF, 0x7D),
    RGB(0x7D, 0xFF, 0x7D), RGB(0x7D, 0xFF, 0x9E), RGB(0x7D, 0xFF, 0xBE), RGB(0x7D, 0xFF, 0xDF),
    RGB(0x7D, 0xFF, 0xFF), RGB(0x7D, 0xDF, 0xFF), RGB(0x7D, 0xBE, 0xFF), RGB(0x7D, 0x9E, 0xFF),
    RGB(0xB6, 0xB6, 0xFF), RGB(0xC7, 0xB6, 0xFF), RGB(0xDB, 0xB6, 0xFF), RGB(0xEB, 0xB6, 0xFF),
    RGB(0xFF, 0xB6, 0xFF), RGB(0xFF, 0xB6, 0xEB), RGB(0xFF, 0xB6, 0xDB), RGB(0xFF, 0xB6, 0xC7),
    RGB(0xFF, 0xB6, 0xB6), RGB(0xFF, 0xC7, 0xB6), RGB(0xFF, 0xDB, 0xB6), RGB(0xFF, 0xEB, 0xB6),
    RGB(0xFF, 0xFF, 0xB6), RGB(0xEB, 0xFF, 0xB6), RGB(0xDB, 0xFF, 0xB6), RGB(0xC7, 0xFF, 0xB6),
    RGB(0xB6, 0xFF, 0xB6), RGB(0xB6, 0xFF, 0xC7), RGB(0xB6, 0xFF, 0xDB), RGB(0xB6, 0xFF, 0xEB),
    RGB(0xB6, 0xFF, 0xFF), RGB(0xB6, 0xEB, 0xFF), RGB(0xB6, 0xDB, 0xFF), RGB(0xB6, 0xC7, 0xFF),
    RGB(0x00, 0x00, 0x71), RGB(0x1C, 0x00, 0x71), RGB(0x38, 0x00, 0x71), RGB(0x55, 0x00, 0x71),
    RGB(0x71, 0x00, 0x71), RGB(0x71, 0x00, 0x55), RGB(0x71, 0x00, 0x38), RGB(0x71, 0x00, 0x1C),
    RGB(0x71, 0x00, 0x00), RGB(0x71, 0x1C, 0x00), RGB(0x71, 0x38, 0x00), RGB(0x71, 0x55, 0x00),
    RGB(0x71, 0x71, 0x00), RGB(0x55, 0x71, 0x00), RGB(0x38, 0x71, 0x00), RGB(0x1C, 0x71, 0x00),
    RGB(0x00, 0x71, 0x00), RGB(0x00, 0x71, 0x1C), RGB(0x00, 0x71, 0x38), RGB(0x00, 0x71, 0x55),
    RGB(0x00, 0x71, 0x71), RGB(0x00, 0x55, 0x71), RGB(0x00, 0x38, 0x71), RGB(0x00, 0x1C, 0x71),

    RGB(0x38, 0x38, 0x71), RGB(0x45, 0x38, 0x71), RGB(0x55, 0x38, 0x71), RGB(0x61, 0x38, 0x71),
    RGB(0x71, 0x38, 0x71), RGB(0x71, 0x38, 0x61), RGB(0x71, 0x38, 0x55), RGB(0x71, 0x38, 0x45),
    RGB(0x71, 0x38, 0x38), RGB(0x71, 0x45, 0x38), RGB(0x71, 0x55, 0x38), RGB(0x71, 0x61, 0x38),
    RGB(0x71, 0x71, 0x38), RGB(0x61, 0x71, 0x38), RGB(0x55, 0x71, 0x38), RGB(0x45, 0x71, 0x38),
    RGB(0x38, 0x71, 0x38), RGB(0x38, 0x71, 0x45), RGB(0x38, 0x71, 0x55), RGB(0x38, 0x71, 0x61),
    RGB(0x38, 0x71, 0x71), RGB(0x38, 0x61, 0x71), RGB(0x38, 0x55, 0x71), RGB(0x38, 0x45, 0x71),
    RGB(0x51, 0x51, 0x71), RGB(0x59, 0x51, 0x71), RGB(0x61, 0x51, 0x71), RGB(0x69, 0x51, 0x71),
    RGB(0x71, 0x51, 0x71), RGB(0x71, 0x51, 0x69), RGB(0x71, 0x51, 0x61), RGB(0x71, 0x51, 0x59),
    RGB(0x71, 0x51, 0x51), RGB(0x71, 0x59, 0x51), RGB(0x71, 0x61, 0x51), RGB(0x71, 0x69, 0x51),
    RGB(0x71, 0x71, 0x51), RGB(0x69, 0x71, 0x51), RGB(0x61, 0x71, 0x51), RGB(0x59, 0x71, 0x51),
    RGB(0x51, 0x71, 0x51), RGB(0x51, 0x71, 0x59), RGB(0x51, 0x71, 0x61), RGB(0x51, 0x71, 0x69),
    RGB(0x51, 0x71, 0x71), RGB(0x51, 0x69, 0x71), RGB(0x51, 0x61, 0x71), RGB(0x51, 0x59, 0x71),
    RGB(0x00, 0x00, 0x41), RGB(0x10, 0x00, 0x41), RGB(0x20, 0x00, 0x41), RGB(0x30, 0x00, 0x41),
    RGB(0x41, 0x00, 0x41), RGB(0x41, 0x00, 0x30), RGB(0x41, 0x00, 0x20), RGB(0x41, 0x00, 0x10),
    RGB(0x41, 0x00, 0x00), RGB(0x41, 0x10, 0x00), RGB(0x41, 0x20, 0x00), RGB(0x41, 0x30, 0x00),
    RGB(0x41, 0x41, 0x00), RGB(0x30, 0x41, 0x00), RGB(0x20, 0x41, 0x00), RGB(0x10, 0x41, 0x00),

    RGB(0x00, 0x41, 0x00), RGB(0x00, 0x41, 0x10), RGB(0x00, 0x41, 0x20), RGB(0x00, 0x41, 0x30),
    RGB(0x00, 0x41, 0x41), RGB(0x00, 0x30, 0x41), RGB(0x00, 0x20, 0x41), RGB(0x00, 0x10, 0x41),
    RGB(0x20, 0x20, 0x41), RGB(0x28, 0x20, 0x41), RGB(0x30, 0x20, 0x41), RGB(0x38, 0x20, 0x41),
    RGB(0x41, 0x20, 0x41), RGB(0x41, 0x20, 0x38), RGB(0x41, 0x20, 0x30), RGB(0x41, 0x20, 0x28),
    RGB(0x41, 0x20, 0x20), RGB(0x41, 0x28, 0x20), RGB(0x41, 0x30, 0x20), RGB(0x41, 0x38, 0x20),
    RGB(0x41, 0x41, 0x20), RGB(0x38, 0x41, 0x20), RGB(0x30, 0x41, 0x20), RGB(0x28, 0x41, 0x20),
    RGB(0x20, 0x41, 0x20), RGB(0x20, 0x41, 0x28), RGB(0x20, 0x41, 0x30), RGB(0x20, 0x41, 0x38),
    RGB(0x20, 0x41, 0x41), RGB(0x20, 0x38, 0x41), RGB(0x20, 0x30, 0x41), RGB(0x20, 0x28, 0x41),
    RGB(0x2C, 0x2C, 0x41), RGB(0x30, 0x2C, 0x41), RGB(0x34, 0x2C, 0x41), RGB(0x3C, 0x2C, 0x41),
    RGB(0x41, 0x2C, 0x41), RGB(0x41, 0x2C, 0x3C), RGB(0x41, 0x2C, 0x34), RGB(0x41, 0x2C, 0x30),
    RGB(0x41, 0x2C, 0x2C), RGB(0x41, 0x30, 0x2C), RGB(0x41, 0x34, 0x2C), RGB(0x41, 0x3C, 0x2C),
    RGB(0x41, 0x41, 0x2C), RGB(0x3C, 0x41, 0x2C), RGB(0x34, 0x41, 0x2C), RGB(0x30, 0x41, 0x2C),
    RGB(0x2C, 0x41, 0x2C), RGB(0x2C, 0x41, 0x30), RGB(0x2C, 0x41, 0x34), RGB(0x2C, 0x41, 0x3C),
    RGB(0x2C, 0x41, 0x41), RGB(0x2C, 0x3C, 0x41), RGB(0x2C, 0x34, 0x41), RGB(0x2C, 0x30, 0x41),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00),
    RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00), RGB(0x00, 0x00, 0x00)
};

#endif

/* PRIVATE FUNCTIONS **********************************************************/

static BOOLEAN BiosKbdBufferPush(WORD Data)
{
    /* Get the location of the element after the tail */
    WORD NextElement = Bda->KeybdBufferTail + sizeof(WORD);

    /* Wrap it around if it's at or beyond the end */
    if (NextElement >= Bda->KeybdBufferEnd) NextElement = Bda->KeybdBufferStart;

    /* If it's full, fail */
    if (NextElement == Bda->KeybdBufferHead) return FALSE;

    /* Put the value in the queue */
    *((LPWORD)((ULONG_PTR)Bda + Bda->KeybdBufferTail)) = Data;
    Bda->KeybdBufferTail += sizeof(WORD);

    /* Check if we are at, or have passed, the end of the buffer */
    if (Bda->KeybdBufferTail >= Bda->KeybdBufferEnd)
    {
        /* Return it to the beginning */
        Bda->KeybdBufferTail = Bda->KeybdBufferStart;
    }

    /* Return success */
    return TRUE;
}

static BOOLEAN BiosKbdBufferTop(LPWORD Data)
{
    /* If it's empty, fail */
    if (Bda->KeybdBufferHead == Bda->KeybdBufferTail) return FALSE;

    /* Otherwise, get the value and return success */
    *Data = *((LPWORD)((ULONG_PTR)Bda + Bda->KeybdBufferHead));

    return TRUE;
}

static BOOLEAN BiosKbdBufferPop(VOID)
{
    /* If it's empty, fail */
    if (Bda->KeybdBufferHead == Bda->KeybdBufferTail) return FALSE;

    /* Remove the value from the queue */
    Bda->KeybdBufferHead += sizeof(WORD);

    /* Check if we are at, or have passed, the end of the buffer */
    if (Bda->KeybdBufferHead >= Bda->KeybdBufferEnd)
    {
        /* Return it to the beginning */
        Bda->KeybdBufferHead = Bda->KeybdBufferStart;
    }

    /* Return success */
    return TRUE;
}

static VOID BiosReadWindow(LPWORD Buffer, SMALL_RECT Rectangle, BYTE Page)
{
    INT i, j;
    INT Counter = 0;
    WORD Character;
    DWORD VideoAddress = TO_LINEAR(TEXT_VIDEO_SEG, Page * Bda->VideoPageSize);

    for (i = Rectangle.Top; i <= Rectangle.Bottom; i++)
    {
        for (j = Rectangle.Left; j <= Rectangle.Right; j++)
        {
            /* Read from video memory */
            VgaReadMemory(VideoAddress + (i * Bda->ScreenColumns + j) * sizeof(WORD),
                          (LPVOID)&Character,
                          sizeof(WORD));

            /* Write the data to the buffer in row order */
            Buffer[Counter++] = Character;
        }
    }
}

static VOID BiosWriteWindow(LPWORD Buffer, SMALL_RECT Rectangle, BYTE Page)
{
    INT i, j;
    INT Counter = 0;
    WORD Character;
    DWORD VideoAddress = TO_LINEAR(TEXT_VIDEO_SEG, Page * Bda->VideoPageSize);

    for (i = Rectangle.Top; i <= Rectangle.Bottom; i++)
    {
        for (j = Rectangle.Left; j <= Rectangle.Right; j++)
        {
            Character = Buffer[Counter++];

            /* Write to video memory */
            VgaWriteMemory(VideoAddress + (i * Bda->ScreenColumns + j) * sizeof(WORD),
                           (LPVOID)&Character,
                           sizeof(WORD));
        }
    }
}

static BOOLEAN BiosScrollWindow(INT Direction,
                                DWORD Amount,
                                SMALL_RECT Rectangle,
                                BYTE Page,
                                BYTE FillAttribute)
{
    DWORD i;
    LPWORD WindowData;
    WORD WindowWidth = Rectangle.Right - Rectangle.Left + 1;
    WORD WindowHeight = Rectangle.Bottom - Rectangle.Top + 1;
    DWORD WindowSize = WindowWidth * WindowHeight;

    /* Allocate a buffer for the window */
    WindowData = (LPWORD)HeapAlloc(GetProcessHeap(),
                                   HEAP_ZERO_MEMORY,
                                   WindowSize * sizeof(WORD));
    if (WindowData == NULL) return FALSE;

    /* Read the window data */
    BiosReadWindow(WindowData, Rectangle, Page);

    if ((Amount == 0)
        || (((Direction == SCROLL_DIRECTION_UP)
        || (Direction == SCROLL_DIRECTION_DOWN))
        && (Amount >= WindowHeight))
        || (((Direction == SCROLL_DIRECTION_LEFT)
        || (Direction == SCROLL_DIRECTION_RIGHT))
        && (Amount >= WindowWidth)))
    {
        /* Fill the window */
        for (i = 0; i < WindowSize; i++)
        {
            WindowData[i] = MAKEWORD(' ', FillAttribute);
        }

        goto Done;
    }

    switch (Direction)
    {
        case SCROLL_DIRECTION_UP:
        {
            RtlMoveMemory(WindowData,
                          &WindowData[WindowWidth * Amount],
                          (WindowSize - WindowWidth * Amount) * sizeof(WORD));

            for (i = 0; i < Amount * WindowWidth; i++)
            {
                WindowData[WindowSize - i - 1] = MAKEWORD(' ', FillAttribute);
            }

            break;
        }

        case SCROLL_DIRECTION_DOWN:
        {
            RtlMoveMemory(&WindowData[WindowWidth * Amount],
                          WindowData,
                          (WindowSize - WindowWidth * Amount) * sizeof(WORD));

            for (i = 0; i < Amount * WindowWidth; i++)
            {
                WindowData[i] = MAKEWORD(' ', FillAttribute);
            }

            break;
        }

        default:
        {
            // TODO: NOT IMPLEMENTED!
            UNIMPLEMENTED;
        }
    }

Done:
    /* Write back the window data */
    BiosWriteWindow(WindowData, Rectangle, Page);

    /* Free the window buffer */
    HeapFree(GetProcessHeap(), 0, WindowData);

    return TRUE;
}

static VOID BiosCopyTextConsoleToVgaMemory(VOID)
{
    PCHAR_INFO CharBuffer;
    COORD BufferSize = {Bda->ScreenColumns, Bda->ScreenRows + 1};
    COORD Origin = { 0, 0 };
    SMALL_RECT ScreenRect;

    INT i, j;
    INT Counter = 0;
    WORD Character;
    DWORD VideoAddress = TO_LINEAR(TEXT_VIDEO_SEG, Bda->VideoPage * Bda->VideoPageSize);

    /* Allocate a temporary buffer for ReadConsoleOutput */
    CharBuffer = HeapAlloc(GetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           BufferSize.X * BufferSize.Y
                             * sizeof(CHAR_INFO));
    if (CharBuffer == NULL) return;

    ScreenRect.Left = ScreenRect.Top = 0;
    ScreenRect.Right  = BufferSize.X;
    ScreenRect.Bottom = BufferSize.Y;

    /* Read the data from the console into the temporary buffer... */
    ReadConsoleOutputA(BiosConsoleOutput,
                       CharBuffer,
                       BufferSize,
                       Origin,
                       &ScreenRect);

    /* ... and copy the temporary buffer into the VGA memory */
    for (i = 0; i < BufferSize.Y; i++)
    {
        for (j = 0; j < BufferSize.X; j++)
        {
            Character = MAKEWORD(CharBuffer[Counter].Char.AsciiChar,
                                 (BYTE)CharBuffer[Counter].Attributes);
            ++Counter;

            /* Write to video memory */
            VgaWriteMemory(VideoAddress + (i * Bda->ScreenColumns + j) * sizeof(WORD),
                           (LPVOID)&Character,
                           sizeof(WORD));
        }
    }
}

static BOOLEAN VgaSetRegisters(PVGA_REGISTERS Registers)
{
    INT i;

    if (Registers == NULL) return FALSE;

    /* Disable interrupts */
    setIF(0);

    /*
     * Set the CRT base address according to the selected mode,
     * monochrome or color. The following macros:
     * VGA_INSTAT1_READ, VGA_CRTC_INDEX and VGA_CRTC_DATA are then
     * used to access the correct VGA I/O ports.
     */
    Bda->CrtBasePort = (Registers->Misc & 0x01) ? VGA_CRTC_INDEX_COLOR
                                                : VGA_CRTC_INDEX_MONO;

    /* Write the misc register */
    VgaWritePort(VGA_MISC_WRITE, Registers->Misc);

    /* Synchronous reset on */
    VgaWritePort(VGA_SEQ_INDEX, VGA_SEQ_RESET_REG);
    VgaWritePort(VGA_SEQ_DATA , VGA_SEQ_RESET_AR);

    /* Write the sequencer registers */
    for (i = 1; i < VGA_SEQ_MAX_REG; i++)
    {
        VgaWritePort(VGA_SEQ_INDEX, i);
        VgaWritePort(VGA_SEQ_DATA, Registers->Sequencer[i]);
    }

    /* Synchronous reset off */
    VgaWritePort(VGA_SEQ_INDEX, VGA_SEQ_RESET_REG);
    VgaWritePort(VGA_SEQ_DATA , VGA_SEQ_RESET_SR | VGA_SEQ_RESET_AR);

    /* Unlock CRTC registers 0-7 */
    VgaWritePort(VGA_CRTC_INDEX, VGA_CRTC_END_HORZ_BLANKING_REG);
    VgaWritePort(VGA_CRTC_DATA, VgaReadPort(VGA_CRTC_DATA) | 0x80);
    VgaWritePort(VGA_CRTC_INDEX, VGA_CRTC_VERT_RETRACE_END_REG);
    VgaWritePort(VGA_CRTC_DATA, VgaReadPort(VGA_CRTC_DATA) & ~0x80);
    // Make sure they remain unlocked
    Registers->CRT[VGA_CRTC_END_HORZ_BLANKING_REG] |= 0x80;
    Registers->CRT[VGA_CRTC_VERT_RETRACE_END_REG] &= ~0x80;

    /* Write the CRTC registers */
    for (i = 0; i < VGA_CRTC_MAX_REG; i++)
    {
        VgaWritePort(VGA_CRTC_INDEX, i);
        VgaWritePort(VGA_CRTC_DATA, Registers->CRT[i]);
    }

    /* Write the GC registers */
    for (i = 0; i < VGA_GC_MAX_REG; i++)
    {
        VgaWritePort(VGA_GC_INDEX, i);
        VgaWritePort(VGA_GC_DATA, Registers->Graphics[i]);
    }

    /* Write the AC registers */
    // DbgPrint("\n");
    for (i = 0; i < VGA_AC_MAX_REG; i++)
    {
        VgaReadPort(VGA_INSTAT1_READ); // Put the AC register into index state
        VgaWritePort(VGA_AC_INDEX, i);
        VgaWritePort(VGA_AC_WRITE, Registers->Attribute[i]);
        // DbgPrint("Registers->Attribute[%d] = %d\n", i, Registers->Attribute[i]);
    }
    // DbgPrint("\n");

    /* Set the PEL mask */
    VgaWritePort(VGA_DAC_MASK, 0xFF);

    /* Enable screen and disable palette access */
    VgaReadPort(VGA_INSTAT1_READ); // Put the AC register into index state
    VgaWritePort(VGA_AC_INDEX, 0x20);

    /* Enable interrupts */
    setIF(1);

    return TRUE;
}

static VOID VgaSetPalette(const COLORREF* Palette, ULONG Size)
{
    ULONG i;

    // /* Disable screen and enable palette access */
    // VgaReadPort(VGA_INSTAT1_READ); // Put the AC register into index state
    // VgaWritePort(VGA_AC_INDEX, 0x00);

    for (i = 0; i < Size; i++)
    {
        VgaWritePort(VGA_DAC_WRITE_INDEX, i);
        VgaWritePort(VGA_DAC_DATA, VGA_COLOR_TO_DAC(GetRValue(Palette[i])));
        VgaWritePort(VGA_DAC_DATA, VGA_COLOR_TO_DAC(GetGValue(Palette[i])));
        VgaWritePort(VGA_DAC_DATA, VGA_COLOR_TO_DAC(GetBValue(Palette[i])));
    }

    /* The following step might be optional */
    for (i = Size; i < VGA_MAX_COLORS; i++)
    {
        VgaWritePort(VGA_DAC_WRITE_INDEX, i);
        VgaWritePort(VGA_DAC_DATA, VGA_COLOR_TO_DAC(0x00));
        VgaWritePort(VGA_DAC_DATA, VGA_COLOR_TO_DAC(0x00));
        VgaWritePort(VGA_DAC_DATA, VGA_COLOR_TO_DAC(0x00));
    }

    /* Enable screen and disable palette access */
    // VgaReadPort(VGA_INSTAT1_READ); // Put the AC register into index state
    // VgaWritePort(VGA_AC_INDEX, 0x20);
}

static VOID VgaChangePalette(BYTE ModeNumber)
{
    const COLORREF* Palette;
    ULONG Size;

    if (ModeNumber >= 0x13)
    {
        /* VGA modes */
        Palette = VgaPalette;
        Size    = sizeof(VgaPalette)/sizeof(VgaPalette[0]);
    }
    else if (ModeNumber == 0x10)
    {
        /* EGA HiRes mode */
        Palette = EgaPalette__HiRes;
        Size    = sizeof(EgaPalette__HiRes)/sizeof(EgaPalette__HiRes[0]);
    }
    else // if ((ModeNumber == 0x0D) || (ModeNumber == 0x0E))
    {
        /* EGA modes */
        Palette = EgaPalette___16ColorFixed_DOSBox;
        Size    = sizeof(EgaPalette___16ColorFixed_DOSBox)/sizeof(EgaPalette___16ColorFixed_DOSBox[0]);
    }

    VgaSetPalette(Palette, Size);
}

static VOID BiosGetCursorPosition(PBYTE Row, PBYTE Column, BYTE Page)
{
    /* Make sure the selected video page is valid */
    if (Page >= BIOS_MAX_PAGES) return;

    /* Get the cursor location */
    *Row    = HIBYTE(Bda->CursorPosition[Page]);
    *Column = LOBYTE(Bda->CursorPosition[Page]);
}

static VOID BiosSetCursorPosition(BYTE Row, BYTE Column, BYTE Page)
{
    /* Make sure the selected video page is valid */
    if (Page >= BIOS_MAX_PAGES) return;

    /* Update the position in the BDA */
    Bda->CursorPosition[Page] = MAKEWORD(Column, Row);

    /* Check if this is the current video page */
    if (Page == Bda->VideoPage)
    {
        WORD Offset = Row * Bda->ScreenColumns + Column;

        /* Modify the CRTC registers */
        VgaWritePort(VGA_CRTC_INDEX, VGA_CRTC_CURSOR_LOC_LOW_REG);
        VgaWritePort(VGA_CRTC_DATA , LOBYTE(Offset));
        VgaWritePort(VGA_CRTC_INDEX, VGA_CRTC_CURSOR_LOC_HIGH_REG);
        VgaWritePort(VGA_CRTC_DATA , HIBYTE(Offset));
    }
}

BYTE BiosGetVideoMode(VOID)
{
    return Bda->VideoMode;
}

static BOOLEAN BiosSetVideoMode(BYTE ModeNumber)
{
    BYTE Page;

    COORD Resolution;
    PVGA_REGISTERS VgaMode = VideoModes[ModeNumber];

    DPRINT1("Switching to mode %Xh; VgaMode = 0x%p\n", ModeNumber, VgaMode);

    if (!VgaSetRegisters(VgaMode)) return FALSE;

    VgaChangePalette(ModeNumber);

    /*
     * IBM standard modes do not clear the screen if the
     * high bit of AL is set (EGA or higher only).
     * See Ralf Brown: http://www.ctyme.com/intr/rb-0069.htm
     * for more information.
     */
    if ((ModeNumber & 0x08) == 0) VgaClearMemory();

    // Bda->CrtModeControl;
    // Bda->CrtColorPaletteMask;
    // Bda->EGAFlags;
    // Bda->VGAFlags;

    /* Update the values in the BDA */
    Bda->VideoMode       = ModeNumber;
    Bda->VideoPageSize   = VideoModePageSize[ModeNumber];
    Bda->VideoPage       = 0;
    Bda->VideoPageOffset = Bda->VideoPage * Bda->VideoPageSize;

    /* Set the start address in the CRTC */
    VgaWritePort(VGA_CRTC_INDEX, VGA_CRTC_START_ADDR_LOW_REG);
    VgaWritePort(VGA_CRTC_DATA , LOBYTE(Bda->VideoPageOffset));
    VgaWritePort(VGA_CRTC_INDEX, VGA_CRTC_START_ADDR_HIGH_REG);
    VgaWritePort(VGA_CRTC_DATA , HIBYTE(Bda->VideoPageOffset));

    /* Get the character height */
    VgaWritePort(VGA_CRTC_INDEX, VGA_CRTC_MAX_SCAN_LINE_REG);
    Bda->CharacterHeight = 1 + (VgaReadPort(VGA_CRTC_DATA) & 0x1F);

    Resolution = VgaGetDisplayResolution();
    Bda->ScreenColumns = Resolution.X;
    Bda->ScreenRows    = Resolution.Y - 1;

    /* Set the cursor position for each page */
    for (Page = 0; Page < BIOS_MAX_PAGES; ++Page)
        BiosSetCursorPosition(0, 0, Page);

    return TRUE;
}

static BOOLEAN BiosSetVideoPage(BYTE PageNumber)
{
    BYTE Row, Column;

    /* Check if the page exists */
    if (PageNumber >= BIOS_MAX_PAGES) return FALSE;

    /* Check if this is the same page */
    if (PageNumber == Bda->VideoPage) return TRUE;

    /* Update the values in the BDA */
    Bda->VideoPage       = PageNumber;
    Bda->VideoPageOffset = Bda->VideoPage * Bda->VideoPageSize;

    /* Set the start address in the CRTC */
    VgaWritePort(VGA_CRTC_INDEX, VGA_CRTC_START_ADDR_LOW_REG);
    VgaWritePort(VGA_CRTC_DATA , LOBYTE(Bda->VideoPageOffset));
    VgaWritePort(VGA_CRTC_INDEX, VGA_CRTC_START_ADDR_HIGH_REG);
    VgaWritePort(VGA_CRTC_DATA , HIBYTE(Bda->VideoPageOffset));

    /*
     * Get the cursor location (we don't update anything on the BIOS side
     * but we update the cursor location on the VGA side).
     */
    BiosGetCursorPosition(&Row, &Column, PageNumber);
    BiosSetCursorPosition(Row, Column, PageNumber);

    return TRUE;
}

static VOID WINAPI BiosVideoService(LPWORD Stack)
{
    switch (getAH())
    {
        /* Set Video Mode */
        case 0x00:
        {
            BiosSetVideoMode(getAL());
            break;
        }

        /* Set Text-Mode Cursor Shape */
        case 0x01:
        {
            /* Update the BDA */
            Bda->CursorStartLine = getCH();
            Bda->CursorEndLine   = getCL();

            /* Modify the CRTC registers */
            VgaWritePort(VGA_CRTC_INDEX, VGA_CRTC_CURSOR_START_REG);
            VgaWritePort(VGA_CRTC_DATA , Bda->CursorStartLine);
            VgaWritePort(VGA_CRTC_INDEX, VGA_CRTC_CURSOR_END_REG);
            VgaWritePort(VGA_CRTC_DATA , Bda->CursorEndLine);

            break;
        }

        /* Set Cursor Position */
        case 0x02:
        {
            BiosSetCursorPosition(getDH(), getDL(), getBH());
            break;
        }

        /* Get Cursor Position */
        case 0x03:
        {
            /* Make sure the selected video page exists */
            if (getBH() >= BIOS_MAX_PAGES) break;

            /* Return the result */
            setAX(0);
            setCX(MAKEWORD(Bda->CursorEndLine, Bda->CursorStartLine));
            setDX(Bda->CursorPosition[getBH()]);
            break;
        }

        /* Query Light Pen */
        case 0x04:
        {
            /*
             * On modern BIOSes, this function returns 0
             * so that we can ignore the other registers.
             */
            setAX(0);
            break;
        }

        /* Select Active Display Page */
        case 0x05:
        {
            BiosSetVideoPage(getAL());
            break;
        }

        /* Scroll Window Up/Down */
        case 0x06:
        case 0x07:
        {
            SMALL_RECT Rectangle = { getCL(), getCH(), getDL(), getDH() };

            /* Call the internal function */
            BiosScrollWindow((getAH() == 0x06) ? SCROLL_DIRECTION_UP
                                               : SCROLL_DIRECTION_DOWN,
                             getAL(),
                             Rectangle,
                             Bda->VideoPage,
                             getBH());

            break;
        }

        /* Read/Write Character From Cursor Position */
        case 0x08:
        case 0x09:
        case 0x0A:
        {
            WORD CharacterData = MAKEWORD(getAL(), getBL());
            BYTE Page = getBH();
            DWORD Offset;

            /* Check if the page exists */
            if (Page >= BIOS_MAX_PAGES) break;

            /* Find the offset of the character */
            Offset = Page * Bda->VideoPageSize +
                     (HIBYTE(Bda->CursorPosition[Page]) * Bda->ScreenColumns +
                      LOBYTE(Bda->CursorPosition[Page])) * 2;

            if (getAH() == 0x08)
            {
                /* Read from the video memory */
                VgaReadMemory(TO_LINEAR(TEXT_VIDEO_SEG, Offset),
                              (LPVOID)&CharacterData,
                              sizeof(WORD));

                /* Return the character in AX */
                setAX(CharacterData);
            }
            else
            {
                /* Write to video memory */
                VgaWriteMemory(TO_LINEAR(TEXT_VIDEO_SEG, Offset),
                               (LPVOID)&CharacterData,
                               (getBH() == 0x09) ? sizeof(WORD) : sizeof(BYTE));
            }

            break;
        }

        /* Teletype Output */
        case 0x0E:
        {
            BiosPrintCharacter(getAL(), getBL(), getBH());
            break;
        }

        /* Get Current Video Mode */
        case 0x0F:
        {
            setAX(MAKEWORD(Bda->VideoMode, Bda->ScreenColumns));
            setBX(MAKEWORD(getBL(), Bda->VideoPage));
            break;
        }

        /* Palette Control */
        case 0x10:
        {
            switch (getAL())
            {
                /* Set Single Palette Register */
                case 0x00:
                {
                    /* Write the index */
                    VgaReadPort(VGA_INSTAT1_READ); // Put the AC register into index state
                    VgaWritePort(VGA_AC_INDEX, getBL());

                    /* Write the data */
                    VgaWritePort(VGA_AC_WRITE, getBH());

                    /* Enable screen and disable palette access */
                    VgaReadPort(VGA_INSTAT1_READ); // Put the AC register into index state
                    VgaWritePort(VGA_AC_INDEX, 0x20);
                    break;
                }

                /* Set Overscan Color */
                case 0x01:
                {
                    /* Write the index */
                    VgaReadPort(VGA_INSTAT1_READ); // Put the AC register into index state
                    VgaWritePort(VGA_AC_INDEX, VGA_AC_OVERSCAN_REG);

                    /* Write the data */
                    VgaWritePort(VGA_AC_WRITE, getBH());

                    /* Enable screen and disable palette access */
                    VgaReadPort(VGA_INSTAT1_READ); // Put the AC register into index state
                    VgaWritePort(VGA_AC_INDEX, 0x20);
                    break;
                }

                /* Set All Palette Registers */
                case 0x02:
                {
                    INT i;
                    LPBYTE Buffer = SEG_OFF_TO_PTR(getES(), getDX());

                    /* Set the palette registers */
                    for (i = 0; i <= VGA_AC_PAL_F_REG; i++)
                    {
                        /* Write the index */
                        VgaReadPort(VGA_INSTAT1_READ); // Put the AC register into index state
                        VgaWritePort(VGA_AC_INDEX, i);

                        /* Write the data */
                        VgaWritePort(VGA_AC_WRITE, Buffer[i]);
                    }

                    /* Set the overscan register */
                    VgaWritePort(VGA_AC_INDEX, VGA_AC_OVERSCAN_REG);
                    VgaWritePort(VGA_AC_WRITE, Buffer[VGA_AC_PAL_F_REG + 1]);

                    /* Enable screen and disable palette access */
                    VgaReadPort(VGA_INSTAT1_READ); // Put the AC register into index state
                    VgaWritePort(VGA_AC_INDEX, 0x20);
                    break;
                }

                /* Get Single Palette Register */
                case 0x07:
                {
                    /* Write the index */
                    VgaReadPort(VGA_INSTAT1_READ); // Put the AC register into index state
                    VgaWritePort(VGA_AC_INDEX, getBL());

                    /* Read the data */
                    setBH(VgaReadPort(VGA_AC_READ));

                    /* Enable screen and disable palette access */
                    VgaReadPort(VGA_INSTAT1_READ); // Put the AC register into index state
                    VgaWritePort(VGA_AC_INDEX, 0x20);
                    break;
                }

                /* Get Overscan Color */
                case 0x08:
                {
                    /* Write the index */
                    VgaReadPort(VGA_INSTAT1_READ); // Put the AC register into index state
                    VgaWritePort(VGA_AC_INDEX, VGA_AC_OVERSCAN_REG);

                    /* Read the data */
                    setBH(VgaReadPort(VGA_AC_READ));

                    /* Enable screen and disable palette access */
                    VgaReadPort(VGA_INSTAT1_READ); // Put the AC register into index state
                    VgaWritePort(VGA_AC_INDEX, 0x20);
                    break;
                }

                /* Get All Palette Registers */
                case 0x09:
                {
                    INT i;
                    LPBYTE Buffer = SEG_OFF_TO_PTR(getES(), getDX());

                    /* Get the palette registers */
                    for (i = 0; i <= VGA_AC_PAL_F_REG; i++)
                    {
                        /* Write the index */
                        VgaReadPort(VGA_INSTAT1_READ); // Put the AC register into index state
                        VgaWritePort(VGA_AC_INDEX, i);

                        /* Read the data */
                        Buffer[i] = VgaReadPort(VGA_AC_READ);
                    }

                    /* Get the overscan register */
                    VgaWritePort(VGA_AC_INDEX, VGA_AC_OVERSCAN_REG);
                    Buffer[VGA_AC_PAL_F_REG + 1] = VgaReadPort(VGA_AC_READ);

                    /* Enable screen and disable palette access */
                    VgaReadPort(VGA_INSTAT1_READ); // Put the AC register into index state
                    VgaWritePort(VGA_AC_INDEX, 0x20);
                    break;
                }

                /* Set Individual DAC Register */
                case 0x10:
                {
                    /* Write the index */
                    // Certainly in BL and not in BX as said by Ralf Brown...
                    VgaWritePort(VGA_DAC_WRITE_INDEX, getBL());

                    /* Write the data in this order: Red, Green, Blue */
                    VgaWritePort(VGA_DAC_DATA, getDH());
                    VgaWritePort(VGA_DAC_DATA, getCH());
                    VgaWritePort(VGA_DAC_DATA, getCL());

                    break;
                }

                /* Set Block of DAC Registers */
                case 0x12:
                {
                    INT i;
                    LPBYTE Buffer = SEG_OFF_TO_PTR(getES(), getDX());

                    /* Write the index */
                    // Certainly in BL and not in BX as said by Ralf Brown...
                    VgaWritePort(VGA_DAC_WRITE_INDEX, getBL());

                    for (i = 0; i < getCX(); i++)
                    {
                        /* Write the data in this order: Red, Green, Blue */
                        VgaWritePort(VGA_DAC_DATA, *Buffer++);
                        VgaWritePort(VGA_DAC_DATA, *Buffer++);
                        VgaWritePort(VGA_DAC_DATA, *Buffer++);
                    }

                    break;
                }

                /* Get Individual DAC Register */
                case 0x15:
                {
                    /* Write the index */
                    VgaWritePort(VGA_DAC_READ_INDEX, getBL());

                    /* Read the data in this order: Red, Green, Blue */
                    setDH(VgaReadPort(VGA_DAC_DATA));
                    setCH(VgaReadPort(VGA_DAC_DATA));
                    setCL(VgaReadPort(VGA_DAC_DATA));

                    break;
                }

                /* Get Block of DAC Registers */
                case 0x17:
                {
                    INT i;
                    LPBYTE Buffer = SEG_OFF_TO_PTR(getES(), getDX());

                    /* Write the index */
                    // Certainly in BL and not in BX as said by Ralf Brown...
                    VgaWritePort(VGA_DAC_READ_INDEX, getBL());

                    for (i = 0; i < getCX(); i++)
                    {
                        /* Write the data in this order: Red, Green, Blue */
                        *Buffer++ = VgaReadPort(VGA_DAC_DATA);
                        *Buffer++ = VgaReadPort(VGA_DAC_DATA);
                        *Buffer++ = VgaReadPort(VGA_DAC_DATA);
                    }

                    break;
                }

                default:
                {
                    DPRINT1("BIOS Palette Control Sub-command AL = 0x%02X NOT IMPLEMENTED\n",
                            getAL());
                    break;
                }
            }

            break;
        }

        /* Scroll Window */
        case 0x12:
        {
            SMALL_RECT Rectangle = { getCL(), getCH(), getDL(), getDH() };

            /* Call the internal function */
            BiosScrollWindow(getBL(),
                             getAL(),
                             Rectangle,
                             Bda->VideoPage,
                             DEFAULT_ATTRIBUTE);

            break;
        }

        /* Display combination code */
        case 0x1A:
        {
            switch(getAL())
            {
                case 0x00: /* Get Display combiantion code */
                   setAX(MAKEWORD(0x1A, 0x1A));
                   setBX(MAKEWORD(0x08, 0x00)); /* VGA w/ color analog display */
                   break;
                case 0x01: /* Set Display combination code */
                   DPRINT1("Set Display combination code - Unsupported\n");
                   break;
                default:
                   break;
            }
            break;
        }

        default:
        {
            DPRINT1("BIOS Function INT 10h, AH = 0x%02X NOT IMPLEMENTED\n",
                    getAH());
        }
    }
}

static VOID WINAPI BiosEquipmentService(LPWORD Stack)
{
    /* Return the equipment list */
    setAX(Bda->EquipmentList);
}

static VOID WINAPI BiosGetMemorySize(LPWORD Stack)
{
    /* Return the conventional memory size in kB, typically 640 kB */
    setAX(Bda->MemorySize);
}

static VOID WINAPI BiosMiscService(LPWORD Stack)
{
    switch (getAH())
    {
        /* Copy Extended Memory */
        case 0x87:
        {
            DWORD Count = (DWORD)getCX() * 2;
            PFAST486_GDT_ENTRY Gdt = (PFAST486_GDT_ENTRY)SEG_OFF_TO_PTR(getES(), getSI());
            DWORD SourceBase = Gdt[2].Base + (Gdt[2].BaseMid << 16) + (Gdt[2].BaseHigh << 24);
            DWORD SourceLimit = Gdt[2].Limit + (Gdt[2].LimitHigh << 16);
            DWORD DestBase = Gdt[3].Base + (Gdt[3].BaseMid << 16) + (Gdt[3].BaseHigh << 24);
            DWORD DestLimit = Gdt[3].Limit + (Gdt[3].LimitHigh << 16);

            /* Check for flags */
            if (Gdt[2].Granularity) SourceLimit = (SourceLimit << 12) | 0xFFF;
            if (Gdt[3].Granularity) DestLimit = (DestLimit << 12) | 0xFFF;

            if ((Count > SourceLimit) || (Count > DestLimit))
            {
                setAX(0x80);
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_CF;

                break;
            }

            /* Copy */
            RtlMoveMemory((PVOID)((ULONG_PTR)BaseAddress + DestBase),
                          (PVOID)((ULONG_PTR)BaseAddress + SourceBase),
                          Count);

            setAX(ERROR_SUCCESS);
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;
            break;
        }

        /* Get Extended Memory Size */
        case 0x88:
        {
            /* Return the number of KB of RAM after 1 MB */
            setAX((MAX_ADDRESS - 0x100000) / 1024);

            /* Clear CF */
            Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_CF;

            break;
        }

        default:
        {
            DPRINT1("BIOS Function INT 15h, AH = 0x%02X NOT IMPLEMENTED\n",
                    getAH());
        }
    }
}

static VOID WINAPI BiosKeyboardService(LPWORD Stack)
{
    switch (getAH())
    {
        /* Wait for keystroke and read */
        case 0x00:
        /* Wait for extended keystroke and read */
        case 0x10:  // FIXME: Temporarily do the same as INT 16h, 00h
        {
            /* Read the character (and wait if necessary) */
            setAX(BiosGetCharacter());
            break;
        }

        /* Get keystroke status */
        case 0x01:
        /* Get extended keystroke status */
        case 0x11:  // FIXME: Temporarily do the same as INT 16h, 01h
        {
            WORD Data = BiosPeekCharacter();

            if (Data != 0xFFFF)
            {
                /* There is a character, clear ZF and return it */
                Stack[STACK_FLAGS] &= ~EMULATOR_FLAG_ZF;
                setAX(Data);
            }
            else
            {
                /* No character, set ZF */
                Stack[STACK_FLAGS] |= EMULATOR_FLAG_ZF;
            }

            break;
        }

        /* Get shift status */
        case 0x02:
        {
            /* Return the lower byte of the keyboard shift status word */
            setAL(LOBYTE(Bda->KeybdShiftFlags));
            break;
        }

        /* Reserved */
        case 0x04:
        {
            DPRINT1("BIOS Function INT 16h, AH = 0x04 is RESERVED\n");
            break;
        }

        /* Push keystroke */
        case 0x05:
        {
            /* Return 0 if success, 1 if failure */
            setAL(BiosKbdBufferPush(getCX()) == FALSE);
            break;
        }

        /* Get extended shift status */
        case 0x12:
        {
            /*
             * Be careful! The returned word is similar to Bda->KeybdShiftFlags
             * but the high byte is organized differently:
             * the bytes 2 and 3 of the high byte are not the same...
             */
            WORD KeybdShiftFlags = (Bda->KeybdShiftFlags & 0xF3FF);

            /* Return the extended keyboard shift status word */
            setAX(KeybdShiftFlags);
            break;
        }

        default:
        {
            DPRINT1("BIOS Function INT 16h, AH = 0x%02X NOT IMPLEMENTED\n",
                    getAH());
        }
    }
}

static VOID WINAPI BiosTimeService(LPWORD Stack)
{
    switch (getAH())
    {
        case 0x00:
        {
            /* Set AL to 1 if midnight had passed, 0 otherwise */
            setAL(Bda->MidnightPassed ? 0x01 : 0x00);

            /* Return the tick count in CX:DX */
            setCX(HIWORD(Bda->TickCounter));
            setDX(LOWORD(Bda->TickCounter));

            /* Reset the midnight flag */
            Bda->MidnightPassed = FALSE;

            break;
        }

        case 0x01:
        {
            /* Set the tick count to CX:DX */
            Bda->TickCounter = MAKELONG(getDX(), getCX());

            /* Reset the midnight flag */
            Bda->MidnightPassed = FALSE;

            break;
        }

        default:
        {
            DPRINT1("BIOS Function INT 1Ah, AH = 0x%02X NOT IMPLEMENTED\n",
                    getAH());
        }
    }
}

static VOID WINAPI BiosSystemTimerInterrupt(LPWORD Stack)
{
    /* Increase the system tick count */
    Bda->TickCounter++;
}

/* PUBLIC FUNCTIONS ***********************************************************/

WORD BiosPeekCharacter(VOID)
{
    WORD CharacterData = 0;

    /* Get the key from the queue, but don't remove it */
    if (BiosKbdBufferTop(&CharacterData)) return CharacterData;
    else return 0xFFFF;
}

WORD BiosGetCharacter(VOID)
{
    WORD CharacterData = 0;

    /* Check if there is a key available */
    if (BiosKbdBufferTop(&CharacterData))
    {
        /* A key was available, remove it from the queue */
        BiosKbdBufferPop();
    }
    else
    {
        /* No key available. Set the handler CF to repeat the BOP */
        setCF(1);
        // CharacterData = 0xFFFF;
    }

    return CharacterData;
}

VOID BiosPrintCharacter(CHAR Character, BYTE Attribute, BYTE Page)
{
    WORD CharData = MAKEWORD(Character, Attribute);
    BYTE Row, Column;

    /* Make sure the page exists */
    if (Page >= BIOS_MAX_PAGES) return;

    /* Get the cursor location */
    BiosGetCursorPosition(&Row, &Column, Page);

    if (Character == '\a')
    {
        /* Bell control character */
        // NOTE: We may use what the terminal emulator offers to us...
        Beep(800, 200);
        return;
    }
    else if (Character == '\b')
    {
        /* Backspace control character */
        if (Column > 0)
        {
            Column--;
        }
        else if (Row > 0)
        {
            Column = Bda->ScreenColumns - 1;
            Row--;
        }

        /* Erase the existing character */
        CharData = MAKEWORD(' ', Attribute);
        EmulatorWriteMemory(&EmulatorContext,
                            TO_LINEAR(TEXT_VIDEO_SEG,
                                Page * Bda->VideoPageSize +
                                (Row * Bda->ScreenColumns + Column) * sizeof(WORD)),
                            (LPVOID)&CharData,
                            sizeof(WORD));
    }
    else if (Character == '\t')
    {
        /* Horizontal Tabulation control character */
        do
        {
            // Taken from DOSBox
            BiosPrintCharacter(' ', Attribute, Page);
            BiosGetCursorPosition(&Row, &Column, Page);
        } while (Column % 8);
    }
    else if (Character == '\n')
    {
        /* Line Feed control character */
        Row++;
    }
    else if (Character == '\r')
    {
        /* Carriage Return control character */
        Column = 0;
    }
    else
    {
        /* Default character */

        /* Write the character */
        EmulatorWriteMemory(&EmulatorContext,
                            TO_LINEAR(TEXT_VIDEO_SEG,
                                Page * Bda->VideoPageSize +
                                (Row * Bda->ScreenColumns + Column) * sizeof(WORD)),
                            (LPVOID)&CharData,
                            sizeof(WORD));

        /* Advance the cursor */
        Column++;
    }

    /* Check if it passed the end of the row */
    if (Column >= Bda->ScreenColumns)
    {
        /* Return to the first column and go to the next line */
        Column = 0;
        Row++;
    }

    /* Scroll the screen up if needed */
    if (Row > Bda->ScreenRows)
    {
        /* The screen must be scrolled up */
        SMALL_RECT Rectangle = { 0, 0, Bda->ScreenColumns - 1, Bda->ScreenRows };

        BiosScrollWindow(SCROLL_DIRECTION_UP,
                         1,
                         Rectangle,
                         Page,
                         DEFAULT_ATTRIBUTE);

        Row--;
    }

    /* Set the cursor position */
    BiosSetCursorPosition(Row, Column, Page);
}

BOOLEAN BiosInitialize(VOID)
{
    CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;

    /* Initialize the BDA */
    Bda = (PBIOS_DATA_AREA)SEG_OFF_TO_PTR(BDA_SEGMENT, 0);
    Bda->EquipmentList = BIOS_EQUIPMENT_LIST;
    /*
     * Conventional memory size is 640 kB,
     * see: http://webpages.charter.net/danrollins/techhelp/0184.HTM
     * and see Ralf Brown: http://www.ctyme.com/intr/rb-0598.htm
     * for more information.
     */
    Bda->MemorySize = 0x0280;
    Bda->KeybdBufferStart = FIELD_OFFSET(BIOS_DATA_AREA, KeybdBuffer);
    Bda->KeybdBufferEnd = Bda->KeybdBufferStart + BIOS_KBD_BUFFER_SIZE * sizeof(WORD);
    Bda->KeybdBufferHead = Bda->KeybdBufferTail = 0;

    /* Initialize the 32-bit Interrupt system */
    InitializeInt32(BIOS_SEGMENT);

    /* Register the BIOS 32-bit Interrupts */
    RegisterInt32(BIOS_VIDEO_INTERRUPT    , BiosVideoService        );
    RegisterInt32(BIOS_EQUIPMENT_INTERRUPT, BiosEquipmentService    );
    RegisterInt32(BIOS_MEMORY_SIZE        , BiosGetMemorySize       );
    RegisterInt32(BIOS_MISC_INTERRUPT     , BiosMiscService         );
    RegisterInt32(BIOS_KBD_INTERRUPT      , BiosKeyboardService     );
    RegisterInt32(BIOS_TIME_INTERRUPT     , BiosTimeService         );
    RegisterInt32(BIOS_SYS_TIMER_INTERRUPT, BiosSystemTimerInterrupt);

    /* Some interrupts are in fact addresses to tables */
    ((PDWORD)BaseAddress)[0x1D] = (DWORD)NULL;
    ((PDWORD)BaseAddress)[0x1E] = (DWORD)NULL;
    ((PDWORD)BaseAddress)[0x1F] = (DWORD)NULL;

    ((PDWORD)BaseAddress)[0x41] = (DWORD)NULL;
    ((PDWORD)BaseAddress)[0x43] = (DWORD)NULL;
    ((PDWORD)BaseAddress)[0x44] = (DWORD)NULL;
    ((PDWORD)BaseAddress)[0x46] = (DWORD)NULL;
    ((PDWORD)BaseAddress)[0x48] = (DWORD)NULL;
    ((PDWORD)BaseAddress)[0x49] = (DWORD)NULL;

    /* Get the input handle to the real console, and check for success */
    BiosConsoleInput = CreateFileW(L"CONIN$",
                                   GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL,
                                   OPEN_EXISTING,
                                   0,
                                   NULL);
    if (BiosConsoleInput == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    /* Get the output handle to the real console, and check for success */
    BiosConsoleOutput = CreateFileW(L"CONOUT$",
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL);
    if (BiosConsoleOutput == INVALID_HANDLE_VALUE)
    {
        CloseHandle(BiosConsoleInput);
        return FALSE;
    }

    /* Save the original input and output console modes */
    if (!GetConsoleMode(BiosConsoleInput , &BiosSavedConInMode ) ||
        !GetConsoleMode(BiosConsoleOutput, &BiosSavedConOutMode))
    {
        CloseHandle(BiosConsoleOutput);
        CloseHandle(BiosConsoleInput);
        return FALSE;
    }

    /* Save the original cursor and console screen buffer information */
    if (!GetConsoleCursorInfo(BiosConsoleOutput, &BiosSavedCursorInfo) ||
        !GetConsoleScreenBufferInfo(BiosConsoleOutput, &BiosSavedBufferInfo))
    {
        CloseHandle(BiosConsoleOutput);
        CloseHandle(BiosConsoleInput);
        return FALSE;
    }

    /* Initialize VGA */
    if (!VgaInitialize(BiosConsoleOutput))
    {
        CloseHandle(BiosConsoleOutput);
        CloseHandle(BiosConsoleInput);
        return FALSE;
    }

    /* Set the default video mode */
    BiosSetVideoMode(BIOS_DEFAULT_VIDEO_MODE);

    /* Copy console data into VGA memory */
    BiosCopyTextConsoleToVgaMemory();

    /* Update the cursor position for the current page */
    GetConsoleScreenBufferInfo(BiosConsoleOutput, &ConsoleInfo);
    BiosSetCursorPosition(ConsoleInfo.dwCursorPosition.Y,
                          ConsoleInfo.dwCursorPosition.X,
                          Bda->VideoPage);

    /* Set the console input mode */
    SetConsoleMode(BiosConsoleInput, ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT);

    /* Initialize PS2 */
    PS2Initialize(BiosConsoleInput);

    /* Initialize the PIC */
    PicWriteCommand(PIC_MASTER_CMD, PIC_ICW1 | PIC_ICW1_ICW4);
    PicWriteCommand(PIC_SLAVE_CMD , PIC_ICW1 | PIC_ICW1_ICW4);

    /* Set the interrupt offsets */
    PicWriteData(PIC_MASTER_DATA, BIOS_PIC_MASTER_INT);
    PicWriteData(PIC_SLAVE_DATA , BIOS_PIC_SLAVE_INT);

    /* Tell the master PIC there is a slave at IRQ 2 */
    PicWriteData(PIC_MASTER_DATA, 1 << 2);
    PicWriteData(PIC_SLAVE_DATA , 2);

    /* Make sure the PIC is in 8086 mode */
    PicWriteData(PIC_MASTER_DATA, PIC_ICW4_8086);
    PicWriteData(PIC_SLAVE_DATA , PIC_ICW4_8086);

    /* Clear the masks for both PICs */
    PicWriteData(PIC_MASTER_DATA, 0x00);
    PicWriteData(PIC_SLAVE_DATA , 0x00);

    PitWriteCommand(0x34);
    PitWriteData(0, 0x00);
    PitWriteData(0, 0x00);

    return TRUE;
}

VOID BiosCleanup(VOID)
{
    SMALL_RECT ConRect;
    CONSOLE_SCREEN_BUFFER_INFO ConsoleInfo;

    PS2Cleanup();

    /* Restore the old screen buffer */
    SetConsoleActiveScreenBuffer(BiosConsoleOutput);

    /* Restore the original console size */
    GetConsoleScreenBufferInfo(BiosConsoleOutput, &ConsoleInfo);
    ConRect.Left = 0; // BiosSavedBufferInfo.srWindow.Left;
    // ConRect.Top  = ConsoleInfo.dwCursorPosition.Y / (BiosSavedBufferInfo.srWindow.Bottom - BiosSavedBufferInfo.srWindow.Top + 1);
    // ConRect.Top *= (BiosSavedBufferInfo.srWindow.Bottom - BiosSavedBufferInfo.srWindow.Top + 1);
    ConRect.Top    = ConsoleInfo.dwCursorPosition.Y;
    ConRect.Right  = ConRect.Left + BiosSavedBufferInfo.srWindow.Right - BiosSavedBufferInfo.srWindow.Left;
    ConRect.Bottom = ConRect.Top  + (BiosSavedBufferInfo.srWindow.Bottom - BiosSavedBufferInfo.srWindow.Top);
    /* See the following trick explanation in vga.c:VgaEnterTextMode() */
    SetConsoleScreenBufferSize(BiosConsoleOutput, BiosSavedBufferInfo.dwSize);
    SetConsoleWindowInfo(BiosConsoleOutput, TRUE, &ConRect);
    // SetConsoleWindowInfo(BiosConsoleOutput, TRUE, &BiosSavedBufferInfo.srWindow);
    SetConsoleScreenBufferSize(BiosConsoleOutput, BiosSavedBufferInfo.dwSize);

    /* Restore the original cursor shape */
    SetConsoleCursorInfo(BiosConsoleOutput, &BiosSavedCursorInfo);

    /* Restore the original input and output console modes */
    SetConsoleMode(BiosConsoleOutput, BiosSavedConOutMode);
    SetConsoleMode(BiosConsoleInput , BiosSavedConInMode );

    /* Close the console handles */
    if (BiosConsoleOutput != INVALID_HANDLE_VALUE) CloseHandle(BiosConsoleOutput);
    if (BiosConsoleInput  != INVALID_HANDLE_VALUE) CloseHandle(BiosConsoleInput);
}

VOID BiosHandleIrq(BYTE IrqNumber, LPWORD Stack)
{
    switch (IrqNumber)
    {
        /* PIT IRQ */
        case 0:
        {
            /* Perform the system timer interrupt */
            EmulatorInterrupt(BIOS_SYS_TIMER_INTERRUPT);
            break;
        }

        /* Keyboard IRQ */
        case 1:
        {
            BYTE ScanCode, VirtualKey;
            WORD Character;

            /* Get the scan code and virtual key code */
            ScanCode = PS2ReadPort(PS2_DATA_PORT);
            VirtualKey = MapVirtualKey(ScanCode & 0x7F, MAPVK_VSC_TO_VK);

            /* Check if this is a key press or release */
            if (!(ScanCode & (1 << 7)))
            {
                /* Key press */
                if (VirtualKey == VK_NUMLOCK ||
                    VirtualKey == VK_CAPITAL ||
                    VirtualKey == VK_SCROLL  ||
                    VirtualKey == VK_INSERT)
                {
                    /* For toggle keys, toggle the lowest bit in the keyboard map */
                    BiosKeyboardMap[VirtualKey] ^= ~(1 << 0);
                }

                /* Set the highest bit */
                BiosKeyboardMap[VirtualKey] |= (1 << 7);

                /* Find out which character this is */
                Character = 0;
                if (ToAscii(VirtualKey, ScanCode, BiosKeyboardMap, &Character, 0) == 0)
                {
                    /* Not ASCII */
                    Character = 0;
                }

                /* Push it onto the BIOS keyboard queue */
                BiosKbdBufferPush(MAKEWORD(Character, ScanCode));
            }
            else
            {
                /* Key release, unset the highest bit */
                BiosKeyboardMap[VirtualKey] &= ~(1 << 7);
            }

            /* Clear the keyboard flags */
            Bda->KeybdShiftFlags = 0;

            /* Set the appropriate flags based on the state */
            if (BiosKeyboardMap[VK_RSHIFT]   & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_RSHIFT;
            if (BiosKeyboardMap[VK_LSHIFT]   & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_LSHIFT;
            if (BiosKeyboardMap[VK_CONTROL]  & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_CTRL;
            if (BiosKeyboardMap[VK_MENU]     & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_ALT;
            if (BiosKeyboardMap[VK_SCROLL]   & (1 << 0)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_SCROLL_ON;
            if (BiosKeyboardMap[VK_NUMLOCK]  & (1 << 0)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_NUMLOCK_ON;
            if (BiosKeyboardMap[VK_CAPITAL]  & (1 << 0)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_CAPSLOCK_ON;
            if (BiosKeyboardMap[VK_INSERT]   & (1 << 0)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_INSERT_ON;
            if (BiosKeyboardMap[VK_RMENU]    & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_RALT;
            if (BiosKeyboardMap[VK_LMENU]    & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_LALT;
            if (BiosKeyboardMap[VK_SNAPSHOT] & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_SYSRQ;
            if (BiosKeyboardMap[VK_PAUSE]    & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_PAUSE;
            if (BiosKeyboardMap[VK_SCROLL]   & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_SCROLL;
            if (BiosKeyboardMap[VK_NUMLOCK]  & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_NUMLOCK;
            if (BiosKeyboardMap[VK_CAPITAL]  & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_CAPSLOCK;
            if (BiosKeyboardMap[VK_INSERT]   & (1 << 7)) Bda->KeybdShiftFlags |= BDA_KBDFLAG_INSERT;

            break;
        }
    }

    /* Send End-of-Interrupt to the PIC */
    if (IrqNumber >= 8) PicWriteCommand(PIC_SLAVE_CMD, PIC_OCW2_EOI);
    PicWriteCommand(PIC_MASTER_CMD, PIC_OCW2_EOI);
}

/* EOF */
