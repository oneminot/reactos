/* This file has been generated by the Perl script `afblue.pl', */
/* using data from file `afblue.dat'.                           */

/***************************************************************************/
/*                                                                         */
/*  afblue.h                                                               */
/*                                                                         */
/*    Auto-fitter data for blue strings (specification).                   */
/*                                                                         */
/*  Copyright 2013 by                                                      */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __AFBLUE_H__
#define __AFBLUE_H__


FT_BEGIN_HEADER


  /* an auxiliary macro to decode a UTF-8 character -- since we only use */
  /* hard-coded, self-converted data, no error checking is performed     */
#define GET_UTF8_CHAR( ch, p )                    \
          ch = (unsigned char)*p++;               \
          if ( ch >= 0x80 )                       \
          {                                       \
            FT_UInt  len;                         \
                                                  \
                                                  \
            if ( ch < 0xE0 )                      \
            {                                     \
              len = 1;                            \
              ch &= 0x1F;                         \
            }                                     \
            else if ( ch < 0xF0 )                 \
            {                                     \
              len = 2;                            \
              ch &= 0x0F;                         \
            }                                     \
            else                                  \
            {                                     \
              len = 3;                            \
              ch &= 0x07;                         \
            }                                     \
                                                  \
            for ( ; len > 0; len-- )              \
              ch = ( ch << 6 ) | ( *p++ & 0x3F ); \
          }


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                    B L U E   S T R I N G S                    *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /* At the bottommost level, we define strings for finding blue zones. */


#define AF_BLUE_STRING_MAX_LEN  25

  /* The AF_Blue_String enumeration values are offsets into the */
  /* `af_blue_strings' array.                                   */

  typedef enum  AF_Blue_String_
  {
    AF_BLUE_STRING_LATIN_CAPITAL_TOP = 0,
    AF_BLUE_STRING_LATIN_CAPITAL_BOTTOM = 9,
    AF_BLUE_STRING_LATIN_SMALL_F_TOP = 18,
    AF_BLUE_STRING_LATIN_SMALL = 26,
    AF_BLUE_STRING_LATIN_SMALL_DESCENDER = 34,
    AF_BLUE_STRING_GREEK_CAPITAL_TOP = 40,
    AF_BLUE_STRING_GREEK_CAPITAL_BOTTOM = 55,
    AF_BLUE_STRING_GREEK_SMALL_BETA_TOP = 68,
    AF_BLUE_STRING_GREEK_SMALL = 81,
    AF_BLUE_STRING_GREEK_SMALL_DESCENDER = 98,
    AF_BLUE_STRING_CYRILLIC_CAPITAL_TOP = 115,
    AF_BLUE_STRING_CYRILLIC_CAPITAL_BOTTOM = 132,
    AF_BLUE_STRING_CYRILLIC_SMALL = 149,
    AF_BLUE_STRING_CYRILLIC_SMALL_DESCENDER = 166,
    AF_BLUE_STRING_HEBREW_TOP = 173,
    AF_BLUE_STRING_HEBREW_BOTTOM = 190,
    AF_BLUE_STRING_HEBREW_DESCENDER = 203,
    af_blue_1_1 = 213,
#ifdef AF_CONFIG_OPTION_CJK
    AF_BLUE_STRING_CJK_TOP_FILL = af_blue_1_1 + 1,
    AF_BLUE_STRING_CJK_TOP_UNFILL = af_blue_1_1 + 77,
    AF_BLUE_STRING_CJK_BOTTOM_FILL = af_blue_1_1 + 153,
    AF_BLUE_STRING_CJK_BOTTOM_UNFILL = af_blue_1_1 + 229,
    af_blue_1_1_1 = af_blue_1_1 + 304,
#ifdef AF_CONFIG_OPTION_CJK_BLUE_HANI_VERT
    AF_BLUE_STRING_CJK_LEFT_FILL = af_blue_1_1_1 + 1,
    AF_BLUE_STRING_CJK_LEFT_UNFILL = af_blue_1_1_1 + 77,
    AF_BLUE_STRING_CJK_RIGHT_FILL = af_blue_1_1_1 + 153,
    AF_BLUE_STRING_CJK_RIGHT_UNFILL = af_blue_1_1_1 + 229,
    af_blue_1_2_1 = af_blue_1_1_1 + 304,
#else
    af_blue_1_2_1 = af_blue_1_1_1 + 0,
#endif /* AF_CONFIG_OPTION_CJK_BLUE_HANI_VERT */
    af_blue_1_2 = af_blue_1_2_1 + 0,
#else
    af_blue_1_2 = af_blue_1_2_1 + 0,
#endif /* AF_CONFIG_OPTION_CJK                */


    AF_BLUE_STRING_MAX   /* do not remove */

  } AF_Blue_String;


  FT_LOCAL_ARRAY( char )
  af_blue_strings[];


  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                 B L U E   S T R I N G S E T S                 *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/

  /* The next level is to group blue strings into script-specific sets. */


  /* Properties are specific to a writing system.  We assume that a given  */
  /* blue string can't be used in more than a single writing system, which */
  /* is a safe bet.                                                        */
#define AF_BLUE_PROPERTY_LATIN_TOP       ( 1 << 0 )
#define AF_BLUE_PROPERTY_LATIN_X_HEIGHT  ( 1 << 1 )
#define AF_BLUE_PROPERTY_LATIN_LONG      ( 1 << 2 )

#define AF_BLUE_PROPERTY_CJK_HORIZ  ( 1 << 0 )
#define AF_BLUE_PROPERTY_CJK_TOP    ( 1 << 1 )
#define AF_BLUE_PROPERTY_CJK_FILL   ( 1 << 2 )
#define AF_BLUE_PROPERTY_CJK_RIGHT  AF_BLUE_PROPERTY_CJK_TOP


#define AF_BLUE_STRINGSET_MAX_LEN  9

  /* The AF_Blue_Stringset enumeration values are offsets into the */
  /* `af_blue_stringsets' array.                                   */

  typedef enum  AF_Blue_Stringset_
  {
    AF_BLUE_STRINGSET_LATN = 0,
    AF_BLUE_STRINGSET_GREK = 7,
    AF_BLUE_STRINGSET_CYRL = 14,
    AF_BLUE_STRINGSET_HEBR = 20,
    af_blue_2_1 = 24,
#ifdef AF_CONFIG_OPTION_CJK
    AF_BLUE_STRINGSET_HANI = af_blue_2_1 + 0,
    af_blue_2_1_1 = af_blue_2_1 + 4,
#ifdef AF_CONFIG_OPTION_CJK_BLUE_HANI_VERT
    af_blue_2_2_1 = af_blue_2_1_1 + 4,
#else
    af_blue_2_2_1 = af_blue_2_1_1 + 0,
#endif /* AF_CONFIG_OPTION_CJK_BLUE_HANI_VERT */
    af_blue_2_2 = af_blue_2_2_1 + 1,
#else
    af_blue_2_2 = af_blue_2_2_1 + 0,
#endif /* AF_CONFIG_OPTION_CJK                */


    AF_BLUE_STRINGSET_MAX   /* do not remove */

  } AF_Blue_Stringset;


  typedef struct  AF_Blue_StringRec_
  {
    AF_Blue_String  string;
    FT_UShort       properties;

  } AF_Blue_StringRec;


  FT_LOCAL_ARRAY( AF_Blue_StringRec )
  af_blue_stringsets[];

/* */

FT_END_HEADER


#endif /* __AFBLUE_H__ */


/* END */
