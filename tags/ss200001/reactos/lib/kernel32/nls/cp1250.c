/*
 * nls/cp1250.c
 * Copyright (C) 1996, Onno Hovers
 * 
 * This file has been generated by gencp.exe
 */

#include <windows.h>
#include <kernel32/nls.h>


WCHAR __CP1250_80[32]=
{
   0x0080, 0x0081, 0x201A, 0x0083, 0x201E, 0x2026, 0x2020, 0x2021, 
   0x0088, 0x2030, 0x0160, 0x2039, 0x015A, 0x0164, 0x017D, 0x0179, 
   0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 
   0x0098, 0x2122, 0x0161, 0x203A, 0x015B, 0x0165, 0x017E, 0x017A, 
};

WCHAR __CP1250_A0[32]=
{
   0x00A0, 0x02C7, 0x02D8, 0x0141, 0x00A4, 0x0104, 0x00A6, 0x00A7, 
   0x00A8, 0x00A9, 0x015E, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x017B, 
   0x00B0, 0x00B1, 0x02DB, 0x0142, 0x00B4, 0x00B5, 0x00B6, 0x00B7, 
   0x00B8, 0x0105, 0x015F, 0x00BB, 0x013D, 0x02DD, 0x013E, 0x017C, 
};

WCHAR __CP1250_C0[32]=
{
   0x0154, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0139, 0x0106, 0x00C7, 
   0x010C, 0x00C9, 0x0118, 0x00CB, 0x011A, 0x00CD, 0x00CE, 0x010E, 
   0x0110, 0x0143, 0x0147, 0x00D3, 0x00D4, 0x0150, 0x00D6, 0x00D7, 
   0x0158, 0x016E, 0x00DA, 0x0170, 0x00DC, 0x00DD, 0x0162, 0x00DF, 
};

WCHAR __CP1250_E0[32]=
{
   0x0155, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x013A, 0x0107, 0x00E7, 
   0x010D, 0x00E9, 0x0119, 0x00EB, 0x011B, 0x00ED, 0x00EE, 0x010F, 
   0x0111, 0x0144, 0x0148, 0x00F3, 0x00F4, 0x0151, 0x00F6, 0x00F7, 
   0x0159, 0x016F, 0x00FA, 0x0171, 0x00FC, 0x00FD, 0x0163, 0x02D9, 
};


WCHAR *__CP1250_ToUnicode[8]=
{
   __ASCII_00, __ASCII_20, __ASCII_40, __ASCII_60,
   __CP1250_80, __CP1250_A0, __CP1250_C0, __CP1250_E0
};


CHAR __CP1250_0080[32]=
{
   0x80, 0x81, 0x00, 0x83, 0x00, 0x00, 0x00, 0x00, 
   0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x90, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_00A0[32]=
{
   0xA0, 0x21, 0x63, 0x4C, 0xA4, 0x59, 0xA6, 0xA7, 
   0xA8, 0xA9, 0x61, 0xAB, 0xAC, 0xAD, 0xAE, 0x97, 
   0xB0, 0xB1, 0x32, 0x33, 0xB4, 0xB5, 0xB6, 0xB7, 
   0xB8, 0x31, 0x6F, 0xBB, 0x31, 0x31, 0x33, 0x00, 
};

CHAR __CP1250_00C0[32]=
{
   0x41, 0xC1, 0xC2, 0x41, 0xC4, 0x41, 0x41, 0xC7, 
   0x45, 0xC9, 0x45, 0xCB, 0x49, 0xCD, 0xCE, 0x49, 
   0x00, 0x4E, 0x4F, 0xD3, 0xD4, 0x4F, 0xD6, 0xD7, 
   0x4F, 0x55, 0xDA, 0x55, 0xDC, 0xDD, 0x00, 0xDF, 
};

CHAR __CP1250_00E0[32]=
{
   0x61, 0xE1, 0xE2, 0x61, 0xE4, 0x61, 0x61, 0xE7, 
   0x65, 0xE9, 0x65, 0xEB, 0x69, 0xED, 0xEE, 0x69, 
   0x00, 0x6E, 0x6F, 0xF3, 0xF4, 0x6F, 0xF6, 0xF7, 
   0x6F, 0x75, 0xFA, 0x75, 0xFC, 0xFD, 0x00, 0x79, 
};

CHAR __CP1250_0100[32]=
{
   0x41, 0x61, 0xC3, 0xE3, 0xA5, 0xB9, 0xC6, 0xE6, 
   0x43, 0x63, 0x43, 0x63, 0xC8, 0xE8, 0xCF, 0xEF, 
   0xD0, 0xF0, 0x45, 0x65, 0x45, 0x65, 0x45, 0x65, 
   0xCA, 0xEA, 0xCC, 0xEC, 0x47, 0x67, 0x47, 0x67, 
};

CHAR __CP1250_0120[32]=
{
   0x47, 0x67, 0x47, 0x67, 0x48, 0x68, 0x48, 0x68, 
   0x49, 0x69, 0x49, 0x69, 0x49, 0x69, 0x49, 0x69, 
   0x49, 0x69, 0x00, 0x00, 0x4A, 0x6A, 0x4B, 0x6B, 
   0x00, 0xC5, 0xE5, 0x4C, 0x6C, 0xBC, 0xBE, 0x00, 
};

CHAR __CP1250_0140[32]=
{
   0x00, 0xA3, 0xB3, 0xD1, 0xF1, 0x4E, 0x6E, 0xD2, 
   0xF2, 0x00, 0x00, 0x00, 0x4F, 0x6F, 0x4F, 0x6F, 
   0xD5, 0xF5, 0x4F, 0x6F, 0xC0, 0xE0, 0x52, 0x72, 
   0xD8, 0xF8, 0x8C, 0x9C, 0x53, 0x73, 0xAA, 0xBA, 
};

CHAR __CP1250_0160[32]=
{
   0x8A, 0x9A, 0xDE, 0xFE, 0x8D, 0x9D, 0x54, 0x74, 
   0x55, 0x75, 0x55, 0x75, 0x55, 0x75, 0xD9, 0xF9, 
   0xDB, 0xFB, 0x55, 0x75, 0x57, 0x77, 0x59, 0x79, 
   0x59, 0x8F, 0x9F, 0xAF, 0xBF, 0x8E, 0x9E, 0x00, 
};

CHAR __CP1250_0180[32]=
{
   0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0xD0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x46, 0x66, 0x00, 0x00, 0x00, 0x00, 0x49, 
   0x00, 0x00, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x4F, 
};

CHAR __CP1250_01A0[32]=
{
   0x4F, 0x6F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x54, 0x55, 
   0x75, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7A, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_01C0[32]=
{
   0x7C, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x61, 0x49, 
   0x69, 0x4F, 0x6F, 0x55, 0x75, 0x55, 0x75, 0x55, 
   0x75, 0x55, 0x75, 0x55, 0x75, 0x00, 0x41, 0x61, 
};

CHAR __CP1250_01E0[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x47, 0x67, 0x47, 0x67, 
   0x4B, 0x6B, 0x4F, 0x6F, 0x4F, 0x6F, 0x00, 0x00, 
   0x6A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_0260[32]=
{
   0x00, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_02A0[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x27, 0x22, 0x91, 0x27, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_02C0[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x5E, 0x00, 0x5E, 0xA1, 
   0x27, 0xAF, 0xB4, 0x60, 0x00, 0x5F, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0xA2, 0xFF, 0xB0, 0xB2, 0x7E, 0xBD, 0x00, 0x00, 
};

CHAR __CP1250_0300[32]=
{
   0x60, 0xB4, 0x5E, 0x7E, 0xAF, 0xAF, 0xA2, 0xFF, 
   0xA8, 0x00, 0xB0, 0x00, 0xA1, 0x00, 0x22, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_0320[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB8, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x5F, 0x5F, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_0360[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3B, 0x00, 
};

CHAR __CP1250_03A0[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0xDF, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0xB5, 0x00, 0x00, 0x00, 
};


CHAR *__CP1250_00XX[32]=
{
   __ASCII_0000, __ASCII_0020, __ASCII_0040, __ASCII_0060,
   __CP1250_0080,   __CP1250_00A0,   __CP1250_00C0,   __CP1250_00E0,
   __CP1250_0100,   __CP1250_0120,   __CP1250_0140,   __CP1250_0160,
   __CP1250_0180,   __CP1250_01A0,   __CP1250_01C0,   __CP1250_01E0,
   __NULL_0000,   __NULL_0000,   __NULL_0000,   __CP1250_0260,
   __NULL_0000,   __CP1250_02A0,   __CP1250_02C0,   __NULL_0000,
   __CP1250_0300,   __CP1250_0320,   __NULL_0000,   __CP1250_0360,
   __NULL_0000,   __CP1250_03A0,   __NULL_0000,   __NULL_0000,
};

CHAR __CP1250_04A0[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_0580[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x3A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_0660[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};


CHAR *__CP1250_04XX[32]=
{
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __CP1250_04A0, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __CP1250_0580, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __CP1250_0660,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
};

CHAR __CP1250_2000[32]=
{
   0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x2D, 0x2D, 0x00, 0x96, 0x97, 0x00, 0x00, 0x00, 
   0x91, 0x92, 0x82, 0x00, 0x93, 0x94, 0x84, 0x00, 
};

CHAR __CP1250_2020[32]=
{
   0x86, 0x87, 0x95, 0x00, 0x95, 0x00, 0x85, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x89, 0x00, 0x27, 0x94, 0x00, 0x60, 0x00, 0x00, 
   0x00, 0x8B, 0x9B, 0x00, 0x21, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_2040[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x2F, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_2060[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0xB0, 0x00, 0x00, 0x00, 0x34, 0x35, 0x36, 0x37, 
   0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_2080[32]=
{
   0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 
   0x38, 0x39, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_20A0[32]=
{
   0x00, 0x00, 0x00, 0x00, 0xA3, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_2100[32]=
{
   0x00, 0x00, 0x43, 0x00, 0x00, 0x00, 0x00, 0x45, 
   0x00, 0x00, 0x67, 0x48, 0x48, 0x48, 0x68, 0x00, 
   0x49, 0x49, 0x4C, 0x6C, 0x00, 0x4E, 0x00, 0x00, 
   0x50, 0x50, 0x51, 0x52, 0x52, 0x52, 0x00, 0x00, 
};

CHAR __CP1250_2120[32]=
{
   0x00, 0x00, 0x99, 0x00, 0x5A, 0x00, 0x00, 0x00, 
   0x5A, 0x00, 0x4B, 0xC5, 0x42, 0x43, 0x65, 0x65, 
   0x45, 0x46, 0x00, 0x4D, 0x6F, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_2180[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x8B, 0x5E, 0x9B, 0xA1, 0x2D, 0x7C, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_21A0[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_2200[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0xD8, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x2D, 0xB1, 0x00, 0x2F, 0x5C, 0x2A, 
   0xB0, 0x95, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4C, 
};

CHAR __CP1250_2220[32]=
{
   0x00, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3A, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_2260[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0xAB, 0xBB, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_22C0[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0xB7, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_2300[32]=
{
   0x00, 0x00, 0xA6, 0x5E, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_2320[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x3C, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};


CHAR *__CP1250_20XX[32]=
{
   __CP1250_2000, __CP1250_2020, __CP1250_2040, __CP1250_2060,
   __CP1250_2080, __CP1250_20A0, __NULL_0000, __NULL_0000,
   __CP1250_2100, __CP1250_2120, __NULL_0000, __NULL_0000,
   __CP1250_2180, __CP1250_21A0, __NULL_0000, __NULL_0000,
   __CP1250_2200, __CP1250_2220, __NULL_0000, __CP1250_2260,
   __NULL_0000, __NULL_0000, __CP1250_22C0, __NULL_0000,
   __CP1250_2300, __CP1250_2320, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
};

CHAR __CP1250_2500[32]=
{
   0xA6, 0x00, 0x2D, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x00, 0x00, 
   0xAC, 0x00, 0x00, 0x00, 0x4C, 0x00, 0x00, 0x00, 
   0x2D, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_2520[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_2540[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x3D, 0xA6, 0x00, 0x00, 0x2D, 0x00, 0x00, 0xAC, 
   0x00, 0x00, 0x4C, 0x00, 0x00, 0x2D, 0x00, 0x00, 
};

CHAR __CP1250_2560[32]=
{
   0xA6, 0x00, 0x00, 0xA6, 0x00, 0x00, 0x54, 0x00, 
   0x00, 0xA6, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_2580[32]=
{
   0x2D, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x00, 0x00, 
   0x2D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x2D, 0x2D, 0x2D, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_25A0[32]=
{
   0xA6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x5E, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x3E, 0x00, 0xA1, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_25C0[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x95, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_2620[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0xA2, 0xA2, 0x30, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_2640[32]=
{
   0x2B, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_2660[32]=
{
   0xA6, 0x00, 0x00, 0xA6, 0x00, 0xA6, 0xA6, 0x00, 
   0x00, 0x00, 0x64, 0x64, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1250_2740[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x7C, 0x00, 0x00, 0x91, 0x92, 0x93, 0x94, 0x00, 
};


CHAR *__CP1250_24XX[32]=
{
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __CP1250_2500, __CP1250_2520, __CP1250_2540, __CP1250_2560,
   __CP1250_2580, __CP1250_25A0, __CP1250_25C0, __NULL_0000,
   __NULL_0000, __CP1250_2620, __CP1250_2640, __CP1250_2660,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __CP1250_2740, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
};

CHAR __CP1250_3000[32]=
{
   0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x3C, 0x3E, 0xAB, 0xBB, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x5B, 0x5D, 0x00, 0x93, 0x94, 0x84, 
};

CHAR __CP1250_30E0[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0xB7, 0x97, 0x00, 0x00, 0x00, 
};


CHAR *__CP1250_30XX[32]=
{
   __CP1250_3000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __CP1250_30E0,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
};

CHAR __CP1250_FF00[32]=
{
   0x00, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 
   0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 
   0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 
   0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x00, 
};

CHAR __CP1250_FF20[32]=
{
   0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 
   0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 
   0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 
   0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 
};

CHAR __CP1250_FF40[32]=
{
   0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 
   0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 
   0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 
   0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x00, 
};


CHAR *__CP1250_FCXX[32]=
{
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __CP1250_FF00, __CP1250_FF20, __CP1250_FF40, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
};


CHAR **__CP1250_FromUnicode[64]=
{
   __CP1250_00XX, __CP1250_04XX, __NULL_00XX, __NULL_00XX,
   __NULL_00XX, __NULL_00XX, __NULL_00XX, __NULL_00XX,
   __CP1250_20XX, __CP1250_24XX, __NULL_00XX, __NULL_00XX,
   __CP1250_30XX, __NULL_00XX, __NULL_00XX, __NULL_00XX,
   __NULL_00XX, __NULL_00XX, __NULL_00XX, __NULL_00XX,
   __NULL_00XX, __NULL_00XX, __NULL_00XX, __NULL_00XX,
   __NULL_00XX, __NULL_00XX, __NULL_00XX, __NULL_00XX,
   __NULL_00XX, __NULL_00XX, __NULL_00XX, __NULL_00XX,
   __NULL_00XX, __NULL_00XX, __NULL_00XX, __NULL_00XX,
   __NULL_00XX, __NULL_00XX, __NULL_00XX, __NULL_00XX,
   __NULL_00XX, __NULL_00XX, __NULL_00XX, __NULL_00XX,
   __NULL_00XX, __NULL_00XX, __NULL_00XX, __NULL_00XX,
   __NULL_00XX, __NULL_00XX, __NULL_00XX, __NULL_00XX,
   __NULL_00XX, __NULL_00XX, __NULL_00XX, __NULL_00XX,
   __NULL_00XX, __NULL_00XX, __NULL_00XX, __NULL_00XX,
   __NULL_00XX, __NULL_00XX, __NULL_00XX, __CP1250_FCXX,
};
