/*
 * Implementation of Uniscribe Script Processor (usp10.dll)
 *
 * Copyright 2005 Steven Edwards for CodeWeavers
 * Copyright 2006 Hans Leidekker
 * Copyright 2010 CodeWeavers, Aric Stewart
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * Notes:
 * Uniscribe allows for processing of complex scripts such as joining
 * and filtering characters and bi-directional text with custom line breaks.
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "usp10.h"

#include "usp10_internal.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(uniscribe);

typedef struct _scriptRange
{
    WORD script;
    WORD rangeFirst;
    WORD rangeLast;
    WORD numericScript;
    WORD punctScript;
} scriptRange;

static const scriptRange scriptRanges[] = {
    /* Basic Latin: U+0000–U+007A */
    { Script_Latin,      0x00,   0x07a ,  Script_Numeric, Script_Punctuation},
    /* Latin-1 Supplement: U+0080–U+00FF */
    /* Latin Extended-A: U+0100–U+017F */
    /* Latin Extended-B: U+0180–U+024F */
    /* IPA Extensions: U+0250–U+02AF */
    { Script_Latin,      0x80,   0x2af ,  Script_Numeric2, Script_Punctuation},
    /* Combining Diacritical Marks : U+0300–U+036F */
    { Script_Diacritical,0x300,  0x36f,  0, 0},
    /* Greek: U+0370–U+03FF */
    { Script_Greek,      0x370,  0x3ff,  0, 0},
    /* Cyrillic: U+0400–U+04FF */
    /* Cyrillic Supplement: U+0500–U+052F */
    { Script_Cyrillic,   0x400,  0x52f,  0, 0},
    /* Armenian: U+0530–U+058F */
    { Script_Armenian,   0x530,  0x58f,  0, 0},
    /* Hebrew: U+0590–U+05FF */
    { Script_Hebrew,     0x590,  0x5ff,  0, 0},
    /* Arabic: U+0600–U+06FF */
    { Script_Arabic,     0x600,  0x6ef,  Script_Arabic_Numeric, 0},
    /* Defined by Windows */
    { Script_Persian,    0x6f0,  0x6f9,  0, 0},
    /* Continue Arabic: U+0600–U+06FF */
    { Script_Arabic,     0x6fa,  0x6ff,  0, 0},
    /* Syriac: U+0700–U+074F*/
    { Script_Syriac,     0x700,  0x74f,  0, 0},
    /* Arabic Supplement: U+0750–U+077F */
    { Script_Arabic,     0x750,  0x77f,  0, 0},
    /* Thaana: U+0780–U+07BF */
    { Script_Thaana,     0x780,  0x7bf,  0, 0},
    /* Devanagari: U+0900–U+097F */
    { Script_Devanagari, 0x900,  0x97f,  Script_Devanagari_Numeric, 0},
    /* Bengali: U+0980–U+09FF */
    { Script_Bengali,    0x980,  0x9ff,  Script_Bengali_Numeric, 0},
    /* Gurmukhi: U+0A00–U+0A7F*/
    { Script_Gurmukhi,   0xa00,  0xa7f,  Script_Gurmukhi_Numeric, 0},
    /* Gujarati: U+0A80–U+0AFF*/
    { Script_Gujarati,   0xa80,  0xaff,  Script_Gujarati_Numeric, 0},
    /* Oriya: U+0B00–U+0B7F */
    { Script_Oriya,      0xb00,  0xb7f,  Script_Oriya_Numeric, 0},
    /* Tamil: U+0B80–U+0BFF */
    { Script_Tamil,      0xb80,  0xbff,  Script_Tamil_Numeric, 0},
    /* Telugu: U+0C00–U+0C7F */
    { Script_Telugu,     0xc00,  0xc7f,  Script_Telugu_Numeric, 0},
    /* Kannada: U+0C80–U+0CFF */
    { Script_Kannada,    0xc80,  0xcff,  Script_Kannada_Numeric, 0},
    /* Malayalam: U+0D00–U+0D7F */
    { Script_Malayalam,  0xd00,  0xd7f,  Script_Malayalam_Numeric, 0},
    /* Sinhala: U+0D80–U+0DFF */
    { Script_Sinhala,   0xd80,  0xdff,  0, 0},
    /* Thai: U+0E00–U+0E7F */
    { Script_Thai,      0xe00,  0xe7f,  Script_Thai_Numeric, 0},
    /* Lao: U+0E80–U+0EFF */
    { Script_Lao,       0xe80,  0xeff,  Script_Lao_Numeric, 0},
    /* Tibetan: U+0F00–U+0FFF */
    { Script_Tibetan,   0xf00,  0xfff,  0, 0},
    /* Georgian: U+10A0–U+10FF */
    { Script_Georgian,   0x10a0,  0x10ff,  0, 0},
    /* Vedic Extensions: U+1CD0-U+1CFF */
    { Script_Devanagari, 0x1cd0, 0x1cff, Script_Devanagari_Numeric, 0},
    /* Phonetic Extensions: U+1D00–U+1DBF */
    { Script_Latin,      0x1d00, 0x1dbf, 0, 0},
    /* Combining Diacritical Marks Supplement: U+1DC0–U+1DFF */
    { Script_Diacritical,0x1dc0, 0x1dff, 0, 0},
    /* Latin Extended Additional: U+1E00–U+1EFF */
    { Script_Latin,      0x1e00, 0x1eff, 0, 0},
    /* Greek Extended: U+1F00–U+1FFF */
    { Script_Greek,      0x1f00, 0x1fff, 0, 0},
    /* General Punctuation: U+2000 –U+206f */
    { Script_Latin,      0x2000, 0x206f, 0, 0},
    /* Superscripts and Subscripts : U+2070 –U+209f */
    /* Currency Symbols : U+20a0 –U+20cf */
    { Script_Numeric2,   0x2070, 0x2070, 0, 0},
    { Script_Latin,      0x2071, 0x2073, 0, 0},
    { Script_Numeric2,   0x2074, 0x2079, 0, 0},
    { Script_Latin,      0x207a, 0x207f, 0, 0},
    { Script_Numeric2,   0x2080, 0x2089, 0, 0},
    { Script_Latin,      0x208a, 0x20cf, 0, 0},
    /* Letterlike Symbols : U+2100 –U+214f */
    /* Number Forms : U+2150 –U+218f */
    /* Arrows : U+2190 –U+21ff */
    /* Mathematical Operators : U+2200 –U+22ff */
    /* Miscellaneous Technical : U+2300 –U+23ff */
    /* Control Pictures : U+2400 –U+243f */
    /* Optical Character Recognition : U+2440 –U+245f */
    /* Enclosed Alphanumerics : U+2460 –U+24ff */
    /* Box Drawing : U+2500 –U+25ff */
    /* Block Elements : U+2580 –U+259f */
    /* Geometric Shapes : U+25a0 –U+25ff */
    /* Miscellaneous Symbols : U+2600 –U+26ff */
    /* Dingbats : U+2700 –U+27bf */
    /* Miscellaneous Mathematical Symbols-A : U+27c0 –U+27ef */
    /* Supplemental Arrows-A : U+27f0 –U+27ff */
    { Script_Latin,      0x2100, 0x27ff, 0, 0},
    /* Supplemental Arrows-B : U+2900 –U+297f */
    /* Miscellaneous Mathematical Symbols-B : U+2980 –U+29ff */
    /* Supplemental Mathematical Operators : U+2a00 –U+2aff */
    /* Miscellaneous Symbols and Arrows : U+2b00 –U+2bff */
    { Script_Latin,      0x2900, 0x2bff, 0, 0},
    /* Latin Extended-C: U+2C60–U+2C7F */
    { Script_Latin,      0x2c60, 0x2c7f, 0, 0},
    /* Georgian: U+2D00–U+2D2F */
    { Script_Georgian,   0x2d00,  0x2d2f,  0, 0},
    /* Cyrillic Extended-A: U+2DE0–U+2DFF */
    { Script_Cyrillic,   0x2de0, 0x2dff,  0, 0},
    /* Cyrillic Extended-B: U+A640–U+A69F */
    { Script_Cyrillic,   0xa640, 0xa69f,  0, 0},
    /* Modifier Tone Letters: U+A700–U+A71F */
    /* Latin Extended-D: U+A720–U+A7FF */
    { Script_Latin,      0xa700, 0xa7ff, 0, 0},
    /* Phags-pa: U+A840–U+A87F */
    { Script_Phags_pa,   0xa840, 0xa87f, 0, 0},
    /* Devanagari Extended: U+A8E0-U+A8FF */
    { Script_Devanagari, 0xa8e0, 0xa8ff, Script_Devanagari_Numeric, 0},
    /* Latin Ligatures: U+FB00–U+FB06 */
    { Script_Latin,      0xfb00, 0xfb06, 0, 0},
    /* Armenian ligatures U+FB13..U+FB17 */
    { Script_Armenian,   0xfb13, 0xfb17,  0, 0},
    /* Alphabetic Presentation Forms: U+FB1D–U+FB4F */
    { Script_Hebrew,     0xfb1d, 0xfb4f, 0, 0},
    /* Arabic Presentation Forms-A: U+FB50–U+FDFF*/
    { Script_Arabic,     0xfb50, 0xfdff, 0, 0},
    /* Arabic Presentation Forms-B: U+FE70–U+FEFF*/
    { Script_Arabic,     0xfe70, 0xfeff, 0, 0},
    /* END */
    { SCRIPT_UNDEFINED,  0, 0, 0}
};

typedef struct _scriptData
{
    SCRIPT_ANALYSIS a;
    SCRIPT_PROPERTIES props;
    OPENTYPE_TAG scriptTag;
    WCHAR fallbackFont[LF_FACESIZE];
} scriptData;

/* the must be in order so that the index matches the Script value */
static const scriptData scriptInformation[] = {
    {{SCRIPT_UNDEFINED, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_NEUTRAL, 0, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     0x00000000,
     {0}},
    {{Script_Latin, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_ENGLISH, 0, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, 0, 0, 1, 0, 0},
     MS_MAKE_TAG('l','a','t','n'),
     {0}},
    {{Script_CR, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_NEUTRAL, 0, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     0x00000000,
     {0}},
    {{Script_Numeric, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_ENGLISH, 1, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     0x00000000,
     {0}},
    {{Script_Control, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_ENGLISH, 0, 1, 0, 0, ANSI_CHARSET, 1, 0, 0, 0, 0, 0, 1, 0, 0},
     0x00000000,
     {0}},
    {{Script_Punctuation, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_NEUTRAL, 0, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     0x00000000,
     {0}},
    {{Script_Arabic, 1, 1, 0, 0, 0, 0, { 1,0,0,0,0,0,0,0,0,0,0}},
     {LANG_ARABIC, 0, 1, 0, 0, ARABIC_CHARSET, 0, 0, 0, 0, 0, 0, 1, 1, 0},
     MS_MAKE_TAG('a','r','a','b'),
     {'M','i','c','r','o','s','o','f','t',' ','S','a','n','s',' ','S','e','r','i','f',0}},
    {{Script_Arabic_Numeric, 1, 1, 0, 0, 0, 0, { 1,0,0,0,0,0,0,0,0,0,0}},
     {LANG_ARABIC, 1, 1, 0, 0, ARABIC_CHARSET, 0, 0, 0, 0, 0, 0, 1, 0, 0},
     MS_MAKE_TAG('a','r','a','b'),
     {'M','i','c','r','o','s','o','f','t',' ','S','a','n','s',' ','S','e','r','i','f',0}},
    {{Script_Hebrew, 1, 1, 0, 0, 0, 0, { 1,0,0,0,0,0,0,0,0,0,0}},
     {LANG_HEBREW, 0, 1, 0, 1, HEBREW_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     MS_MAKE_TAG('h','e','b','r'),
     {'M','i','c','r','o','s','o','f','t',' ','S','a','n','s',' ','S','e','r','i','f',0}},
    {{Script_Syriac, 1, 1, 0, 0, 0, 0, { 1,0,0,0,0,0,0,0,0,0,0}},
     {LANG_SYRIAC, 0, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, 1, 0, 0, 1, 0},
     MS_MAKE_TAG('s','y','r','c'),
     {'E','s','t','r','a','n','g','e','l','o',' ','E','d','e','s','s','a',0}},
    {{Script_Persian, 1, 1, 0, 0, 0, 0, { 1,0,0,0,0,0,0,0,0,0,0}},
     {LANG_PERSIAN, 1, 1, 0, 0, ARABIC_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     MS_MAKE_TAG('s','y','r','c'),
     {'E','s','t','r','a','n','g','e','l','o',' ','E','d','e','s','s','a',0}},
    {{Script_Thaana, 1, 1, 0, 0, 0, 0, { 1,0,0,0,0,0,0,0,0,0,0}},
     {LANG_DIVEHI, 0, 1, 0, 1, DEFAULT_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     MS_MAKE_TAG('t','h','a','a'),
     {'M','V',' ','B','o','l','i',0}},
    {{Script_Greek, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_GREEK, 0, 0, 0, 0, GREEK_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     MS_MAKE_TAG('g','r','e','k'),
     {0}},
    {{Script_Cyrillic, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_RUSSIAN, 0, 0, 0, 0, RUSSIAN_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     MS_MAKE_TAG('c','y','r','l'),
     {0}},
    {{Script_Armenian, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_ARMENIAN, 0, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, 0, 0, 1, 0, 0},
     MS_MAKE_TAG('a','r','m','n'),
     {'S','y','l','f','a','e','n',0}},
    {{Script_Georgian, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_GEORGIAN, 0, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, 0, 0, 1, 0, 0},
     MS_MAKE_TAG('g','e','o','r'),
     {'S','y','l','f','a','e','n',0}},
    {{Script_Sinhala, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_SINHALESE, 0, 1, 0, 1, DEFAULT_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     MS_MAKE_TAG('s','i','n','h'),
     {'I','s','k','o','o','l','a',' ','P','o','t','a',0}},
    {{Script_Tibetan, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_TIBETAN, 0, 1, 1, 1, DEFAULT_CHARSET, 0, 0, 1, 0, 1, 0, 0, 0, 0},
     MS_MAKE_TAG('t','i','b','t'),
     {'M','i','c','r','o','s','o','f','t',' ','H','i','m','a','l','a','y','a',0}},
    {{Script_Tibetan_Numeric, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_TIBETAN, 1, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     MS_MAKE_TAG('t','i','b','t'),
     {'M','i','c','r','o','s','o','f','t',' ','H','i','m','a','l','a','y','a',0}},
    {{Script_Phags_pa, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_MONGOLIAN, 0, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     MS_MAKE_TAG('p','h','a','g'),
     {'M','i','c','r','o','s','o','f','t',' ','P','h','a','g','s','P','a',0}},
    {{Script_Thai, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_THAI, 0, 1, 1, 1, THAI_CHARSET, 0, 0, 1, 0, 1, 0, 0, 0, 1},
     MS_MAKE_TAG('t','h','a','i'),
     {'M','i','c','r','o','s','o','f','t',' ','S','a','n','s',' ','S','e','r','i','f',0}},
    {{Script_Thai_Numeric, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_THAI, 1, 1, 0, 0, THAI_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     MS_MAKE_TAG('t','h','a','i'),
     {'M','i','c','r','o','s','o','f','t',' ','S','a','n','s',' ','S','e','r','i','f',0}},
    {{Script_Lao, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_LAO, 0, 1, 1, 1, DEFAULT_CHARSET, 0, 0, 1, 0, 1, 0, 0, 0, 0},
     MS_MAKE_TAG('l','a','o',' '),
     {'D','o','k','C','h','a','m','p','a',0}},
    {{Script_Lao_Numeric, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_LAO, 1, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     MS_MAKE_TAG('l','a','o',' '),
     {'D','o','k','C','h','a','m','p','a',0}},
    {{Script_Devanagari, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_HINDI, 0, 1, 0, 1, DEFAULT_CHARSET, 0, 0, 0, 0, 1, 0, 0, 0, 0},
     MS_MAKE_TAG('d','e','v','a'),
     {'M','a','n','g','a','l',0}},
    {{Script_Devanagari_Numeric, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_HINDI, 1, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     MS_MAKE_TAG('d','e','v','a'),
     {'M','a','n','g','a','l',0}},
    {{Script_Bengali, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_BENGALI, 0, 1, 0, 1, DEFAULT_CHARSET, 0, 0, 0, 0, 1, 0, 0, 0, 0},
     MS_MAKE_TAG('b','e','n','g'),
     {'V','r','i','n','d','a',0}},
    {{Script_Bengali_Numeric, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_BENGALI, 1, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     MS_MAKE_TAG('b','e','n','g'),
     {'V','r','i','n','d','a',0}},
    {{Script_Bengali_Currency, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_BENGALI, 0, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     MS_MAKE_TAG('b','e','n','g'),
     {'V','r','i','n','d','a',0}},
    {{Script_Gurmukhi, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_PUNJABI, 0, 1, 0, 1, DEFAULT_CHARSET, 0, 0, 0, 0, 1, 0, 0, 0, 0},
     MS_MAKE_TAG('g','u','r','u'),
     {'R','a','a','v','i',0}},
    {{Script_Gurmukhi_Numeric, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_PUNJABI, 1, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     MS_MAKE_TAG('g','u','r','u'),
     {'R','a','a','v','i',0}},
    {{Script_Gujarati, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_GUJARATI, 0, 1, 0, 1, DEFAULT_CHARSET, 0, 0, 0, 0, 1, 0, 0, 0, 0},
     MS_MAKE_TAG('g','u','j','r'),
     {'S','h','r','u','t','i',0}},
    {{Script_Gujarati_Numeric, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_GUJARATI, 1, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     MS_MAKE_TAG('g','u','j','r'),
     {'S','h','r','u','t','i',0}},
    {{Script_Gujarati_Currency, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_GUJARATI, 0, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     MS_MAKE_TAG('g','u','j','r'),
     {'S','h','r','u','t','i',0}},
    {{Script_Oriya, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_ORIYA, 0, 1, 0, 1, DEFAULT_CHARSET, 0, 0, 0, 0, 1, 0, 0, 0, 0},
     MS_MAKE_TAG('o','r','y','a'),
     {'K','a','l','i','n','g','a',0}},
    {{Script_Oriya_Numeric, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_ORIYA, 1, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     MS_MAKE_TAG('o','r','y','a'),
     {'K','a','l','i','n','g','a',0}},
    {{Script_Tamil, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_TAMIL, 0, 1, 0, 1, DEFAULT_CHARSET, 0, 0, 0, 0, 1, 0, 0, 0, 0},
     MS_MAKE_TAG('t','a','m','l'),
     {'L','a','t','h','a',0}},
    {{Script_Tamil_Numeric, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_TAMIL, 1, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     MS_MAKE_TAG('t','a','m','l'),
     {'L','a','t','h','a',0}},
    {{Script_Telugu, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_TELUGU, 0, 1, 0, 1, DEFAULT_CHARSET, 0, 0, 0, 0, 1, 0, 0, 0, 0},
     MS_MAKE_TAG('t','e','l','u'),
     {'G','a','u','t','a','m','i',0}},
    {{Script_Telugu_Numeric, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_TELUGU, 1, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     MS_MAKE_TAG('t','e','l','u'),
     {'G','a','u','t','a','m','i',0}},
    {{Script_Kannada, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_KANNADA, 0, 1, 0, 1, DEFAULT_CHARSET, 0, 0, 0, 0, 1, 0, 0, 0, 0},
     MS_MAKE_TAG('k','n','d','a'),
     {'T','u','n','g','a',0}},
    {{Script_Kannada_Numeric, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_KANNADA, 1, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     MS_MAKE_TAG('k','n','d','a'),
     {'T','u','n','g','a',0}},
    {{Script_Malayalam, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_MALAYALAM, 0, 1, 0, 1, DEFAULT_CHARSET, 0, 0, 0, 0, 1, 0, 0, 0, 0},
     MS_MAKE_TAG('m','l','y','m'),
     {'K','a','r','t','i','k','a',0}},
    {{Script_Malayalam_Numeric, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_MALAYALAM, 1, 1, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     MS_MAKE_TAG('m','l','y','m'),
     {'K','a','r','t','i','k','a',0}},
    {{Script_Diacritical, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_ENGLISH, 0, 1, 0, 1, ANSI_CHARSET, 0, 0, 0, 0, 0, 1, 1, 0, 0},
     0x00000000,
     {0}},
    {{Script_Punctuation2, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_ENGLISH, 0, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     MS_MAKE_TAG('l','a','t','n'),
     {0}},
    {{Script_Numeric2, 0, 0, 0, 0, 0, 0, { 0,0,0,0,0,0,0,0,0,0,0}},
     {LANG_ENGLISH, 1, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, 0, 0, 1, 0, 0},
     0x00000000,
     {0}},
};

static const SCRIPT_PROPERTIES *script_props[] =
{
    &scriptInformation[0].props, &scriptInformation[1].props,
    &scriptInformation[2].props, &scriptInformation[3].props,
    &scriptInformation[4].props, &scriptInformation[5].props,
    &scriptInformation[6].props, &scriptInformation[7].props,
    &scriptInformation[8].props, &scriptInformation[9].props,
    &scriptInformation[10].props, &scriptInformation[11].props,
    &scriptInformation[12].props, &scriptInformation[13].props,
    &scriptInformation[14].props, &scriptInformation[15].props,
    &scriptInformation[16].props, &scriptInformation[17].props,
    &scriptInformation[18].props, &scriptInformation[19].props,
    &scriptInformation[20].props, &scriptInformation[21].props,
    &scriptInformation[22].props, &scriptInformation[23].props,
    &scriptInformation[24].props, &scriptInformation[25].props,
    &scriptInformation[26].props, &scriptInformation[27].props,
    &scriptInformation[28].props, &scriptInformation[29].props,
    &scriptInformation[30].props, &scriptInformation[31].props,
    &scriptInformation[32].props, &scriptInformation[33].props,
    &scriptInformation[34].props, &scriptInformation[35].props,
    &scriptInformation[36].props, &scriptInformation[37].props,
    &scriptInformation[38].props, &scriptInformation[39].props,
    &scriptInformation[40].props, &scriptInformation[41].props,
    &scriptInformation[42].props, &scriptInformation[43].props,
    &scriptInformation[44].props, &scriptInformation[45].props
};

typedef struct {
    ScriptCache *sc;
    int numGlyphs;
    WORD* glyphs;
    WORD* pwLogClust;
    int* piAdvance;
    SCRIPT_VISATTR* psva;
    GOFFSET* pGoffset;
    ABC* abc;
    int iMaxPosX;
    HFONT fallbackFont;
} StringGlyphs;

typedef struct {
    HDC hdc;
    DWORD dwFlags;
    BOOL invalid;
    int clip_len;
    int cItems;
    int cMaxGlyphs;
    SCRIPT_ITEM* pItem;
    int numItems;
    StringGlyphs* glyphs;
    SCRIPT_LOGATTR* logattrs;
    SIZE* sz;
    int* logical2visual;
} StringAnalysis;

static inline void *heap_alloc(SIZE_T size)
{
    return HeapAlloc(GetProcessHeap(), 0, size);
}

static inline void *heap_alloc_zero(SIZE_T size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

static inline void *heap_realloc_zero(LPVOID mem, SIZE_T size)
{
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, mem, size);
}

static inline BOOL heap_free(LPVOID mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

static inline WCHAR get_cache_default_char(SCRIPT_CACHE *psc)
{
    return ((ScriptCache *)*psc)->tm.tmDefaultChar;
}

static inline LONG get_cache_height(SCRIPT_CACHE *psc)
{
    return ((ScriptCache *)*psc)->tm.tmHeight;
}

static inline BYTE get_cache_pitch_family(SCRIPT_CACHE *psc)
{
    return ((ScriptCache *)*psc)->tm.tmPitchAndFamily;
}

static inline WORD get_cache_glyph(SCRIPT_CACHE *psc, WCHAR c)
{
    WORD *block = ((ScriptCache *)*psc)->glyphs[c >> GLYPH_BLOCK_SHIFT];

    if (!block) return 0;
    return block[c & GLYPH_BLOCK_MASK];
}

static inline WORD set_cache_glyph(SCRIPT_CACHE *psc, WCHAR c, WORD glyph)
{
    WORD **block = &((ScriptCache *)*psc)->glyphs[c >> GLYPH_BLOCK_SHIFT];

    if (!*block && !(*block = heap_alloc_zero(sizeof(WORD) * GLYPH_BLOCK_SIZE))) return 0;
    return ((*block)[c & GLYPH_BLOCK_MASK] = glyph);
}

static inline BOOL get_cache_glyph_widths(SCRIPT_CACHE *psc, WORD glyph, ABC *abc)
{
    static const ABC nil;
    ABC *block = ((ScriptCache *)*psc)->widths[glyph >> GLYPH_BLOCK_SHIFT];

    if (!block || !memcmp(&block[glyph & GLYPH_BLOCK_MASK], &nil, sizeof(ABC))) return FALSE;
    memcpy(abc, &block[glyph & GLYPH_BLOCK_MASK], sizeof(ABC));
    return TRUE;
}

static inline BOOL set_cache_glyph_widths(SCRIPT_CACHE *psc, WORD glyph, ABC *abc)
{
    ABC **block = &((ScriptCache *)*psc)->widths[glyph >> GLYPH_BLOCK_SHIFT];

    if (!*block && !(*block = heap_alloc_zero(sizeof(ABC) * GLYPH_BLOCK_SIZE))) return FALSE;
    memcpy(&(*block)[glyph & GLYPH_BLOCK_MASK], abc, sizeof(ABC));
    return TRUE;
}

static HRESULT init_script_cache(const HDC hdc, SCRIPT_CACHE *psc)
{
    ScriptCache *sc;

    if (!psc) return E_INVALIDARG;
    if (*psc) return S_OK;
    if (!hdc) return E_PENDING;

    if (!(sc = heap_alloc_zero(sizeof(ScriptCache)))) return E_OUTOFMEMORY;
    if (!GetTextMetricsW(hdc, &sc->tm))
    {
        heap_free(sc);
        return E_INVALIDARG;
    }
    if (!GetObjectW(GetCurrentObject(hdc, OBJ_FONT), sizeof(LOGFONTW), &sc->lf))
    {
        heap_free(sc);
        return E_INVALIDARG;
    }
    sc->sfnt = (GetFontData(hdc, MS_MAKE_TAG('h','e','a','d'), 0, NULL, 0)!=GDI_ERROR);
    *psc = sc;
    TRACE("<- %p\n", sc);
    return S_OK;
}

static WCHAR mirror_char( WCHAR ch )
{
    extern const WCHAR wine_mirror_map[];
    return ch + wine_mirror_map[wine_mirror_map[ch >> 8] + (ch & 0xff)];
}

static WORD get_char_script( WCHAR ch)
{
    static const WCHAR latin_punc[] = {'#','$','&','\'',',',';','<','>','?','@','\\','^','_','`','{','|','}','~', 0x00a0, 0};
    WORD type = 0;
    int i;

    if (ch == 0xc || ch == 0x20 || ch == 0x202f)
        return Script_CR;

    /* These punctuation are separated out as Latin punctuation */
    if (strchrW(latin_punc,ch))
        return Script_Punctuation2;

    /* These chars are itemized as Punctuation by Windows */
    if (ch == 0x2212 || ch == 0x2044)
        return Script_Punctuation;

    GetStringTypeW(CT_CTYPE1, &ch, 1, &type);

    if (type == 0)
        return SCRIPT_UNDEFINED;

    if (type & C1_CNTRL)
        return Script_Control;

    i = 0;
    do
    {
        if (ch < scriptRanges[i].rangeFirst || scriptRanges[i].script == SCRIPT_UNDEFINED)
            break;

        if (ch >= scriptRanges[i].rangeFirst && ch <= scriptRanges[i].rangeLast)
        {
            if (scriptRanges[i].numericScript && type & C1_DIGIT)
                return scriptRanges[i].numericScript;
            if (scriptRanges[i].punctScript && type & C1_PUNCT)
                return scriptRanges[i].punctScript;
            return scriptRanges[i].script;
        }
        i++;
    } while (1);

    return SCRIPT_UNDEFINED;
}

/***********************************************************************
 *      DllMain
 *
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

/***********************************************************************
 *      ScriptFreeCache (USP10.@)
 *
 * Free a script cache.
 *
 * PARAMS
 *   psc [I/O] Script cache.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptFreeCache(SCRIPT_CACHE *psc)
{
    TRACE("%p\n", psc);

    if (psc && *psc)
    {
        unsigned int i;
        for (i = 0; i < GLYPH_MAX / GLYPH_BLOCK_SIZE; i++)
        {
            heap_free(((ScriptCache *)*psc)->glyphs[i]);
            heap_free(((ScriptCache *)*psc)->widths[i]);
        }
        heap_free(((ScriptCache *)*psc)->GSUB_Table);
        heap_free(((ScriptCache *)*psc)->GDEF_Table);
        heap_free(((ScriptCache *)*psc)->features);
        heap_free(*psc);
        *psc = NULL;
    }
    return S_OK;
}

/***********************************************************************
 *      ScriptGetProperties (USP10.@)
 *
 * Retrieve a list of script properties.
 *
 * PARAMS
 *  props [I] Pointer to an array of SCRIPT_PROPERTIES pointers.
 *  num   [I] Pointer to the number of scripts.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 *
 * NOTES
 *  Behaviour matches WinXP.
 */
HRESULT WINAPI ScriptGetProperties(const SCRIPT_PROPERTIES ***props, int *num)
{
    TRACE("(%p,%p)\n", props, num);

    if (!props && !num) return E_INVALIDARG;

    if (num) *num = sizeof(script_props)/sizeof(script_props[0]);
    if (props) *props = script_props;

    return S_OK;
}

/***********************************************************************
 *      ScriptGetFontProperties (USP10.@)
 *
 * Get information on special glyphs.
 *
 * PARAMS
 *  hdc [I]   Device context.
 *  psc [I/O] Opaque pointer to a script cache.
 *  sfp [O]   Font properties structure.
 */
HRESULT WINAPI ScriptGetFontProperties(HDC hdc, SCRIPT_CACHE *psc, SCRIPT_FONTPROPERTIES *sfp)
{
    HRESULT hr;

    TRACE("%p,%p,%p\n", hdc, psc, sfp);

    if (!sfp) return E_INVALIDARG;
    if ((hr = init_script_cache(hdc, psc)) != S_OK) return hr;

    if (sfp->cBytes != sizeof(SCRIPT_FONTPROPERTIES))
        return E_INVALIDARG;

    /* return something sensible? */
    sfp->wgBlank = 0;
    sfp->wgDefault = get_cache_default_char(psc);
    sfp->wgInvalid = 0;
    sfp->wgKashida = 0xffff;
    sfp->iKashidaWidth = 0;

    return S_OK;
}

/***********************************************************************
 *      ScriptRecordDigitSubstitution (USP10.@)
 *
 *  Record digit substitution settings for a given locale.
 *
 *  PARAMS
 *   locale [I] Locale identifier.
 *   sds    [I] Structure to record substitution settings.
 *
 *  RETURNS
 *   Success: S_OK
 *   Failure: E_POINTER if sds is NULL, E_INVALIDARG otherwise.
 *
 *  SEE ALSO
 *   http://blogs.msdn.com/michkap/archive/2006/02/22/536877.aspx
 */
HRESULT WINAPI ScriptRecordDigitSubstitution(LCID locale, SCRIPT_DIGITSUBSTITUTE *sds)
{
    DWORD plgid, sub;

    TRACE("0x%x, %p\n", locale, sds);

    /* This implementation appears to be correct for all languages, but it's
     * not clear if sds->DigitSubstitute is ever set to anything except 
     * CONTEXT or NONE in reality */

    if (!sds) return E_POINTER;

    locale = ConvertDefaultLocale(locale);

    if (!IsValidLocale(locale, LCID_INSTALLED))
        return E_INVALIDARG;

    plgid = PRIMARYLANGID(LANGIDFROMLCID(locale));
    sds->TraditionalDigitLanguage = plgid;

    if (plgid == LANG_ARABIC || plgid == LANG_FARSI)
        sds->NationalDigitLanguage = plgid;
    else
        sds->NationalDigitLanguage = LANG_ENGLISH;

    if (!GetLocaleInfoW(locale, LOCALE_IDIGITSUBSTITUTION | LOCALE_RETURN_NUMBER,
                        (LPWSTR)&sub, sizeof(sub)/sizeof(WCHAR))) return E_INVALIDARG;

    switch (sub)
    {
    case 0: 
        if (plgid == LANG_ARABIC || plgid == LANG_FARSI)
            sds->DigitSubstitute = SCRIPT_DIGITSUBSTITUTE_CONTEXT;
        else
            sds->DigitSubstitute = SCRIPT_DIGITSUBSTITUTE_NONE;
        break;
    case 1:
        sds->DigitSubstitute = SCRIPT_DIGITSUBSTITUTE_NONE;
        break;
    case 2:
        sds->DigitSubstitute = SCRIPT_DIGITSUBSTITUTE_NATIONAL;
        break;
    default:
        sds->DigitSubstitute = SCRIPT_DIGITSUBSTITUTE_TRADITIONAL;
        break;
    }

    sds->dwReserved = 0;
    return S_OK;
}

/***********************************************************************
 *      ScriptApplyDigitSubstitution (USP10.@)
 *
 *  Apply digit substitution settings.
 *
 *  PARAMS
 *   sds [I] Structure with recorded substitution settings.
 *   sc  [I] Script control structure.
 *   ss  [I] Script state structure.
 *
 *  RETURNS
 *   Success: S_OK
 *   Failure: E_INVALIDARG if sds is invalid. Otherwise an HRESULT.
 */
HRESULT WINAPI ScriptApplyDigitSubstitution(const SCRIPT_DIGITSUBSTITUTE *sds, 
                                            SCRIPT_CONTROL *sc, SCRIPT_STATE *ss)
{
    SCRIPT_DIGITSUBSTITUTE psds;

    TRACE("%p, %p, %p\n", sds, sc, ss);

    if (!sc || !ss) return E_POINTER;
    if (!sds)
    {
        sds = &psds;
        if (ScriptRecordDigitSubstitution(LOCALE_USER_DEFAULT, &psds) != S_OK)
            return E_INVALIDARG;
    }

    sc->uDefaultLanguage = LANG_ENGLISH;
    sc->fContextDigits = 0;
    ss->fDigitSubstitute = 0;

    switch (sds->DigitSubstitute) {
        case SCRIPT_DIGITSUBSTITUTE_CONTEXT:
        case SCRIPT_DIGITSUBSTITUTE_NATIONAL:
        case SCRIPT_DIGITSUBSTITUTE_NONE:
        case SCRIPT_DIGITSUBSTITUTE_TRADITIONAL:
            return S_OK;
        default:
            return E_INVALIDARG;
    }
}

static inline BOOL is_indic(WORD script)
{
    return (script >= Script_Devanagari && script <= Script_Malayalam_Numeric);
}

static inline WORD base_indic(WORD script)
{
    switch (script)
    {
        case Script_Devanagari:
        case Script_Devanagari_Numeric: return Script_Devanagari;
        case Script_Bengali:
        case Script_Bengali_Numeric:
        case Script_Bengali_Currency: return Script_Bengali;
        case Script_Gurmukhi:
        case Script_Gurmukhi_Numeric: return Script_Gurmukhi;
        case Script_Gujarati:
        case Script_Gujarati_Numeric:
        case Script_Gujarati_Currency: return Script_Gujarati;
        case Script_Oriya:
        case Script_Oriya_Numeric: return Script_Oriya;
        case Script_Tamil:
        case Script_Tamil_Numeric: return Script_Tamil;
        case Script_Telugu:
        case Script_Telugu_Numeric: return Script_Telugu;
        case Script_Kannada:
        case Script_Kannada_Numeric: return Script_Kannada;
        case Script_Malayalam:
        case Script_Malayalam_Numeric: return Script_Malayalam;
        default:
            return -1;
    };
}

/***********************************************************************
 *      ScriptItemizeOpenType (USP10.@)
 *
 * Split a Unicode string into shapeable parts.
 *
 * PARAMS
 *  pwcInChars  [I] String to split.
 *  cInChars    [I] Number of characters in pwcInChars.
 *  cMaxItems   [I] Maximum number of items to return.
 *  psControl   [I] Pointer to a SCRIPT_CONTROL structure.
 *  psState     [I] Pointer to a SCRIPT_STATE structure.
 *  pItems      [O] Buffer to receive SCRIPT_ITEM structures.
 *  pScriptTags [O] Buffer to receive OPENTYPE_TAGs.
 *  pcItems     [O] Number of script items returned.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptItemizeOpenType(const WCHAR *pwcInChars, int cInChars, int cMaxItems,
                             const SCRIPT_CONTROL *psControl, const SCRIPT_STATE *psState,
                             SCRIPT_ITEM *pItems, OPENTYPE_TAG *pScriptTags, int *pcItems)
{

#define Numeric_space 0x0020
#define ZWNJ 0x200C
#define ZWJ  0x200D

    int   cnt = 0, index = 0, str = 0;
    int   New_Script = -1;
    int   i;
    WORD  *levels = NULL;
    WORD  *strength = NULL;
    WORD  *scripts = NULL;
    WORD  baselevel = 0;
    BOOL  new_run;
    WORD  last_indic = -1;
    WORD layoutRTL = 0;
    BOOL forceLevels = FALSE;

    TRACE("%s,%d,%d,%p,%p,%p,%p\n", debugstr_wn(pwcInChars, cInChars), cInChars, cMaxItems, 
          psControl, psState, pItems, pcItems);

    if (!pwcInChars || !cInChars || !pItems || cMaxItems < 2)
        return E_INVALIDARG;

    scripts = heap_alloc(cInChars * sizeof(WORD));
    if (!scripts)
        return E_OUTOFMEMORY;

    for (i = 0; i < cInChars; i++)
    {
        scripts[i] = get_char_script(pwcInChars[i]);
        /* Devanagari danda (U+0964) and double danda (U+0965) are used for
           all Indic scripts */
        if ((pwcInChars[i] == 0x964 || pwcInChars[i] ==0x965) && last_indic > 0)
            scripts[i] = last_indic;
        else if (is_indic(scripts[i]))
            last_indic = base_indic(scripts[i]);

        /* Some unicode points (Zero Width Space U+200B -
           Right-to-Left Mark U+200F) will force us into bidi mode */
        if (!forceLevels && pwcInChars[i] >= 0x200B && pwcInChars[i] <= 0x200F)
            forceLevels = TRUE;

        /* Diacritical marks merge with other scripts */
        if (scripts[i] == Script_Diacritical && i > 0)
                scripts[i] = scripts[i-1];
    }

    for (i = 0; i < cInChars; i++)
    {
        /* Joiners get merged preferencially right */
        if (i > 0 && (pwcInChars[i] == ZWJ || pwcInChars[i] == ZWNJ))
        {
            int j;
            if (i+1 == cInChars)
                scripts[i] = scripts[i-1];
            else
            {
                for (j = i+1; j < cInChars; j++)
                {
                    if (pwcInChars[j] != ZWJ && pwcInChars[j] != ZWNJ && pwcInChars[j] != Numeric_space)
                    {
                        scripts[i] = scripts[j];
                        break;
                    }
                }
            }
        }
    }

    if (psState && psControl)
    {
        levels = heap_alloc_zero(cInChars * sizeof(WORD));
        if (!levels)
        {
            heap_free(scripts);
            return E_OUTOFMEMORY;
        }

        BIDI_DetermineLevels(pwcInChars, cInChars, psState, psControl, levels);
        baselevel = levels[0];
        for (i = 0; i < cInChars; i++)
            if (levels[i]!=levels[0])
                break;
        if (i >= cInChars && !odd(baselevel) && !odd(psState->uBidiLevel) && !forceLevels)
        {
            heap_free(levels);
            levels = NULL;
        }
        else
        {
            BOOL inNumber = FALSE;
            static WCHAR math_punc[] = {'#','$','%','+',',','-','.','/',':',0x2212, 0x2044, 0x00a0,0};

            strength = heap_alloc_zero(cInChars * sizeof(WORD));
            if (!strength)
            {
                heap_free(scripts);
                heap_free(levels);
                return E_OUTOFMEMORY;
            }
            BIDI_GetStrengths(pwcInChars, cInChars, psControl, strength);

            /* We currently mis-level leading Diacriticals */
            if (scripts[0] == Script_Diacritical)
                for (i = 0; i < cInChars && scripts[0] == Script_Diacritical; i++)
                {
                    levels[i] = odd(levels[i])?levels[i]+1:levels[i];
                    strength[i] = BIDI_STRONG;
                }

            for (i = 0; i < cInChars; i++)
            {
                /* Script_Numeric and select puncuation at level 0 get bumped to level 2 */
                if ((levels[i] == 0 || (odd(psState->uBidiLevel) && levels[i] == psState->uBidiLevel+1)) && inNumber && strchrW(math_punc,pwcInChars[i]))
                {
                    scripts[i] = Script_Numeric;
                    levels[i] = 2;
                }
                else if ((levels[i] == 0 || (odd(psState->uBidiLevel) && levels[i] == psState->uBidiLevel+1)) && scripts[i] == Script_Numeric)
                {
                    levels[i] = 2;
                    inNumber = TRUE;
                }
                else
                    inNumber = FALSE;

                /* Joiners get merged preferencially right */
                if (i > 0 && (pwcInChars[i] == ZWJ || pwcInChars[i] == ZWNJ))
                {
                    int j;
                    if (i+1 == cInChars && levels[i-1] == levels[i])
                        strength[i] = strength[i-1];
                    else
                        for (j = i+1; j < cInChars && levels[i] == levels[j]; j++)
                            if (pwcInChars[j] != ZWJ && pwcInChars[j] != ZWNJ && pwcInChars[j] != Numeric_space)
                            {
                                strength[i] = strength[j];
                                break;
                            }
                }
            }
            if (psControl->fMergeNeutralItems)
            {
                /* Merge the neutrals */
                for (i = 0; i < cInChars; i++)
                {
                    if (strength[i] == BIDI_NEUTRAL || strength[i] == BIDI_WEAK)
                    {
                        int j;
                        for (j = i; j > 0; j--)
                        {
                            if (levels[i] != levels[j])
                                break;
                            if ((strength[j] == BIDI_STRONG) || (strength[i] == BIDI_NEUTRAL && strength[j] == BIDI_WEAK))
                            {
                                scripts[i] = scripts[j];
                                strength[i] = strength[j];
                                break;
                            }
                        }
                    }
                    /* Try going the other way */
                    if (strength[i] == BIDI_NEUTRAL || strength[i] == BIDI_WEAK)
                    {
                        int j;
                        for (j = i; j < cInChars; j++)
                        {
                            if (levels[i] != levels[j])
                                break;
                            if ((strength[j] == BIDI_STRONG) || (strength[i] == BIDI_NEUTRAL && strength[j] == BIDI_WEAK))
                            {
                                scripts[i] = scripts[j];
                                strength[i] = strength[j];
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    while ((!levels || (levels && levels[cnt+1] == levels[0])) && (pwcInChars[cnt] == Numeric_space) && cnt < cInChars)
        cnt++;

    if (cnt == cInChars) /* All Spaces */
    {
        cnt = 0;
        New_Script = scripts[cnt];
    }

    pItems[index].iCharPos = 0;
    pItems[index].a = scriptInformation[scripts[cnt]].a;
    pScriptTags[index] = scriptInformation[scripts[cnt]].scriptTag;

    if (strength && strength[cnt] == BIDI_STRONG)
        str = strength[cnt];
    else if (strength)
        str = strength[0];

    cnt = 0;

    if (levels)
    {
        if (strength[cnt] == BIDI_STRONG)
            layoutRTL = (odd(levels[cnt]))?1:0;
        else
            layoutRTL = (psState->uBidiLevel || odd(levels[cnt]))?1:0;
        pItems[index].a.fRTL = odd(levels[cnt]);
        pItems[index].a.fLayoutRTL = layoutRTL;
        pItems[index].a.s.uBidiLevel = levels[cnt];
    }
    else if (!pItems[index].a.s.uBidiLevel)
    {
        layoutRTL = (odd(baselevel))?1:0;
        pItems[index].a.s.uBidiLevel = baselevel;
        pItems[index].a.fLayoutRTL = odd(baselevel);
        pItems[index].a.fRTL = odd(baselevel);
    }

    TRACE("New_Level=%i New_Strength=%i New_Script=%d, eScript=%d index=%d cnt=%d iCharPos=%d\n",
          levels?levels[cnt]:-1, str, New_Script, pItems[index].a.eScript, index, cnt,
          pItems[index].iCharPos);

    for (cnt=1; cnt < cInChars; cnt++)
    {
        if(pwcInChars[cnt] != Numeric_space)
            New_Script = scripts[cnt];
        else if (levels)
        {
            int j = 1;
            while (cnt + j < cInChars - 1 && pwcInChars[cnt+j] == Numeric_space && levels[cnt] == levels[cnt+j])
                j++;
            if (cnt + j < cInChars && levels[cnt] == levels[cnt+j])
                New_Script = scripts[cnt+j];
            else
                New_Script = scripts[cnt];
        }

        new_run = FALSE;
        /* merge space strengths*/
        if (strength && strength[cnt] == BIDI_STRONG && str != BIDI_STRONG && New_Script == pItems[index].a.eScript)
            str = BIDI_STRONG;

        if (strength && strength[cnt] == BIDI_NEUTRAL && str == BIDI_STRONG && pwcInChars[cnt] != Numeric_space && New_Script == pItems[index].a.eScript)
            str = BIDI_NEUTRAL;

        /* changes in level */
        if (levels && (levels[cnt] != pItems[index].a.s.uBidiLevel))
        {
            TRACE("Level break(%i/%i)\n",pItems[index].a.s.uBidiLevel,levels[cnt]);
            new_run = TRUE;
        }
        /* changes in strength */
        else if (strength && pwcInChars[cnt] != Numeric_space && str != strength[cnt])
        {
            TRACE("Strength break (%i/%i)\n",str,strength[cnt]);
            new_run = TRUE;
        }
        /* changes in script */
        else if (((pwcInChars[cnt] != Numeric_space) && (New_Script != -1) && (New_Script != pItems[index].a.eScript)) || (New_Script == Script_Control))
        {
            TRACE("Script break(%i/%i)\n",pItems[index].a.eScript,New_Script);
            new_run = TRUE;
        }

        if (!new_run && strength && str == BIDI_STRONG)
        {
            layoutRTL = odd(levels[cnt])?1:0;
            pItems[index].a.fLayoutRTL = layoutRTL;
        }

        if (new_run)
        {
            TRACE("New_Level = %i, New_Strength = %i, New_Script=%d, eScript=%d\n", levels?levels[cnt]:-1, strength?strength[cnt]:str, New_Script, pItems[index].a.eScript);

            index++;
            if  (index+1 > cMaxItems)
                return E_OUTOFMEMORY;

            if (strength)
                str = strength[cnt];

            pItems[index].iCharPos = cnt;
            memset(&pItems[index].a, 0, sizeof(SCRIPT_ANALYSIS));

            pItems[index].a = scriptInformation[New_Script].a;
            pScriptTags[index] = scriptInformation[New_Script].scriptTag;
            if (levels)
            {
                if (levels[cnt] == 0)
                    layoutRTL = 0;
                else
                    layoutRTL = (layoutRTL || odd(levels[cnt]))?1:0;
                pItems[index].a.fRTL = odd(levels[cnt]);
                pItems[index].a.fLayoutRTL = layoutRTL;
                pItems[index].a.s.uBidiLevel = levels[cnt];
            }
            else if (!pItems[index].a.s.uBidiLevel)
            {
                pItems[index].a.s.uBidiLevel = baselevel;
                pItems[index].a.fLayoutRTL = layoutRTL;
                pItems[index].a.fRTL = odd(baselevel);
            }

            TRACE("index=%d cnt=%d iCharPos=%d\n", index, cnt, pItems[index].iCharPos);
        }
    }

    /* While not strictly necessary according to the spec, make sure the n+1
     * item is set up to prevent random behaviour if the caller erroneously
     * checks the n+1 structure                                              */
    index++;
    memset(&pItems[index].a, 0, sizeof(SCRIPT_ANALYSIS));

    TRACE("index=%d cnt=%d iCharPos=%d\n", index, cnt, pItems[index].iCharPos);

    /*  Set one SCRIPT_STATE item being returned  */
    if  (index + 1 > cMaxItems) return E_OUTOFMEMORY;
    if (pcItems) *pcItems = index;

    /*  Set SCRIPT_ITEM                                     */
    pItems[index].iCharPos = cnt;         /* the last item contains the ptr to the lastchar */
    heap_free(levels);
    heap_free(strength);
    heap_free(scripts);
    return S_OK;
}

/***********************************************************************
 *      ScriptItemize (USP10.@)
 *
 * Split a Unicode string into shapeable parts.
 *
 * PARAMS
 *  pwcInChars [I] String to split.
 *  cInChars   [I] Number of characters in pwcInChars.
 *  cMaxItems  [I] Maximum number of items to return.
 *  psControl  [I] Pointer to a SCRIPT_CONTROL structure.
 *  psState    [I] Pointer to a SCRIPT_STATE structure.
 *  pItems     [O] Buffer to receive SCRIPT_ITEM structures.
 *  pcItems    [O] Number of script items returned.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptItemize(const WCHAR *pwcInChars, int cInChars, int cMaxItems,
                             const SCRIPT_CONTROL *psControl, const SCRIPT_STATE *psState,
                             SCRIPT_ITEM *pItems, int *pcItems)
{
    OPENTYPE_TAG *discarded_tags;
    HRESULT res;

    discarded_tags = heap_alloc(cMaxItems * sizeof(OPENTYPE_TAG));
    if (!discarded_tags)
        return E_OUTOFMEMORY;
    res = ScriptItemizeOpenType(pwcInChars, cInChars, cMaxItems, psControl, psState, pItems, discarded_tags, pcItems);
    heap_free(discarded_tags);
    return res;
}

static inline int getGivenTabWidth(ScriptCache *psc, SCRIPT_TABDEF *pTabdef, int charPos, int current_x)
{
    int defWidth;
    int cTabStops=0;
    INT *lpTabPos = NULL;
    INT nTabOrg = 0;
    INT x = 0;

    if (pTabdef)
        lpTabPos = pTabdef->pTabStops;

    if (pTabdef && pTabdef->iTabOrigin)
    {
        if (pTabdef->iScale)
            nTabOrg = (pTabdef->iTabOrigin * pTabdef->iScale)/4;
        else
            nTabOrg = pTabdef->iTabOrigin * psc->tm.tmAveCharWidth;
    }

    if (pTabdef)
        cTabStops = pTabdef->cTabStops;

    if (cTabStops == 1)
    {
        if (pTabdef->iScale)
            defWidth = ((pTabdef->pTabStops[0])*pTabdef->iScale) / 4;
        else
            defWidth = (pTabdef->pTabStops[0])*psc->tm.tmAveCharWidth;
        cTabStops = 0;
    }
    else
        defWidth = 8 * psc->tm.tmAveCharWidth;

    for (; cTabStops>0 ; lpTabPos++, cTabStops--)
    {
        int position = *lpTabPos;
        if (position < 0)
            position = -1 * position;
        if (pTabdef->iScale)
            position = (position * pTabdef->iScale) / 4;
        else
            position = position * psc->tm.tmAveCharWidth;

        if( nTabOrg + position > current_x)
        {
            if( *lpTabPos >= 0)
            {
                /* a left aligned tab */
                x = (nTabOrg + *lpTabPos) - current_x;
                break;
            }
            else
            {
                FIXME("Negative tabstop\n");
                break;
            }
        }
    }
    if ((!cTabStops) && (defWidth > 0))
        x =((((current_x - nTabOrg) / defWidth)+1) * defWidth) - current_x;
    else if ((!cTabStops) && (defWidth < 0))
        FIXME("TODO: Negative defWidth\n");

    return x;
}

/***********************************************************************
 * Helper function for ScriptStringAnalyse
 */
static BOOL requires_fallback(HDC hdc, SCRIPT_CACHE *psc, SCRIPT_ANALYSIS *psa,
                              const WCHAR *pwcInChars, int cChars )
{
    /* FIXME: When to properly fallback is still a bit of a mystery */
    WORD *glyphs;

    if (psa->fNoGlyphIndex)
        return FALSE;

    if (init_script_cache(hdc, psc) != S_OK)
        return FALSE;

    if (SHAPE_CheckFontForRequiredFeatures(hdc, (ScriptCache *)*psc, psa) != S_OK)
        return TRUE;

    glyphs = heap_alloc(sizeof(WORD) * cChars);
    if (!glyphs)
        return FALSE;
    if (ScriptGetCMap(hdc, psc, pwcInChars, cChars, 0, glyphs) != S_OK)
    {
        heap_free(glyphs);
        return TRUE;
    }
    heap_free(glyphs);

    return FALSE;
}

static void find_fallback_font(DWORD scriptid, LPWSTR FaceName)
{
    HKEY hkey;

    if (!RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Uniscribe\\Fallback", &hkey))
    {
        static const WCHAR szFmt[] = {'%','x',0};
        WCHAR value[10];
        DWORD count = LF_FACESIZE * sizeof(WCHAR);
        DWORD type;

        sprintfW(value, szFmt, scriptInformation[scriptid].scriptTag);
        if (RegQueryValueExW(hkey, value, 0, &type, (LPBYTE)FaceName, &count))
            lstrcpyW(FaceName,scriptInformation[scriptid].fallbackFont);
        RegCloseKey(hkey);
    }
    else
        lstrcpyW(FaceName,scriptInformation[scriptid].fallbackFont);
}

/***********************************************************************
 *      ScriptStringAnalyse (USP10.@)
 *
 */
HRESULT WINAPI ScriptStringAnalyse(HDC hdc, const void *pString, int cString,
                                   int cGlyphs, int iCharset, DWORD dwFlags,
                                   int iReqWidth, SCRIPT_CONTROL *psControl,
                                   SCRIPT_STATE *psState, const int *piDx,
                                   SCRIPT_TABDEF *pTabdef, const BYTE *pbInClass,
                                   SCRIPT_STRING_ANALYSIS *pssa)
{
    HRESULT hr = E_OUTOFMEMORY;
    StringAnalysis *analysis = NULL;
    SCRIPT_CONTROL sControl;
    SCRIPT_STATE sState;
    int i, num_items = 255;
    BYTE   *BidiLevel;
    WCHAR *iString = NULL;

    TRACE("(%p,%p,%d,%d,%d,0x%x,%d,%p,%p,%p,%p,%p,%p)\n",
          hdc, pString, cString, cGlyphs, iCharset, dwFlags, iReqWidth,
          psControl, psState, piDx, pTabdef, pbInClass, pssa);

    if (iCharset != -1)
    {
        FIXME("Only Unicode strings are supported\n");
        return E_INVALIDARG;
    }
    if (cString < 1 || !pString) return E_INVALIDARG;
    if ((dwFlags & SSA_GLYPHS) && !hdc) return E_PENDING;

    if (!(analysis = heap_alloc_zero(sizeof(StringAnalysis)))) return E_OUTOFMEMORY;
    if (!(analysis->pItem = heap_alloc_zero(num_items * sizeof(SCRIPT_ITEM) + 1))) goto error;

    /* FIXME: handle clipping */
    analysis->clip_len = cString;
    analysis->hdc = hdc;
    analysis->dwFlags = dwFlags;

    if (psState)
        sState = *psState;
    else
        memset(&sState, 0, sizeof(SCRIPT_STATE));

    if (psControl)
        sControl = *psControl;
    else
        memset(&sControl, 0, sizeof(SCRIPT_CONTROL));

    if (dwFlags & SSA_PASSWORD)
    {
        iString = heap_alloc(sizeof(WCHAR)*cString);
        if (!iString)
        {
            hr = E_OUTOFMEMORY;
            goto error;
        }
        for (i = 0; i < cString; i++)
            iString[i] = *((const WCHAR *)pString);
        pString = iString;
    }

    hr = ScriptItemize(pString, cString, num_items, &sControl, &sState, analysis->pItem,
                       &analysis->numItems);

    while (hr == E_OUTOFMEMORY)
    {
        SCRIPT_ITEM *tmp;

        num_items *= 2;
        if (!(tmp = heap_realloc_zero(analysis->pItem, num_items * sizeof(SCRIPT_ITEM) + 1)))
            goto error;

        analysis->pItem = tmp;
        hr = ScriptItemize(pString, cString, num_items, psControl, psState, analysis->pItem,
                           &analysis->numItems);
    }
    if (hr != S_OK) goto error;

    /* set back to out of memory for default goto error behaviour */
    hr = E_OUTOFMEMORY;

    if (dwFlags & SSA_BREAK)
    {
        if ((analysis->logattrs = heap_alloc(sizeof(SCRIPT_LOGATTR) * cString)))
        {
            for (i = 0; i < analysis->numItems; i++)
                ScriptBreak(&((LPWSTR)pString)[analysis->pItem[i].iCharPos], analysis->pItem[i+1].iCharPos - analysis->pItem[i].iCharPos, &analysis->pItem[i].a, &analysis->logattrs[analysis->pItem[i].iCharPos]);
        }
        else
            goto error;
    }

    if (!(analysis->logical2visual = heap_alloc_zero(sizeof(int) * analysis->numItems)))
        goto error;
    if (!(BidiLevel = heap_alloc_zero(analysis->numItems)))
        goto error;

    if (dwFlags & SSA_GLYPHS)
    {
        int tab_x = 0;
        if (!(analysis->glyphs = heap_alloc_zero(sizeof(StringGlyphs) * analysis->numItems)))
        {
            heap_free(BidiLevel);
            goto error;
        }

        for (i = 0; i < analysis->numItems; i++)
        {
            SCRIPT_CACHE *sc = (SCRIPT_CACHE*)&analysis->glyphs[i].sc;
            int cChar = analysis->pItem[i+1].iCharPos - analysis->pItem[i].iCharPos;
            int numGlyphs = 1.5 * cChar + 16;
            WORD *glyphs = heap_alloc_zero(sizeof(WORD) * numGlyphs);
            WORD *pwLogClust = heap_alloc_zero(sizeof(WORD) * cChar);
            int *piAdvance = heap_alloc_zero(sizeof(int) * numGlyphs);
            SCRIPT_VISATTR *psva = heap_alloc_zero(sizeof(SCRIPT_VISATTR) * numGlyphs);
            GOFFSET *pGoffset = heap_alloc_zero(sizeof(GOFFSET) * numGlyphs);
            ABC *abc = heap_alloc_zero(sizeof(ABC));
            int numGlyphsReturned;
            HFONT originalFont = 0x0;

            /* FIXME: non unicode strings */
            const WCHAR* pStr = (const WCHAR*)pString;
            analysis->glyphs[i].fallbackFont = NULL;

            if (!glyphs || !pwLogClust || !piAdvance || !psva || !pGoffset || !abc)
            {
                heap_free (BidiLevel);
                heap_free (glyphs);
                heap_free (pwLogClust);
                heap_free (piAdvance);
                heap_free (psva);
                heap_free (pGoffset);
                heap_free (abc);
                hr = E_OUTOFMEMORY;
                goto error;
            }

            if ((dwFlags & SSA_FALLBACK) && requires_fallback(hdc, sc, &analysis->pItem[i].a, &pStr[analysis->pItem[i].iCharPos], cChar))
            {
                LOGFONTW lf;
                GetObjectW(GetCurrentObject(hdc, OBJ_FONT), sizeof(lf), & lf);
                lf.lfCharSet = scriptInformation[analysis->pItem[i].a.eScript].props.bCharSet;
                find_fallback_font(analysis->pItem[i].a.eScript, lf.lfFaceName);
                analysis->glyphs[i].fallbackFont = CreateFontIndirectW(&lf);
                if (analysis->glyphs[i].fallbackFont)
                {
                    ScriptFreeCache(sc);
                    originalFont = SelectObject(hdc, analysis->glyphs[i].fallbackFont);
                }
            }

            hr = ScriptShape(hdc, sc, &pStr[analysis->pItem[i].iCharPos],
                             cChar, numGlyphs, &analysis->pItem[i].a,
                             glyphs, pwLogClust, psva, &numGlyphsReturned);
            hr = ScriptPlace(hdc, sc, glyphs, numGlyphsReturned, psva, &analysis->pItem[i].a,
                             piAdvance, pGoffset, abc);
            if (originalFont)
                SelectObject(hdc,originalFont);

            if (dwFlags & SSA_TAB)
            {
                int tabi = 0;
                for (tabi = 0; tabi < cChar; tabi++)
                {
                    if (pStr[analysis->pItem[i].iCharPos+tabi] == 0x0009)
                        piAdvance[tabi] = getGivenTabWidth(analysis->glyphs[i].sc, pTabdef, analysis->pItem[i].iCharPos+tabi, tab_x);
                    tab_x+=piAdvance[tabi];
                }
            }

            analysis->glyphs[i].numGlyphs = numGlyphsReturned;
            analysis->glyphs[i].glyphs = glyphs;
            analysis->glyphs[i].pwLogClust = pwLogClust;
            analysis->glyphs[i].piAdvance = piAdvance;
            analysis->glyphs[i].psva = psva;
            analysis->glyphs[i].pGoffset = pGoffset;
            analysis->glyphs[i].abc = abc;
            analysis->glyphs[i].iMaxPosX= -1;

            BidiLevel[i] = analysis->pItem[i].a.s.uBidiLevel;
        }
    }
    else
    {
        for (i = 0; i < analysis->numItems; i++)
            BidiLevel[i] = analysis->pItem[i].a.s.uBidiLevel;
    }

    ScriptLayout(analysis->numItems, BidiLevel, NULL, analysis->logical2visual);
    heap_free(BidiLevel);

    *pssa = analysis;
    heap_free(iString);
    return S_OK;

error:
    heap_free(iString);
    heap_free(analysis->glyphs);
    heap_free(analysis->logattrs);
    heap_free(analysis->pItem);
    heap_free(analysis->logical2visual);
    heap_free(analysis);
    return hr;
}

static inline BOOL does_glyph_start_cluster(const SCRIPT_VISATTR *pva, const WORD *pwLogClust, int cChars, int glyph, int direction)
{
    int i;

    if (pva[glyph].fClusterStart)
        return TRUE;
    for (i = 0; i < cChars; i++)
        if (pwLogClust[i] == glyph) break;
    if (i != cChars)
        return TRUE;

    return FALSE;
}


static HRESULT SS_ItemOut( SCRIPT_STRING_ANALYSIS ssa,
                           int iX,
                           int iY,
                           int iItem,
                           int cStart,
                           int cEnd,
                           UINT uOptions,
                           const RECT *prc,
                           BOOL fSelected,
                           BOOL fDisabled)
{
    StringAnalysis *analysis;
    int off_x = 0;
    HRESULT hr;
    COLORREF BkColor = 0x0;
    COLORREF TextColor = 0x0;
    INT BkMode = 0;
    INT runStart, runEnd;
    INT iGlyph, cGlyphs;
    HFONT oldFont = 0x0;

    TRACE("(%p,%d,%d,%d,%d,%d, 0x%1x, %d, %d)\n",
         ssa, iX, iY, iItem, cStart, cEnd, uOptions, fSelected, fDisabled);

    if (!(analysis = ssa)) return E_INVALIDARG;

    if ((cStart >= 0 && analysis->pItem[iItem+1].iCharPos <= cStart) ||
         (cEnd >= 0 && analysis->pItem[iItem].iCharPos >= cEnd))
            return S_OK;

    if (fSelected)
    {
        BkMode = GetBkMode(analysis->hdc);
        SetBkMode( analysis->hdc, OPAQUE);
        BkColor = GetBkColor(analysis->hdc);
        SetBkColor(analysis->hdc, GetSysColor(COLOR_HIGHLIGHT));
        if (!fDisabled)
        {
            TextColor = GetTextColor(analysis->hdc);
            SetTextColor(analysis->hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
        }
    }
    if (analysis->glyphs[iItem].fallbackFont)
        oldFont = SelectObject(analysis->hdc, analysis->glyphs[iItem].fallbackFont);

    if (cStart >= 0 && analysis->pItem[iItem+1].iCharPos > cStart && analysis->pItem[iItem].iCharPos <= cStart)
        runStart = cStart - analysis->pItem[iItem].iCharPos;
    else
        runStart =  0;
    if (cEnd >= 0 && analysis->pItem[iItem+1].iCharPos > cEnd && analysis->pItem[iItem].iCharPos <= cEnd)
        runEnd = (cEnd-1) - analysis->pItem[iItem].iCharPos;
    else
        runEnd = (analysis->pItem[iItem+1].iCharPos - analysis->pItem[iItem].iCharPos) - 1;

    if (analysis->pItem[iItem].a.fRTL)
    {
        if (cEnd >= 0 && cEnd < analysis->pItem[iItem+1].iCharPos)
            ScriptStringCPtoX(ssa, cEnd, FALSE, &off_x);
        else
            ScriptStringCPtoX(ssa, analysis->pItem[iItem+1].iCharPos-1, TRUE, &off_x);
    }
    else
    {
        if (cStart >=0 && runStart)
            ScriptStringCPtoX(ssa, cStart, FALSE, &off_x);
        else
            ScriptStringCPtoX(ssa, analysis->pItem[iItem].iCharPos, FALSE, &off_x);
    }

    if (analysis->pItem[iItem].a.fRTL)
        iGlyph = analysis->glyphs[iItem].pwLogClust[runEnd];
    else
        iGlyph = analysis->glyphs[iItem].pwLogClust[runStart];

    if (analysis->pItem[iItem].a.fRTL)
        cGlyphs = analysis->glyphs[iItem].pwLogClust[runStart] - iGlyph;
    else
        cGlyphs = analysis->glyphs[iItem].pwLogClust[runEnd] - iGlyph;

    cGlyphs++;

    if (cEnd < 0 || scriptInformation[analysis->pItem[iItem].a.eScript].props.fNeedsCaretInfo)
    {
        INT direction;
        INT clust_glyph;

        clust_glyph = iGlyph + cGlyphs;
        if (analysis->pItem[iItem].a.fRTL)
            direction = -1;
        else
            direction = 1;

        while(clust_glyph < analysis->glyphs[iItem].numGlyphs &&
              !does_glyph_start_cluster(analysis->glyphs[iItem].psva, analysis->glyphs[iItem].pwLogClust, (analysis->pItem[iItem+1].iCharPos - analysis->pItem[iItem].iCharPos), clust_glyph, direction))
        {
            cGlyphs++;
            clust_glyph++;
        }
    }

    hr = ScriptTextOut(analysis->hdc,
                       (SCRIPT_CACHE *)&analysis->glyphs[iItem].sc, iX + off_x,
                       iY, uOptions, prc, &analysis->pItem[iItem].a, NULL, 0,
                       &analysis->glyphs[iItem].glyphs[iGlyph], cGlyphs,
                       &analysis->glyphs[iItem].piAdvance[iGlyph], NULL,
                       &analysis->glyphs[iItem].pGoffset[iGlyph]);

    TRACE("ScriptTextOut hr=%08x\n", hr);

    if (fSelected)
    {
        SetBkColor(analysis->hdc, BkColor);
        SetBkMode( analysis->hdc, BkMode);
        if (!fDisabled)
            SetTextColor(analysis->hdc, TextColor);
    }
    if (analysis->glyphs[iItem].fallbackFont)
        SelectObject(analysis->hdc, oldFont);

    return hr;
}

/***********************************************************************
 *      ScriptStringOut (USP10.@)
 *
 * This function takes the output of ScriptStringAnalyse and joins the segments
 * of glyphs and passes the resulting string to ScriptTextOut.  ScriptStringOut
 * only processes glyphs.
 *
 * Parameters:
 *  ssa       [I] buffer to hold the analysed string components
 *  iX        [I] X axis displacement for output
 *  iY        [I] Y axis displacement for output
 *  uOptions  [I] flags controling output processing
 *  prc       [I] rectangle coordinates
 *  iMinSel   [I] starting pos for substringing output string
 *  iMaxSel   [I] ending pos for substringing output string
 *  fDisabled [I] controls text highlighting
 *
 *  RETURNS
 *   Success: S_OK
 *   Failure: is the value returned by ScriptTextOut
 */
HRESULT WINAPI ScriptStringOut(SCRIPT_STRING_ANALYSIS ssa,
                               int iX,
                               int iY, 
                               UINT uOptions, 
                               const RECT *prc, 
                               int iMinSel, 
                               int iMaxSel,
                               BOOL fDisabled)
{
    StringAnalysis *analysis;
    int   item;
    HRESULT hr;

    TRACE("(%p,%d,%d,0x%1x,%p,%d,%d,%d)\n",
         ssa, iX, iY, uOptions, prc, iMinSel, iMaxSel, fDisabled);

    if (!(analysis = ssa)) return E_INVALIDARG;
    if (!(analysis->dwFlags & SSA_GLYPHS)) return E_INVALIDARG;

    for (item = 0; item < analysis->numItems; item++)
    {
        hr = SS_ItemOut( ssa, iX, iY, analysis->logical2visual[item], -1, -1, uOptions, prc, FALSE, fDisabled);
        if (FAILED(hr))
            return hr;
    }

    if (iMinSel < iMaxSel && (iMinSel > 0 || iMaxSel > 0))
    {
        if (iMaxSel > 0 &&  iMinSel < 0)
            iMinSel = 0;
        for (item = 0; item < analysis->numItems; item++)
        {
            hr = SS_ItemOut( ssa, iX, iY, analysis->logical2visual[item], iMinSel, iMaxSel, uOptions, prc, TRUE, fDisabled);
            if (FAILED(hr))
                return hr;
        }
    }

    return S_OK;
}

/***********************************************************************
 *      ScriptStringCPtoX (USP10.@)
 *
 */
HRESULT WINAPI ScriptStringCPtoX(SCRIPT_STRING_ANALYSIS ssa, int icp, BOOL fTrailing, int* pX)
{
    int item;
    int runningX = 0;
    StringAnalysis* analysis = ssa;

    TRACE("(%p), %d, %d, (%p)\n", ssa, icp, fTrailing, pX);

    if (!ssa || !pX) return S_FALSE;
    if (!(analysis->dwFlags & SSA_GLYPHS)) return S_FALSE;

    /* icp out of range */
    if(icp < 0)
    {
        analysis->invalid = TRUE;
        return E_INVALIDARG;
    }

    for(item=0; item<analysis->numItems; item++)
    {
        int CP, i;
        int offset;

        i = analysis->logical2visual[item];
        CP = analysis->pItem[i+1].iCharPos - analysis->pItem[i].iCharPos;
        /* initialize max extents for uninitialized runs */
        if (analysis->glyphs[i].iMaxPosX == -1)
        {
            if (analysis->pItem[i].a.fRTL)
                ScriptCPtoX(0, FALSE, CP, analysis->glyphs[i].numGlyphs, analysis->glyphs[i].pwLogClust,
                            analysis->glyphs[i].psva, analysis->glyphs[i].piAdvance,
                            &analysis->pItem[i].a, &analysis->glyphs[i].iMaxPosX);
            else
                ScriptCPtoX(CP, TRUE, CP, analysis->glyphs[i].numGlyphs, analysis->glyphs[i].pwLogClust,
                            analysis->glyphs[i].psva, analysis->glyphs[i].piAdvance,
                            &analysis->pItem[i].a, &analysis->glyphs[i].iMaxPosX);
        }

        if (icp >= analysis->pItem[i+1].iCharPos || icp < analysis->pItem[i].iCharPos)
        {
            runningX += analysis->glyphs[i].iMaxPosX;
            continue;
        }

        icp -= analysis->pItem[i].iCharPos;
        ScriptCPtoX(icp, fTrailing, CP, analysis->glyphs[i].numGlyphs, analysis->glyphs[i].pwLogClust,
                    analysis->glyphs[i].psva, analysis->glyphs[i].piAdvance,
                    &analysis->pItem[i].a, &offset);
        runningX += offset;

        *pX = runningX;
        return S_OK;
    }

    /* icp out of range */
    analysis->invalid = TRUE;
    return E_INVALIDARG;
}

/***********************************************************************
 *      ScriptStringXtoCP (USP10.@)
 *
 */
HRESULT WINAPI ScriptStringXtoCP(SCRIPT_STRING_ANALYSIS ssa, int iX, int* piCh, int* piTrailing)
{
    StringAnalysis* analysis = ssa;
    int item;

    TRACE("(%p), %d, (%p), (%p)\n", ssa, iX, piCh, piTrailing);

    if (!ssa || !piCh || !piTrailing) return S_FALSE;
    if (!(analysis->dwFlags & SSA_GLYPHS)) return S_FALSE;

    /* out of range */
    if(iX < 0)
    {
        if (analysis->pItem[0].a.fRTL)
        {
            *piCh = 1;
            *piTrailing = FALSE;
        }
        else
        {
            *piCh = -1;
            *piTrailing = TRUE;
        }
        return S_OK;
    }

    for(item=0; item<analysis->numItems; item++)
    {
        int i;
        int CP;

        for (i = 0; i < analysis->numItems && analysis->logical2visual[i] != item; i++)
        /* nothing */;

        CP = analysis->pItem[i+1].iCharPos - analysis->pItem[i].iCharPos;
        /* initialize max extents for uninitialized runs */
        if (analysis->glyphs[i].iMaxPosX == -1)
        {
            if (analysis->pItem[i].a.fRTL)
                ScriptCPtoX(0, FALSE, CP, analysis->glyphs[i].numGlyphs, analysis->glyphs[i].pwLogClust,
                            analysis->glyphs[i].psva, analysis->glyphs[i].piAdvance,
                            &analysis->pItem[i].a, &analysis->glyphs[i].iMaxPosX);
            else
                ScriptCPtoX(CP, TRUE, CP, analysis->glyphs[i].numGlyphs, analysis->glyphs[i].pwLogClust,
                            analysis->glyphs[i].psva, analysis->glyphs[i].piAdvance,
                            &analysis->pItem[i].a, &analysis->glyphs[i].iMaxPosX);
        }

        if (iX > analysis->glyphs[i].iMaxPosX)
        {
            iX -= analysis->glyphs[i].iMaxPosX;
            continue;
        }

        ScriptXtoCP(iX, CP, analysis->glyphs[i].numGlyphs, analysis->glyphs[i].pwLogClust,
                    analysis->glyphs[i].psva, analysis->glyphs[i].piAdvance,
                    &analysis->pItem[i].a, piCh, piTrailing);
        *piCh += analysis->pItem[i].iCharPos;

        return S_OK;
    }

    /* out of range */
    *piCh = analysis->pItem[analysis->numItems].iCharPos;
    *piTrailing = FALSE;

    return S_OK;
}


/***********************************************************************
 *      ScriptStringFree (USP10.@)
 *
 * Free a string analysis.
 *
 * PARAMS
 *  pssa [I] string analysis.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptStringFree(SCRIPT_STRING_ANALYSIS *pssa)
{
    StringAnalysis* analysis;
    BOOL invalid;
    int i;

    TRACE("(%p)\n", pssa);

    if (!pssa || !(analysis = *pssa)) return E_INVALIDARG;

    invalid = analysis->invalid;

    if (analysis->glyphs)
    {
        for (i = 0; i < analysis->numItems; i++)
        {
            heap_free(analysis->glyphs[i].glyphs);
            heap_free(analysis->glyphs[i].pwLogClust);
            heap_free(analysis->glyphs[i].piAdvance);
            heap_free(analysis->glyphs[i].psva);
            heap_free(analysis->glyphs[i].pGoffset);
            heap_free(analysis->glyphs[i].abc);
            if (analysis->glyphs[i].fallbackFont)
                DeleteObject(analysis->glyphs[i].fallbackFont);
            ScriptFreeCache((SCRIPT_CACHE *)&analysis->glyphs[i].sc);
            heap_free(analysis->glyphs[i].sc);
        }
        heap_free(analysis->glyphs);
    }

    heap_free(analysis->pItem);
    heap_free(analysis->logattrs);
    heap_free(analysis->sz);
    heap_free(analysis->logical2visual);
    heap_free(analysis);

    if (invalid) return E_INVALIDARG;
    return S_OK;
}

static inline int get_cluster_size(const WORD *pwLogClust, int cChars, int item,
                                   int direction, int* iCluster, int *check_out)
{
    int clust_size = 1;
    int check;
    WORD clust = pwLogClust[item];

    for (check = item+direction; check < cChars && check >= 0; check+=direction)
    {
        if (pwLogClust[check] == clust)
        {
            clust_size ++;
            if (iCluster && *iCluster == -1)
                *iCluster = item;
        }
        else break;
    }

    if (check_out)
        *check_out = check;

    return clust_size;
}

static inline int get_glyph_cluster_advance(const int* piAdvance, const SCRIPT_VISATTR *pva, const WORD *pwLogClust, int cGlyphs, int cChars, int glyph, int direction)
{
    int advance;
    int log_clust_max = 0;
    int i;

    advance = piAdvance[glyph];

    for (i = 0; i < cChars; i++)
    {
        if (pwLogClust[i] > log_clust_max)
            log_clust_max = pwLogClust[i];
    }

    if (glyph > log_clust_max)
        return advance;

    for (glyph+=direction; glyph < cGlyphs && glyph >= 0; glyph +=direction)
    {

        if (does_glyph_start_cluster(pva, pwLogClust, cChars, glyph, direction))
            break;
        if (glyph > log_clust_max)
            break;
        advance += piAdvance[glyph];
    }

    return advance;
}

/***********************************************************************
 *      ScriptCPtoX (USP10.@)
 *
 */
HRESULT WINAPI ScriptCPtoX(int iCP,
                           BOOL fTrailing,
                           int cChars,
                           int cGlyphs,
                           const WORD *pwLogClust,
                           const SCRIPT_VISATTR *psva,
                           const int *piAdvance,
                           const SCRIPT_ANALYSIS *psa,
                           int *piX)
{
    int item;
    float iPosX;
    int iSpecial = -1;
    int iCluster = -1;
    int clust_size = 1;
    float special_size = 0.0;
    int iMaxPos = 0;
    int advance = 0;
    BOOL rtl = FALSE;

    TRACE("(%d,%d,%d,%d,%p,%p,%p,%p,%p)\n",
          iCP, fTrailing, cChars, cGlyphs, pwLogClust, psva, piAdvance,
          psa, piX);

    if (psa->fRTL && ! psa->fLogicalOrder)
        rtl = TRUE;

    if (fTrailing)
        iCP++;

    if (rtl)
    {
        int max_clust = pwLogClust[0];

        for (item=0; item < cGlyphs; item++)
            if (pwLogClust[item] > max_clust)
            {
                ERR("We do not handle non reversed clusters properly\n");
                break;
            }

        iMaxPos = 0;
        for (item = max_clust; item >=0; item --)
            iMaxPos += piAdvance[item];
    }

    iPosX = 0.0;
    for (item=0; item < iCP && item < cChars; item++)
    {
        if (iSpecial == -1 && (iCluster == -1 || (iCluster != -1 && iCluster+clust_size <= item)))
        {
            int check;
            int clust = pwLogClust[item];

            iCluster = -1;
            clust_size = get_cluster_size(pwLogClust, cChars, item, 1, &iCluster,
                                          &check);

            advance = get_glyph_cluster_advance(piAdvance, psva, pwLogClust, cGlyphs, cChars, clust, 1);

            if (check >= cChars && !iMaxPos)
            {
                for (check = clust; check < cChars; check++)
                    special_size += get_glyph_cluster_advance(piAdvance, psva, pwLogClust, cGlyphs, cChars, check, 1);
                iSpecial = item;
                special_size /= (cChars - item);
                iPosX += special_size;
            }
            else
            {
                if (scriptInformation[psa->eScript].props.fNeedsCaretInfo)
                {
                    clust_size --;
                    if (clust_size == 0)
                        iPosX += advance;
                }
                else
                    iPosX += advance / (float)clust_size;
            }
        }
        else if (iSpecial != -1)
            iPosX += special_size;
        else /* (iCluster != -1) */
        {
            int adv = get_glyph_cluster_advance(piAdvance, psva, pwLogClust, cGlyphs, cChars, pwLogClust[iCluster], 1);
            if (scriptInformation[psa->eScript].props.fNeedsCaretInfo)
            {
                clust_size --;
                if (clust_size == 0)
                    iPosX += adv;
            }
            else
                iPosX += adv / (float)clust_size;
        }
    }

    if (iMaxPos > 0)
    {
        iPosX = iMaxPos - iPosX;
        if (iPosX < 0)
            iPosX = 0;
    }

    *piX = iPosX;
    TRACE("*piX=%d\n", *piX);
    return S_OK;
}

/***********************************************************************
 *      ScriptXtoCP (USP10.@)
 *
 */
HRESULT WINAPI ScriptXtoCP(int iX,
                           int cChars,
                           int cGlyphs,
                           const WORD *pwLogClust,
                           const SCRIPT_VISATTR *psva,
                           const int *piAdvance,
                           const SCRIPT_ANALYSIS *psa,
                           int *piCP,
                           int *piTrailing)
{
    int item;
    float iPosX;
    float iLastPosX;
    int iSpecial = -1;
    int iCluster = -1;
    int clust_size = 1;
    int cjump = 0;
    int advance;
    float special_size = 0.0;
    int direction = 1;

    TRACE("(%d,%d,%d,%p,%p,%p,%p,%p,%p)\n",
          iX, cChars, cGlyphs, pwLogClust, psva, piAdvance,
          psa, piCP, piTrailing);

    if (psa->fRTL && ! psa->fLogicalOrder)
        direction = -1;

    if (direction<0)
    {
        int max_clust = pwLogClust[0];

        if (iX < 0)
        {
            *piCP = cChars;
            *piTrailing = 0;
            return S_OK;
        }

        for (item=0; item < cChars; item++)
            if (pwLogClust[item] > max_clust)
            {
                ERR("We do not handle non reversed clusters properly\n");
                break;
            }
    }

    if (iX < 0)
    {
        *piCP = -1;
        *piTrailing = 1;
        return S_OK;
    }

    iPosX = iLastPosX = 0;
    if (direction > 0)
        item = 0;
    else
        item = cChars - 1;
    for (; iPosX <= iX && item < cChars && item >= 0; item+=direction)
    {
        iLastPosX = iPosX;
        if (iSpecial == -1 &&
             (iCluster == -1 ||
              (iCluster != -1 &&
                 ((direction > 0 && iCluster+clust_size <= item) ||
                  (direction < 0 && iCluster-clust_size >= item))
              )
             )
            )
        {
            int check;
            int clust = pwLogClust[item];

            iCluster = -1;
            cjump = 0;
            clust_size = get_cluster_size(pwLogClust, cChars, item, direction,
                                          &iCluster, &check);
            advance = get_glyph_cluster_advance(piAdvance, psva, pwLogClust, cGlyphs, cChars, clust, direction);

            if (check >= cChars && direction > 0)
            {
                for (check = clust; check < cChars; check++)
                    special_size += get_glyph_cluster_advance(piAdvance, psva, pwLogClust, cGlyphs, cChars, check, direction);
                iSpecial = item;
                special_size /= (cChars - item);
                iPosX += special_size;
            }
            else
            {
                if (scriptInformation[psa->eScript].props.fNeedsCaretInfo)
                {
                    if (!cjump)
                        iPosX += advance;
                    cjump++;
                }
                else
                    iPosX += advance / (float)clust_size;
            }
        }
        else if (iSpecial != -1)
            iPosX += special_size;
        else /* (iCluster != -1) */
        {
            int adv = get_glyph_cluster_advance(piAdvance, psva, pwLogClust, cGlyphs, cChars, pwLogClust[iCluster], direction);
            if (scriptInformation[psa->eScript].props.fNeedsCaretInfo)
            {
                if (!cjump)
                    iPosX += adv;
                cjump++;
            }
            else
                iPosX += adv / (float)clust_size;
        }
    }

    if (direction > 0)
    {
        if (iPosX > iX)
            item--;
        if (item < cChars && ((iPosX - iLastPosX) / 2.0) + iX >= iPosX)
        {
            if (scriptInformation[psa->eScript].props.fNeedsCaretInfo && clust_size > 1)
                item+=(clust_size-1);
            *piTrailing = 1;
        }
        else
            *piTrailing = 0;
    }
    else
    {
        if (iX == iLastPosX)
            item++;
        if (iX >= iLastPosX && iX <= iPosX)
            item++;

        if (iLastPosX == iX)
            *piTrailing = 0;
        else if (item < 0 || ((iLastPosX - iPosX) / 2.0) + iX <= iLastPosX)
        {
            if (scriptInformation[psa->eScript].props.fNeedsCaretInfo && clust_size > 1)
                item-=(clust_size-1);
            *piTrailing = 1;
        }
        else
            *piTrailing = 0;
    }

    *piCP = item;

    TRACE("*piCP=%d\n", *piCP);
    TRACE("*piTrailing=%d\n", *piTrailing);
    return S_OK;
}

/***********************************************************************
 *      ScriptBreak (USP10.@)
 *
 *  Retrieve line break information.
 *
 *  PARAMS
 *   chars [I] Array of characters.
 *   sa    [I] String analysis.
 *   la    [I] Array of logical attribute structures.
 *
 *  RETURNS
 *   Success: S_OK
 *   Failure: S_FALSE
 */
HRESULT WINAPI ScriptBreak(const WCHAR *chars, int count, const SCRIPT_ANALYSIS *sa, SCRIPT_LOGATTR *la)
{
    TRACE("(%s, %d, %p, %p)\n", debugstr_wn(chars, count), count, sa, la);

    if (!la) return S_FALSE;

    BREAK_line(chars, count, sa, la);

    return S_OK;
}

/***********************************************************************
 *      ScriptIsComplex (USP10.@)
 *
 *  Determine if a string is complex.
 *
 *  PARAMS
 *   chars [I] Array of characters to test.
 *   len   [I] Length in characters.
 *   flag  [I] Flag.
 *
 *  RETURNS
 *   Success: S_OK
 *   Failure: S_FALSE
 *
 */
HRESULT WINAPI ScriptIsComplex(const WCHAR *chars, int len, DWORD flag)
{
    int i;

    TRACE("(%s,%d,0x%x)\n", debugstr_wn(chars, len), len, flag);

    for (i = 0; i < len; i++)
    {
        int script;

        if ((flag & SIC_ASCIIDIGIT) && chars[i] >= 0x30 && chars[i] <= 0x39)
            return S_OK;

        script = get_char_script(chars[i]);
        if ((scriptInformation[script].props.fComplex && (flag & SIC_COMPLEX))||
            (!scriptInformation[script].props.fComplex && (flag & SIC_NEUTRAL)))
            return S_OK;
    }
    return S_FALSE;
}

/***********************************************************************
 *      ScriptShapeOpenType (USP10.@)
 *
 * Produce glyphs and visual attributes for a run.
 *
 * PARAMS
 *  hdc         [I]   Device context.
 *  psc         [I/O] Opaque pointer to a script cache.
 *  psa         [I/O] Script analysis.
 *  tagScript   [I]   The OpenType tag for the Script
 *  tagLangSys  [I]   The OpenType tag for the Language
 *  rcRangeChars[I]   Array of Character counts in each range
 *  rpRangeProperties [I] Array of TEXTRANGE_PROPERTIES structures
 *  cRanges     [I]   Count of ranges
 *  pwcChars    [I]   Array of characters specifying the run.
 *  cChars      [I]   Number of characters in pwcChars.
 *  cMaxGlyphs  [I]   Length of pwOutGlyphs.
 *  pwLogClust  [O]   Array of logical cluster info.
 *  pCharProps  [O]   Array of character property values
 *  pwOutGlyphs [O]   Array of glyphs.
 *  pOutGlyphProps [O]  Array of attributes for the retrieved glyphs
 *  pcGlyphs    [O]   Number of glyphs returned.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptShapeOpenType( HDC hdc, SCRIPT_CACHE *psc,
                                    SCRIPT_ANALYSIS *psa, OPENTYPE_TAG tagScript,
                                    OPENTYPE_TAG tagLangSys, int *rcRangeChars,
                                    TEXTRANGE_PROPERTIES **rpRangeProperties,
                                    int cRanges, const WCHAR *pwcChars, int cChars,
                                    int cMaxGlyphs, WORD *pwLogClust,
                                    SCRIPT_CHARPROP *pCharProps, WORD *pwOutGlyphs,
                                    SCRIPT_GLYPHPROP *pOutGlyphProps, int *pcGlyphs)
{
    HRESULT hr;
    unsigned int i;
    BOOL rtl;

    TRACE("(%p, %p, %p, %s, %s, %p, %p, %d, %s, %d, %d, %p, %p, %p, %p, %p )\n",
     hdc, psc, psa,
     debugstr_an((char*)&tagScript,4), debugstr_an((char*)&tagLangSys,4),
     rcRangeChars, rpRangeProperties, cRanges, debugstr_wn(pwcChars, cChars),
     cChars, cMaxGlyphs, pwLogClust, pCharProps, pwOutGlyphs, pOutGlyphProps, pcGlyphs);

    if (psa) TRACE("psa values: %d, %d, %d, %d, %d, %d, %d\n", psa->eScript, psa->fRTL, psa->fLayoutRTL,
                   psa->fLinkBefore, psa->fLinkAfter, psa->fLogicalOrder, psa->fNoGlyphIndex);

    if (!pOutGlyphProps || !pcGlyphs || !pCharProps) return E_INVALIDARG;
    if (cChars > cMaxGlyphs) return E_OUTOFMEMORY;

    if (cRanges)
        FIXME("Ranges not supported yet\n");

    rtl = (psa && !psa->fLogicalOrder && psa->fRTL);

    *pcGlyphs = cChars;
    if ((hr = init_script_cache(hdc, psc)) != S_OK) return hr;
    if (!pwLogClust) return E_FAIL;

    ((ScriptCache *)*psc)->userScript = tagScript;
    ((ScriptCache *)*psc)->userLang = tagLangSys;

    /* set fNoGlyphIndex non truetype/opentype fonts */
    if (!psa->fNoGlyphIndex && !((ScriptCache *)*psc)->sfnt)
        psa->fNoGlyphIndex = TRUE;

    /* Initialize a SCRIPT_VISATTR and LogClust for each char in this run */
    for (i = 0; i < cChars; i++)
    {
        int idx = i;
        if (rtl) idx = cChars - 1 - i;
        /* FIXME: set to better values */
        pOutGlyphProps[i].sva.uJustification = (pwcChars[idx] == ' ') ? SCRIPT_JUSTIFY_BLANK : SCRIPT_JUSTIFY_CHARACTER;
        pOutGlyphProps[i].sva.fClusterStart  = 1;
        pOutGlyphProps[i].sva.fDiacritic     = 0;
        pOutGlyphProps[i].sva.fZeroWidth     = 0;
        pOutGlyphProps[i].sva.fReserved      = 0;
        pOutGlyphProps[i].sva.fShapeReserved = 0;

        /* FIXME: have the shaping engine set this */
        pCharProps[i].fCanGlyphAlone = 0;

        pwLogClust[i] = idx;
    }

    if (psa && !psa->fNoGlyphIndex)
    {
        WCHAR *rChars;
        if ((hr = SHAPE_CheckFontForRequiredFeatures(hdc, (ScriptCache *)*psc, psa)) != S_OK) return hr;

        rChars = heap_alloc(sizeof(WCHAR) * cChars);
        if (!rChars) return E_OUTOFMEMORY;
        for (i = 0; i < cChars; i++)
        {
            int idx = i;
            WCHAR chInput;
            if (rtl) idx = cChars - 1 - i;
            if (psa->fRTL)
                chInput = mirror_char(pwcChars[idx]);
            else
                chInput = pwcChars[idx];
            /* special case for tabs */
            if (chInput == 0x0009)
                chInput = 0x0020;
            if (!(pwOutGlyphs[i] = get_cache_glyph(psc, chInput)))
            {
                WORD glyph;
                if (!hdc)
                {
                    heap_free(rChars);
                    return E_PENDING;
                }
                if (GetGlyphIndicesW(hdc, &chInput, 1, &glyph, 0) == GDI_ERROR)
                {
                    heap_free(rChars);
                    return S_FALSE;
                }
                pwOutGlyphs[i] = set_cache_glyph(psc, chInput, glyph);
            }
            rChars[i] = chInput;
        }

        SHAPE_ContextualShaping(hdc, (ScriptCache *)*psc, psa, rChars, cChars, pwOutGlyphs, pcGlyphs, cMaxGlyphs, pwLogClust);
        SHAPE_ApplyDefaultOpentypeFeatures(hdc, (ScriptCache *)*psc, psa, pwOutGlyphs, pcGlyphs, cMaxGlyphs, cChars, pwLogClust);
        SHAPE_CharGlyphProp(hdc, (ScriptCache *)*psc, psa, pwcChars, cChars, pwOutGlyphs, *pcGlyphs, pwLogClust, pCharProps, pOutGlyphProps);
        heap_free(rChars);
    }
    else
    {
        TRACE("no glyph translation\n");
        for (i = 0; i < cChars; i++)
        {
            int idx = i;
            /* No mirroring done here */
            if (rtl) idx = cChars - 1 - i;
            pwOutGlyphs[i] = pwcChars[idx];
        }
    }

    return S_OK;
}


/***********************************************************************
 *      ScriptShape (USP10.@)
 *
 * Produce glyphs and visual attributes for a run.
 *
 * PARAMS
 *  hdc         [I]   Device context.
 *  psc         [I/O] Opaque pointer to a script cache.
 *  pwcChars    [I]   Array of characters specifying the run.
 *  cChars      [I]   Number of characters in pwcChars.
 *  cMaxGlyphs  [I]   Length of pwOutGlyphs.
 *  psa         [I/O] Script analysis.
 *  pwOutGlyphs [O]   Array of glyphs.
 *  pwLogClust  [O]   Array of logical cluster info.
 *  psva        [O]   Array of visual attributes.
 *  pcGlyphs    [O]   Number of glyphs returned.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptShape(HDC hdc, SCRIPT_CACHE *psc, const WCHAR *pwcChars,
                           int cChars, int cMaxGlyphs,
                           SCRIPT_ANALYSIS *psa, WORD *pwOutGlyphs, WORD *pwLogClust,
                           SCRIPT_VISATTR *psva, int *pcGlyphs)
{
    HRESULT hr;
    int i;
    SCRIPT_CHARPROP *charProps;
    SCRIPT_GLYPHPROP *glyphProps;

    if (!psva || !pcGlyphs) return E_INVALIDARG;
    if (cChars > cMaxGlyphs) return E_OUTOFMEMORY;

    charProps = heap_alloc_zero(sizeof(SCRIPT_CHARPROP)*cChars);
    if (!charProps) return E_OUTOFMEMORY;
    glyphProps = heap_alloc_zero(sizeof(SCRIPT_GLYPHPROP)*cMaxGlyphs);
    if (!glyphProps)
    {
        heap_free(charProps);
        return E_OUTOFMEMORY;
    }

    hr = ScriptShapeOpenType(hdc, psc, psa, scriptInformation[psa->eScript].scriptTag, 0, NULL, NULL, 0, pwcChars, cChars, cMaxGlyphs, pwLogClust, charProps, pwOutGlyphs, glyphProps, pcGlyphs);

    if (SUCCEEDED(hr))
    {
        for (i = 0; i < *pcGlyphs; i++)
            psva[i] = glyphProps[i].sva;
    }

    heap_free(charProps);
    heap_free(glyphProps);

    return hr;
}

/***********************************************************************
 *      ScriptPlaceOpenType (USP10.@)
 *
 * Produce advance widths for a run.
 *
 * PARAMS
 *  hdc       [I]   Device context.
 *  psc       [I/O] Opaque pointer to a script cache.
 *  psa       [I/O] String analysis.
 *  tagScript   [I]   The OpenType tag for the Script
 *  tagLangSys  [I]   The OpenType tag for the Language
 *  rcRangeChars[I]   Array of Character counts in each range
 *  rpRangeProperties [I] Array of TEXTRANGE_PROPERTIES structures
 *  cRanges     [I]   Count of ranges
 *  pwcChars    [I]   Array of characters specifying the run.
 *  pwLogClust  [I]   Array of logical cluster info
 *  pCharProps  [I]   Array of character property values
 *  cChars      [I]   Number of characters in pwcChars.
 *  pwGlyphs  [I]   Array of glyphs.
 *  pGlyphProps [I]  Array of attributes for the retrieved glyphs
 *  cGlyphs [I] Count of Glyphs
 *  piAdvance [O]   Array of advance widths.
 *  pGoffset  [O]   Glyph offsets.
 *  pABC      [O]   Combined ABC width.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */

HRESULT WINAPI ScriptPlaceOpenType( HDC hdc, SCRIPT_CACHE *psc, SCRIPT_ANALYSIS *psa,
                                    OPENTYPE_TAG tagScript, OPENTYPE_TAG tagLangSys,
                                    int *rcRangeChars, TEXTRANGE_PROPERTIES **rpRangeProperties,
                                    int cRanges, const WCHAR *pwcChars, WORD *pwLogClust,
                                    SCRIPT_CHARPROP *pCharProps, int cChars,
                                    const WORD *pwGlyphs, const SCRIPT_GLYPHPROP *pGlyphProps,
                                    int cGlyphs, int *piAdvance,
                                    GOFFSET *pGoffset, ABC *pABC
)
{
    HRESULT hr;
    int i;

    TRACE("(%p, %p, %p, %s, %s, %p, %p, %d, %s, %p, %p, %d, %p, %p, %d, %p %p %p)\n",
     hdc, psc, psa,
     debugstr_an((char*)&tagScript,4), debugstr_an((char*)&tagLangSys,4),
     rcRangeChars, rpRangeProperties, cRanges, debugstr_wn(pwcChars, cChars),
     pwLogClust, pCharProps, cChars, pwGlyphs, pGlyphProps, cGlyphs, piAdvance,
     pGoffset, pABC);

    if (!pGlyphProps) return E_INVALIDARG;
    if ((hr = init_script_cache(hdc, psc)) != S_OK) return hr;
    if (!pGoffset) return E_FAIL;

    if (cRanges)
        FIXME("Ranges not supported yet\n");

    ((ScriptCache *)*psc)->userScript = tagScript;
    ((ScriptCache *)*psc)->userLang = tagLangSys;

    if (pABC) memset(pABC, 0, sizeof(ABC));
    for (i = 0; i < cGlyphs; i++)
    {
        ABC abc;
        if (!get_cache_glyph_widths(psc, pwGlyphs[i], &abc))
        {
            if (!hdc) return E_PENDING;
            if ((get_cache_pitch_family(psc) & TMPF_TRUETYPE) && !psa->fNoGlyphIndex)
            {
                if (!GetCharABCWidthsI(hdc, 0, 1, (WORD *)&pwGlyphs[i], &abc)) return S_FALSE;
            }
            else
            {
                INT width;
                if (!GetCharWidth32W(hdc, pwGlyphs[i], pwGlyphs[i], &width)) return S_FALSE;
                abc.abcB = width;
                abc.abcA = abc.abcC = 0;
            }
            set_cache_glyph_widths(psc, pwGlyphs[i], &abc);
        }
        if (pABC)
        {
            pABC->abcA += abc.abcA;
            pABC->abcB += abc.abcB;
            pABC->abcC += abc.abcC;
        }
        /* FIXME: set to more reasonable values */
        pGoffset[i].du = pGoffset[i].dv = 0;
        if (piAdvance) piAdvance[i] = abc.abcA + abc.abcB + abc.abcC;
    }

    if (pABC) TRACE("Total for run: abcA=%d, abcB=%d, abcC=%d\n", pABC->abcA, pABC->abcB, pABC->abcC);
    return S_OK;
}

/***********************************************************************
 *      ScriptPlace (USP10.@)
 *
 * Produce advance widths for a run.
 *
 * PARAMS
 *  hdc       [I]   Device context.
 *  psc       [I/O] Opaque pointer to a script cache.
 *  pwGlyphs  [I]   Array of glyphs.
 *  cGlyphs   [I]   Number of glyphs in pwGlyphs.
 *  psva      [I]   Array of visual attributes.
 *  psa       [I/O] String analysis.
 *  piAdvance [O]   Array of advance widths.
 *  pGoffset  [O]   Glyph offsets.
 *  pABC      [O]   Combined ABC width.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptPlace(HDC hdc, SCRIPT_CACHE *psc, const WORD *pwGlyphs,
                           int cGlyphs, const SCRIPT_VISATTR *psva,
                           SCRIPT_ANALYSIS *psa, int *piAdvance, GOFFSET *pGoffset, ABC *pABC )
{
    HRESULT hr;
    SCRIPT_GLYPHPROP *glyphProps;
    int i;

    TRACE("(%p, %p, %p, %d, %p, %p, %p, %p, %p)\n",  hdc, psc, pwGlyphs, cGlyphs, psva, psa,
          piAdvance, pGoffset, pABC);

    if (!psva) return E_INVALIDARG;
    if (!pGoffset) return E_FAIL;

    glyphProps = heap_alloc(sizeof(SCRIPT_GLYPHPROP)*cGlyphs);
    if (!glyphProps) return E_OUTOFMEMORY;

    for (i = 0; i < cGlyphs; i++)
        glyphProps[i].sva = psva[i];

    hr = ScriptPlaceOpenType(hdc, psc, psa, scriptInformation[psa->eScript].scriptTag, 0, NULL, NULL, 0, NULL, NULL, NULL, 0, pwGlyphs, glyphProps, cGlyphs, piAdvance, pGoffset, pABC);

    heap_free(glyphProps);

    return hr;
}

/***********************************************************************
 *      ScriptGetCMap (USP10.@)
 *
 * Retrieve glyph indices.
 *
 * PARAMS
 *  hdc         [I]   Device context.
 *  psc         [I/O] Opaque pointer to a script cache.
 *  pwcInChars  [I]   Array of Unicode characters.
 *  cChars      [I]   Number of characters in pwcInChars.
 *  dwFlags     [I]   Flags.
 *  pwOutGlyphs [O]   Buffer to receive the array of glyph indices.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptGetCMap(HDC hdc, SCRIPT_CACHE *psc, const WCHAR *pwcInChars,
                             int cChars, DWORD dwFlags, WORD *pwOutGlyphs)
{
    HRESULT hr;
    int i;

    TRACE("(%p,%p,%s,%d,0x%x,%p)\n", hdc, psc, debugstr_wn(pwcInChars, cChars),
          cChars, dwFlags, pwOutGlyphs);

    if ((hr = init_script_cache(hdc, psc)) != S_OK) return hr;

    hr = S_OK;

    if ((get_cache_pitch_family(psc) & TMPF_TRUETYPE))
    {
        for (i = 0; i < cChars; i++)
        {
            WCHAR inChar;
            if (dwFlags == SGCM_RTL)
                inChar = mirror_char(pwcInChars[i]);
            else
                inChar = pwcInChars[i];
            if (!(pwOutGlyphs[i] = get_cache_glyph(psc, inChar)))
            {
                WORD glyph;
                if (!hdc) return E_PENDING;
                if (GetGlyphIndicesW(hdc, &inChar, 1, &glyph, GGI_MARK_NONEXISTING_GLYPHS) == GDI_ERROR) return S_FALSE;
                if (glyph == 0xffff)
                {
                    hr = S_FALSE;
                    glyph = 0x0;
                }
                pwOutGlyphs[i] = set_cache_glyph(psc, inChar, glyph);
            }
        }
    }
    else
    {
        TRACE("no glyph translation\n");
        for (i = 0; i < cChars; i++)
        {
            WCHAR inChar;
            if (dwFlags == SGCM_RTL)
                inChar = mirror_char(pwcInChars[i]);
            else
                inChar = pwcInChars[i];
            pwOutGlyphs[i] = inChar;
        }
    }
    return hr;
}

/***********************************************************************
 *      ScriptTextOut (USP10.@)
 *
 */
HRESULT WINAPI ScriptTextOut(const HDC hdc, SCRIPT_CACHE *psc, int x, int y, UINT fuOptions, 
                             const RECT *lprc, const SCRIPT_ANALYSIS *psa, const WCHAR *pwcReserved, 
                             int iReserved, const WORD *pwGlyphs, int cGlyphs, const int *piAdvance,
                             const int *piJustify, const GOFFSET *pGoffset)
{
    HRESULT hr = S_OK;

    TRACE("(%p, %p, %d, %d, %04x, %p, %p, %p, %d, %p, %d, %p, %p, %p)\n",
         hdc, psc, x, y, fuOptions, lprc, psa, pwcReserved, iReserved, pwGlyphs, cGlyphs,
         piAdvance, piJustify, pGoffset);

    if (!hdc || !psc) return E_INVALIDARG;
    if (!piAdvance || !psa || !pwGlyphs) return E_INVALIDARG;

    fuOptions &= ETO_CLIPPED + ETO_OPAQUE;
    fuOptions |= ETO_IGNORELANGUAGE;
    if  (!psa->fNoGlyphIndex)                                     /* Have Glyphs?                      */
        fuOptions |= ETO_GLYPH_INDEX;                             /* Say don't do translation to glyph */

    if (psa->fRTL && psa->fLogicalOrder)
    {
        int i;
        WORD *rtlGlyphs;

        rtlGlyphs = heap_alloc(cGlyphs * sizeof(WORD));
        if (!rtlGlyphs)
            return E_OUTOFMEMORY;

        for (i = 0; i < cGlyphs; i++)
            rtlGlyphs[i] = pwGlyphs[cGlyphs-1-i];

        if (!ExtTextOutW(hdc, x, y, fuOptions, lprc, rtlGlyphs, cGlyphs, NULL))
            hr = S_FALSE;
        heap_free(rtlGlyphs);
    }
    else
        if (!ExtTextOutW(hdc, x, y, fuOptions, lprc, pwGlyphs, cGlyphs, NULL))
            hr = S_FALSE;

    return hr;
}

/***********************************************************************
 *      ScriptCacheGetHeight (USP10.@)
 *
 * Retrieve the height of the font in the cache.
 *
 * PARAMS
 *  hdc    [I]    Device context.
 *  psc    [I/O]  Opaque pointer to a script cache.
 *  height [O]    Receives font height.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptCacheGetHeight(HDC hdc, SCRIPT_CACHE *psc, LONG *height)
{
    HRESULT hr;

    TRACE("(%p, %p, %p)\n", hdc, psc, height);

    if (!height) return E_INVALIDARG;
    if ((hr = init_script_cache(hdc, psc)) != S_OK) return hr;

    *height = get_cache_height(psc);
    return S_OK;
}

/***********************************************************************
 *      ScriptGetGlyphABCWidth (USP10.@)
 *
 * Retrieve the width of a glyph.
 *
 * PARAMS
 *  hdc    [I]    Device context.
 *  psc    [I/O]  Opaque pointer to a script cache.
 *  glyph  [I]    Glyph to retrieve the width for.
 *  abc    [O]    ABC widths of the glyph.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 */
HRESULT WINAPI ScriptGetGlyphABCWidth(HDC hdc, SCRIPT_CACHE *psc, WORD glyph, ABC *abc)
{
    HRESULT hr;

    TRACE("(%p, %p, 0x%04x, %p)\n", hdc, psc, glyph, abc);

    if (!abc) return E_INVALIDARG;
    if ((hr = init_script_cache(hdc, psc)) != S_OK) return hr;

    if (!get_cache_glyph_widths(psc, glyph, abc))
    {
        if (!hdc) return E_PENDING;
        if ((get_cache_pitch_family(psc) & TMPF_TRUETYPE))
        {
            if (!GetCharABCWidthsI(hdc, 0, 1, &glyph, abc)) return S_FALSE;
        }
        else
        {
            INT width;
            if (!GetCharWidth32W(hdc, glyph, glyph, &width)) return S_FALSE;
            abc->abcB = width;
            abc->abcA = abc->abcC = 0;
        }
        set_cache_glyph_widths(psc, glyph, abc);
    }
    return S_OK;
}

/***********************************************************************
 *      ScriptLayout (USP10.@)
 *
 * Map embedding levels to visual and/or logical order.
 *
 * PARAMS
 *  runs     [I] Size of level array.
 *  level    [I] Array of embedding levels.
 *  vistolog [O] Map of embedding levels from visual to logical order.
 *  logtovis [O] Map of embedding levels from logical to visual order.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: Non-zero HRESULT value.
 *
 * BUGS
 *  This stub works correctly for any sequence of a single
 *  embedding level but not for sequences of different
 *  embedding levels, i.e. mixtures of RTL and LTR scripts.
 */
HRESULT WINAPI ScriptLayout(int runs, const BYTE *level, int *vistolog, int *logtovis)
{
    int* indexs;
    int ich;

    TRACE("(%d, %p, %p, %p)\n", runs, level, vistolog, logtovis);

    if (!level || (!vistolog && !logtovis))
        return E_INVALIDARG;

    indexs = heap_alloc(sizeof(int) * runs);
    if (!indexs)
        return E_OUTOFMEMORY;


    if (vistolog)
    {
        for( ich = 0; ich < runs; ich++)
            indexs[ich] = ich;

        ich = 0;
        while (ich < runs)
            ich += BIDI_ReorderV2lLevel(0, indexs+ich, level+ich, runs - ich, FALSE);
        for (ich = 0; ich < runs; ich++)
            vistolog[ich] = indexs[ich];
    }


    if (logtovis)
    {
        for( ich = 0; ich < runs; ich++)
            indexs[ich] = ich;

        ich = 0;
        while (ich < runs)
            ich += BIDI_ReorderL2vLevel(0, indexs+ich, level+ich, runs - ich, FALSE);
        for (ich = 0; ich < runs; ich++)
            logtovis[ich] = indexs[ich];
    }
    heap_free(indexs);

    return S_OK;
}

/***********************************************************************
 *      ScriptStringGetLogicalWidths (USP10.@)
 *
 * Returns logical widths from a string analysis.
 *
 * PARAMS
 *  ssa  [I] string analysis.
 *  piDx [O] logical widths returned.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: a non-zero HRESULT.
 */
HRESULT WINAPI ScriptStringGetLogicalWidths(SCRIPT_STRING_ANALYSIS ssa, int *piDx)
{
    int i, j, next = 0;
    StringAnalysis *analysis = ssa;

    TRACE("%p, %p\n", ssa, piDx);

    if (!analysis) return S_FALSE;
    if (!(analysis->dwFlags & SSA_GLYPHS)) return S_FALSE;

    for (i = 0; i < analysis->numItems; i++)
    {
        int cChar = analysis->pItem[i+1].iCharPos - analysis->pItem[i].iCharPos;
        int direction = 1;

        if (analysis->pItem[i].a.fRTL && ! analysis->pItem[i].a.fLogicalOrder)
            direction = -1;

        for (j = 0; j < cChar; j++)
        {
            int k;
            int glyph = analysis->glyphs[i].pwLogClust[j];
            int clust_size = get_cluster_size(analysis->glyphs[i].pwLogClust,
                                              cChar, j, direction, NULL, NULL);
            int advance = get_glyph_cluster_advance(analysis->glyphs[i].piAdvance, analysis->glyphs[i].psva, analysis->glyphs[i].pwLogClust, analysis->glyphs[i].numGlyphs, cChar, glyph, direction);

            for (k = 0; k < clust_size; k++)
            {
                piDx[next] = advance / clust_size;
                next++;
                if (k) j++;
            }
        }
    }
    return S_OK;
}

/***********************************************************************
 *      ScriptStringValidate (USP10.@)
 *
 * Validate a string analysis.
 *
 * PARAMS
 *  ssa [I] string analysis.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: S_FALSE if invalid sequences are found
 *           or a non-zero HRESULT if it fails.
 */
HRESULT WINAPI ScriptStringValidate(SCRIPT_STRING_ANALYSIS ssa)
{
    StringAnalysis *analysis = ssa;

    TRACE("(%p)\n", ssa);

    if (!analysis) return E_INVALIDARG;
    return (analysis->invalid) ? S_FALSE : S_OK;
}

/***********************************************************************
 *      ScriptString_pSize (USP10.@)
 *
 * Retrieve width and height of an analysed string.
 *
 * PARAMS
 *  ssa [I] string analysis.
 *
 * RETURNS
 *  Success: Pointer to a SIZE structure.
 *  Failure: NULL
 */
const SIZE * WINAPI ScriptString_pSize(SCRIPT_STRING_ANALYSIS ssa)
{
    int i, j;
    StringAnalysis *analysis = ssa;

    TRACE("(%p)\n", ssa);

    if (!analysis) return NULL;
    if (!(analysis->dwFlags & SSA_GLYPHS)) return NULL;

    if (!analysis->sz)
    {
        if (!(analysis->sz = heap_alloc(sizeof(SIZE)))) return NULL;
        analysis->sz->cy = analysis->glyphs[0].sc->tm.tmHeight;

        analysis->sz->cx = 0;
        for (i = 0; i < analysis->numItems; i++)
        {
            if (analysis->glyphs[i].sc->tm.tmHeight > analysis->sz->cy)
                analysis->sz->cy = analysis->glyphs[i].sc->tm.tmHeight;
            for (j = 0; j < analysis->glyphs[i].numGlyphs; j++)
                analysis->sz->cx += analysis->glyphs[i].piAdvance[j];
        }
    }
    return analysis->sz;
}

/***********************************************************************
 *      ScriptString_pLogAttr (USP10.@)
 *
 * Retrieve logical attributes of an analysed string.
 *
 * PARAMS
 *  ssa [I] string analysis.
 *
 * RETURNS
 *  Success: Pointer to an array of SCRIPT_LOGATTR structures.
 *  Failure: NULL
 */
const SCRIPT_LOGATTR * WINAPI ScriptString_pLogAttr(SCRIPT_STRING_ANALYSIS ssa)
{
    StringAnalysis *analysis = ssa;

    TRACE("(%p)\n", ssa);

    if (!analysis) return NULL;
    if (!(analysis->dwFlags & SSA_BREAK)) return NULL;
    return analysis->logattrs;
}

/***********************************************************************
 *      ScriptString_pcOutChars (USP10.@)
 *
 * Retrieve the length of a string after clipping.
 *
 * PARAMS
 *  ssa [I] String analysis.
 *
 * RETURNS
 *  Success: Pointer to the length.
 *  Failure: NULL
 */
const int * WINAPI ScriptString_pcOutChars(SCRIPT_STRING_ANALYSIS ssa)
{
    StringAnalysis *analysis = ssa;

    TRACE("(%p)\n", ssa);

    if (!analysis) return NULL;
    return &analysis->clip_len;
}

/***********************************************************************
 *      ScriptStringGetOrder (USP10.@)
 *
 * Retrieve a glyph order map.
 *
 * PARAMS
 *  ssa   [I]   String analysis.
 *  order [I/O] Array of glyph positions.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: a non-zero HRESULT.
 */
HRESULT WINAPI ScriptStringGetOrder(SCRIPT_STRING_ANALYSIS ssa, UINT *order)
{
    int i, j;
    unsigned int k;
    StringAnalysis *analysis = ssa;

    TRACE("(%p)\n", ssa);

    if (!analysis) return S_FALSE;
    if (!(analysis->dwFlags & SSA_GLYPHS)) return S_FALSE;

    /* FIXME: handle RTL scripts */
    for (i = 0, k = 0; i < analysis->numItems; i++)
        for (j = 0; j < analysis->glyphs[i].numGlyphs; j++, k++)
            order[k] = k;

    return S_OK;
}

/***********************************************************************
 *      ScriptGetLogicalWidths (USP10.@)
 *
 * Convert advance widths to logical widths.
 *
 * PARAMS
 *  sa          [I] Script analysis.
 *  nbchars     [I] Number of characters.
 *  nbglyphs    [I] Number of glyphs.
 *  glyph_width [I] Array of glyph widths.
 *  log_clust   [I] Array of logical clusters.
 *  sva         [I] Visual attributes.
 *  widths      [O] Array of logical widths.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: a non-zero HRESULT.
 */
HRESULT WINAPI ScriptGetLogicalWidths(const SCRIPT_ANALYSIS *sa, int nbchars, int nbglyphs,
                                      const int *glyph_width, const WORD *log_clust,
                                      const SCRIPT_VISATTR *sva, int *widths)
{
    int i;

    TRACE("(%p, %d, %d, %p, %p, %p, %p)\n",
          sa, nbchars, nbglyphs, glyph_width, log_clust, sva, widths);

    /* FIXME */
    for (i = 0; i < nbchars; i++) widths[i] = glyph_width[i];
    return S_OK;
}

/***********************************************************************
 *      ScriptApplyLogicalWidth (USP10.@)
 *
 * Generate glyph advance widths.
 *
 * PARAMS
 *  dx          [I]   Array of logical advance widths.
 *  num_chars   [I]   Number of characters.
 *  num_glyphs  [I]   Number of glyphs.
 *  log_clust   [I]   Array of logical clusters.
 *  sva         [I]   Visual attributes.
 *  advance     [I]   Array of glyph advance widths.
 *  sa          [I]   Script analysis.
 *  abc         [I/O] Summed ABC widths.
 *  justify     [O]   Array of glyph advance widths.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: a non-zero HRESULT.
 */
HRESULT WINAPI ScriptApplyLogicalWidth(const int *dx, int num_chars, int num_glyphs,
                                       const WORD *log_clust, const SCRIPT_VISATTR *sva,
                                       const int *advance, const SCRIPT_ANALYSIS *sa,
                                       ABC *abc, int *justify)
{
    int i;

    FIXME("(%p, %d, %d, %p, %p, %p, %p, %p, %p)\n",
          dx, num_chars, num_glyphs, log_clust, sva, advance, sa, abc, justify);

    for (i = 0; i < num_chars; i++) justify[i] = advance[i];
    return S_OK;
}

HRESULT WINAPI ScriptJustify(const SCRIPT_VISATTR *sva, const int *advance,
                             int num_glyphs, int dx, int min_kashida, int *justify)
{
    int i;

    FIXME("(%p, %p, %d, %d, %d, %p)\n", sva, advance, num_glyphs, dx, min_kashida, justify);

    for (i = 0; i < num_glyphs; i++) justify[i] = advance[i];
    return S_OK;
}

BOOL gbLpkPresent = FALSE;
VOID WINAPI LpkPresent()
{
    gbLpkPresent = TRUE; /* Turn it on this way! Wine is out of control! */
}
