/*
 * nls/cp1251.c
 * Copyright (C) 1996, Onno Hovers
 * 
 * This file has been generated by gencp.exe
 */

#include <windows.h>
#include <kernel32/nls.h>


WCHAR __CP1251_80[32]=
{
   0x0402, 0x0403, 0x201A, 0x0453, 0x201E, 0x2026, 0x2020, 0x2021, 
   0x0088, 0x2030, 0x0409, 0x2039, 0x040A, 0x040C, 0x040B, 0x040F, 
   0x0452, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 
   0x0098, 0x2122, 0x0459, 0x203A, 0x045A, 0x045C, 0x045B, 0x045F, 
};

WCHAR __CP1251_A0[32]=
{
   0x00A0, 0x040E, 0x045E, 0x0408, 0x00A4, 0x0490, 0x00A6, 0x00A7, 
   0x0401, 0x00A9, 0x0404, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x0407, 
   0x00B0, 0x00B1, 0x0406, 0x0456, 0x0491, 0x00B5, 0x00B6, 0x00B7, 
   0x0451, 0x2116, 0x0454, 0x00BB, 0x0458, 0x0405, 0x0455, 0x0457, 
};

WCHAR __CP1251_C0[32]=
{
   0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417, 
   0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F, 
   0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427, 
   0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F, 
};

WCHAR __CP1251_E0[32]=
{
   0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437, 
   0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F, 
   0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447, 
   0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F, 
};


WCHAR *__CP1251_ToUnicode[8]=
{
   __ASCII_00, __ASCII_20, __ASCII_40, __ASCII_60,
   __CP1251_80, __CP1251_A0, __CP1251_C0, __CP1251_E0
};


CHAR __CP1251_0080[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1251_00A0[32]=
{
   0xA0, 0x00, 0x00, 0x00, 0xA4, 0x00, 0xA6, 0xA7, 
   0x00, 0xA9, 0x00, 0xAB, 0xAC, 0xAD, 0xAE, 0x00, 
   0xB0, 0xB1, 0x00, 0x00, 0x00, 0xB5, 0xB6, 0xB7, 
   0x00, 0x00, 0x00, 0xBB, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1251_00C0[32]=
{
   0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x00, 0x43, 
   0x45, 0x45, 0x45, 0x45, 0x49, 0x49, 0x49, 0x49, 
   0x00, 0x4E, 0x4F, 0x4F, 0x4F, 0x4F, 0x4F, 0x00, 
   0x4F, 0x55, 0x55, 0x55, 0x55, 0x59, 0x00, 0x00, 
};

CHAR __CP1251_00E0[32]=
{
   0x61, 0x61, 0x61, 0x61, 0x61, 0x61, 0x00, 0x63, 
   0x65, 0x65, 0x65, 0x65, 0x69, 0x69, 0x69, 0x69, 
   0x00, 0x6E, 0x6F, 0x6F, 0x6F, 0x6F, 0x6F, 0x00, 
   0x6F, 0x75, 0x75, 0x75, 0x75, 0x79, 0x00, 0x79, 
};

CHAR __CP1251_0100[32]=
{
   0x41, 0x61, 0x41, 0x61, 0x41, 0x61, 0x43, 0x63, 
   0x43, 0x63, 0x43, 0x63, 0x43, 0x63, 0x44, 0x64, 
   0x44, 0x64, 0x45, 0x65, 0x45, 0x65, 0x45, 0x65, 
   0x45, 0x65, 0x45, 0x65, 0x47, 0x67, 0x47, 0x67, 
};

CHAR __CP1251_0120[32]=
{
   0x47, 0x67, 0x47, 0x67, 0x48, 0x68, 0x48, 0x68, 
   0x49, 0x69, 0x49, 0x69, 0x49, 0x69, 0x49, 0x69, 
   0x49, 0x00, 0x00, 0x00, 0x4A, 0x6A, 0x4B, 0x6B, 
   0x00, 0x4C, 0x6C, 0x4C, 0x6C, 0x4C, 0x6C, 0x00, 
};

CHAR __CP1251_0140[32]=
{
   0x00, 0x4C, 0x6C, 0x4E, 0x6E, 0x4E, 0x6E, 0x4E, 
   0x6E, 0x00, 0x00, 0x00, 0x4F, 0x6F, 0x4F, 0x6F, 
   0x4F, 0x6F, 0x00, 0x00, 0x52, 0x72, 0x52, 0x72, 
   0x52, 0x72, 0x53, 0x73, 0x53, 0x73, 0x53, 0x73, 
};

CHAR __CP1251_0160[32]=
{
   0x53, 0x73, 0x54, 0x74, 0x54, 0x74, 0x54, 0x74, 
   0x55, 0x75, 0x55, 0x75, 0x55, 0x75, 0x55, 0x75, 
   0x55, 0x75, 0x55, 0x75, 0x57, 0x77, 0x59, 0x79, 
   0x59, 0x5A, 0x7A, 0x5A, 0x7A, 0x5A, 0x7A, 0x00, 
};

CHAR __CP1251_0180[32]=
{
   0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49, 
   0x00, 0x00, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x4F, 
};

CHAR __CP1251_01A0[32]=
{
   0x4F, 0x6F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x54, 0x55, 
   0x75, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1251_01C0[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x61, 0x49, 
   0x69, 0x4F, 0x6F, 0x55, 0x75, 0x55, 0x75, 0x55, 
   0x75, 0x55, 0x75, 0x55, 0x75, 0x00, 0x41, 0x61, 
};

CHAR __CP1251_01E0[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x47, 0x67, 0x47, 0x67, 
   0x4B, 0x6B, 0x4F, 0x6F, 0x4F, 0x6F, 0x00, 0x00, 
   0x6A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};


CHAR *__CP1251_00XX[32]=
{
   __ASCII_0000, __ASCII_0020, __ASCII_0040, __ASCII_0060,
   __CP1251_0080,   __CP1251_00A0,   __CP1251_00C0,   __CP1251_00E0,
   __CP1251_0100,   __CP1251_0120,   __CP1251_0140,   __CP1251_0160,
   __CP1251_0180,   __CP1251_01A0,   __CP1251_01C0,   __CP1251_01E0,
   __NULL_0000,   __NULL_0000,   __NULL_0000,   __NULL_0000,
   __NULL_0000,   __NULL_0000,   __NULL_0000,   __NULL_0000,
   __NULL_0000,   __NULL_0000,   __NULL_0000,   __NULL_0000,
   __NULL_0000,   __NULL_0000,   __NULL_0000,   __NULL_0000,
};

CHAR __CP1251_0400[32]=
{
   0x00, 0xA8, 0x80, 0x81, 0xAA, 0xBD, 0xB2, 0xAF, 
   0xA3, 0x8A, 0x8C, 0x8E, 0x8D, 0x00, 0xA1, 0x8F, 
   0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 
   0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 
};

CHAR __CP1251_0420[32]=
{
   0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 
   0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 
   0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 
   0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF, 
};

CHAR __CP1251_0440[32]=
{
   0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 
   0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF, 
   0x00, 0xB8, 0x90, 0x83, 0xBA, 0xBE, 0xB3, 0xBF, 
   0xBC, 0x9A, 0x9C, 0x9E, 0x9D, 0x00, 0xA2, 0x9F, 
};

CHAR __CP1251_0480[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0xA5, 0xB4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};


CHAR *__CP1251_04XX[32]=
{
   __CP1251_0400, __CP1251_0420, __CP1251_0440, __NULL_0000,
   __CP1251_0480, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
};

CHAR __CP1251_2000[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x96, 0x97, 0x00, 0x00, 0x00, 
   0x91, 0x92, 0x82, 0x00, 0x93, 0x94, 0x84, 0x00, 
};

CHAR __CP1251_2020[32]=
{
   0x86, 0x87, 0x95, 0x00, 0x00, 0x00, 0x85, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x8B, 0x9B, 0x00, 0x21, 0x00, 0x00, 0x00, 
};

CHAR __CP1251_2100[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB9, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1251_2120[32]=
{
   0x00, 0x00, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1251_2180[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x3C, 0x5E, 0x3E, 0x76, 0x2D, 0xA6, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1251_21A0[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0xA6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1251_2200[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x95, 0x76, 0x00, 0x00, 0x00, 0x00, 0x4C, 
};

CHAR __CP1251_2300[32]=
{
   0x00, 0x00, 0xA6, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};


CHAR *__CP1251_20XX[32]=
{
   __CP1251_2000, __CP1251_2020, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __CP1251_2100, __CP1251_2120, __NULL_0000, __NULL_0000,
   __CP1251_2180, __CP1251_21A0, __NULL_0000, __NULL_0000,
   __CP1251_2200, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __CP1251_2300, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
};

CHAR __CP1251_2500[32]=
{
   0x2D, 0x00, 0xA6, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x00, 0x00, 
   0xAC, 0x00, 0x00, 0x00, 0x4C, 0x00, 0x00, 0x00, 
   0x2D, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00, 
};

CHAR __CP1251_2520[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x2B, 0x00, 0x00, 0x00, 
};

CHAR __CP1251_2540[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x3D, 0xA6, 0x2D, 0xE3, 0xE3, 0xAC, 0xAC, 0xAC, 
   0x4C, 0x4C, 0x4C, 0x2D, 0x2D, 0x2D, 0xA6, 0xA6, 
};

CHAR __CP1251_2560[32]=
{
   0xA6, 0xA6, 0xA6, 0xA6, 0x54, 0x54, 0x54, 0xA6, 
   0xA6, 0xA6, 0x2B, 0x2B, 0x2B, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1251_2580[32]=
{
   0x2D, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x00, 0x00, 
   0x2D, 0x00, 0x00, 0x00, 0xA6, 0x00, 0x00, 0x00, 
   0xA6, 0x2D, 0x2D, 0x2D, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1251_25A0[32]=
{
   0xA6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x5E, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x3E, 0x00, 0xA1, 0x00, 0x00, 0x00, 
};

CHAR __CP1251_25C0[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x95, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1251_2620[32]=
{
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x4F, 0x4F, 0x30, 0x00, 0x00, 0x00, 
};

CHAR __CP1251_2640[32]=
{
   0x2B, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};

CHAR __CP1251_2660[32]=
{
   0xA6, 0x00, 0x00, 0xA6, 0x00, 0xA6, 0xA6, 0x00, 
   0x00, 0x00, 0x64, 0x64, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};


CHAR *__CP1251_24XX[32]=
{
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __CP1251_2500, __CP1251_2520, __CP1251_2540, __CP1251_2560,
   __CP1251_2580, __CP1251_25A0, __CP1251_25C0, __NULL_0000,
   __NULL_0000, __CP1251_2620, __CP1251_2640, __CP1251_2660,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
};

CHAR __CP1251_FF00[32]=
{
   0x00, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 
   0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 
   0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 
   0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x00, 
};

CHAR __CP1251_FF20[32]=
{
   0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 
   0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 
   0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 
   0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 
};

CHAR __CP1251_FF40[32]=
{
   0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 
   0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 
   0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 
   0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x00, 
};


CHAR *__CP1251_FCXX[32]=
{
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
   __CP1251_FF00, __CP1251_FF20, __CP1251_FF40, __NULL_0000,
   __NULL_0000, __NULL_0000, __NULL_0000, __NULL_0000,
};


CHAR **__CP1251_FromUnicode[64]=
{
   __CP1251_00XX, __CP1251_04XX, __NULL_00XX, __NULL_00XX,
   __NULL_00XX, __NULL_00XX, __NULL_00XX, __NULL_00XX,
   __CP1251_20XX, __CP1251_24XX, __NULL_00XX, __NULL_00XX,
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
   __NULL_00XX, __NULL_00XX, __NULL_00XX, __NULL_00XX,
   __NULL_00XX, __NULL_00XX, __NULL_00XX, __CP1251_FCXX,
};
