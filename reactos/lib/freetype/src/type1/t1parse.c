/***************************************************************************/
/*                                                                         */
/*  t1parse.c                                                              */
/*                                                                         */
/*    Type 1 parser (body).                                                */
/*                                                                         */
/*  Copyright 1996-2001, 2002, 2003, 2004 by                               */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


  /*************************************************************************/
  /*                                                                       */
  /* The Type 1 parser is in charge of the following:                      */
  /*                                                                       */
  /*  - provide an implementation of a growing sequence of objects called  */
  /*    a `T1_Table' (used to build various tables needed by the loader).  */
  /*                                                                       */
  /*  - opening .pfb and .pfa files to extract their top-level and private */
  /*    dictionaries.                                                      */
  /*                                                                       */
  /*  - read numbers, arrays & strings from any dictionary.                */
  /*                                                                       */
  /* See `t1load.c' to see how data is loaded from the font file.          */
  /*                                                                       */
  /*************************************************************************/


#include <ft2build.h>
#include FT_INTERNAL_DEBUG_H
#include FT_INTERNAL_CALC_H
#include FT_INTERNAL_STREAM_H
#include FT_INTERNAL_POSTSCRIPT_AUX_H

#include "t1parse.h"

#include "t1errors.h"


  /*************************************************************************/
  /*                                                                       */
  /* The macro FT_COMPONENT is used in trace mode.  It is an implicit      */
  /* parameter of the FT_TRACE() and FT_ERROR() macros, used to print/log  */
  /* messages during execution.                                            */
  /*                                                                       */
#undef  FT_COMPONENT
#define FT_COMPONENT  trace_t1parse


  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/
  /*****                                                               *****/
  /*****                   INPUT STREAM PARSER                         *****/
  /*****                                                               *****/
  /*************************************************************************/
  /*************************************************************************/
  /*************************************************************************/


  static FT_Error
  read_pfb_tag( FT_Stream   stream,
                FT_UShort  *atag,
                FT_Long    *asize )
  {
    FT_Error   error;
    FT_UShort  tag;
    FT_Long    size;


    *atag  = 0;
    *asize = 0;

    if ( !FT_READ_USHORT( tag ) )
    {
      if ( tag == 0x8001U || tag == 0x8002U )
      {
        if ( !FT_READ_LONG_LE( size ) )
          *asize = size;
      }

      *atag = tag;
    }

    return error;
  }


  FT_LOCAL_DEF( FT_Error )
  T1_New_Parser( T1_Parser      parser,
                 FT_Stream      stream,
                 FT_Memory      memory,
                 PSAux_Service  psaux )
  {
    FT_Error   error;
    FT_UShort  tag;
    FT_Long    size;


    psaux->ps_parser_funcs->init( &parser->root, 0, 0, memory );

    parser->stream       = stream;
    parser->base_len     = 0;
    parser->base_dict    = 0;
    parser->private_len  = 0;
    parser->private_dict = 0;
    parser->in_pfb       = 0;
    parser->in_memory    = 0;
    parser->single_block = 0;

    /******************************************************************/
    /*                                                                */
    /* Here a short summary of what is going on:                      */
    /*                                                                */
    /*   When creating a new Type 1 parser, we try to locate and load */
    /*   the base dictionary if this is possible (i.e. for PFB        */
    /*   files).  Otherwise, we load the whole font into memory.      */
    /*                                                                */
    /*   When `loading' the base dictionary, we only setup pointers   */
    /*   in the case of a memory-based stream.  Otherwise, we         */
    /*   allocate and load the base dictionary in it.                 */
    /*                                                                */
    /*   parser->in_pfb is set if we are in a binary (".pfb") font.   */
    /*   parser->in_memory is set if we have a memory stream.         */
    /*                                                                */

    /* try to compute the size of the base dictionary;   */
    /* look for a Postscript binary file tag, i.e 0x8001 */
    if ( FT_STREAM_SEEK( 0L ) )
      goto Exit;

    error = read_pfb_tag( stream, &tag, &size );
    if ( error )
      goto Exit;

    if ( tag != 0x8001U )
    {
      /* assume that this is a PFA file for now; an error will */
      /* be produced later when more things are checked        */
      if ( FT_STREAM_SEEK( 0L ) )
        goto Exit;
      size = stream->size;
    }
    else
      parser->in_pfb = 1;

    /* now, try to load `size' bytes of the `base' dictionary we */
    /* found previously                                          */

    /* if it is a memory-based resource, set up pointers */
    if ( !stream->read )
    {
      parser->base_dict = (FT_Byte*)stream->base + stream->pos;
      parser->base_len  = size;
      parser->in_memory = 1;

      /* check that the `size' field is valid */
      if ( FT_STREAM_SKIP( size ) )
        goto Exit;
    }
    else
    {
      /* read segment in memory */
      if ( FT_ALLOC( parser->base_dict, size )     ||
           FT_STREAM_READ( parser->base_dict, size ) )
        goto Exit;
      parser->base_len = size;
    }

    /* Now check font format; we must see `%!PS-AdobeFont-1' */
    /* or `%!FontType'                                       */
    {
      if ( size <= 16                                       ||
           ( ft_strncmp( (const char*)parser->base_dict,
                         "%!PS-AdobeFont-1", 16 )        &&
             ft_strncmp( (const char*)parser->base_dict,
                         "%!FontType", 10 )              )  )
      {
        FT_TRACE2(( "[not a Type1 font]\n" ));
        error = T1_Err_Unknown_File_Format;
      }
      else
      {
        parser->root.base   = parser->base_dict;
        parser->root.cursor = parser->base_dict;
        parser->root.limit  = parser->root.cursor + parser->base_len;
      }
    }

  Exit:
    if ( error && !parser->in_memory )
      FT_FREE( parser->base_dict );

    return error;
  }


  FT_LOCAL_DEF( void )
  T1_Finalize_Parser( T1_Parser  parser )
  {
    FT_Memory  memory = parser->root.memory;


    /* always free the private dictionary */
    FT_FREE( parser->private_dict );

    /* free the base dictionary only when we have a disk stream */
    if ( !parser->in_memory )
      FT_FREE( parser->base_dict );

    parser->root.funcs.done( &parser->root );
  }


  FT_LOCAL_DEF( FT_Error )
  T1_Get_Private_Dict( T1_Parser      parser,
                       PSAux_Service  psaux )
  {
    FT_Stream  stream = parser->stream;
    FT_Memory  memory = parser->root.memory;
    FT_Error   error  = T1_Err_Ok;
    FT_Long    size;


    if ( parser->in_pfb )
    {
      /* in the case of the PFB format, the private dictionary can be  */
      /* made of several segments.  We thus first read the number of   */
      /* segments to compute the total size of the private dictionary  */
      /* then re-read them into memory.                                */
      FT_Long    start_pos = FT_STREAM_POS();
      FT_UShort  tag;


      parser->private_len = 0;
      for (;;)
      {
        error = read_pfb_tag( stream, &tag, &size );
        if ( error )
          goto Fail;

        if ( tag != 0x8002U )
          break;

        parser->private_len += size;

        if ( FT_STREAM_SKIP( size ) )
          goto Fail;
      }

      /* Check that we have a private dictionary there */
      /* and allocate private dictionary buffer        */
      if ( parser->private_len == 0 )
      {
        FT_ERROR(( "T1_Get_Private_Dict:" ));
        FT_ERROR(( " invalid private dictionary section\n" ));
        error = T1_Err_Invalid_File_Format;
        goto Fail;
      }

      if ( FT_STREAM_SEEK( start_pos )                             ||
           FT_ALLOC( parser->private_dict, parser->private_len ) )
        goto Fail;

      parser->private_len = 0;
      for (;;)
      {
        error = read_pfb_tag( stream, &tag, &size );
        if ( error || tag != 0x8002U )
        {
          error = T1_Err_Ok;
          break;
        }

        if ( FT_STREAM_READ( parser->private_dict + parser->private_len,
                             size ) )
          goto Fail;

        parser->private_len += size;
      }
    }
    else
    {
      /* We have already `loaded' the whole PFA font file into memory; */
      /* if this is a memory resource, allocate a new block to hold    */
      /* the private dict.  Otherwise, simply overwrite into the base  */
      /* dictionary block in the heap.                                 */

      /* first of all, look at the `eexec' keyword */
      FT_Byte*  cur   = parser->base_dict;
      FT_Byte*  limit = cur + parser->base_len;
      FT_Byte   c;


    Again:
      for (;;)
      {
        c = cur[0];
        if ( c == 'e' && cur + 9 < limit )  /* 9 = 5 letters for `eexec' + */
                                            /* newline + 4 chars           */
        {
          if ( cur[1] == 'e' &&
               cur[2] == 'x' &&
               cur[3] == 'e' &&
               cur[4] == 'c' )
            break;
        }
        cur++;
        if ( cur >= limit )
        {
          FT_ERROR(( "T1_Get_Private_Dict:" ));
          FT_ERROR(( " could not find `eexec' keyword\n" ));
          error = T1_Err_Invalid_File_Format;
          goto Exit;
        }
      }

      /* check whether `eexec' was real -- it could be in a comment */
      /* or string (as e.g. in u003043t.gsf from ghostscript)       */

      parser->root.cursor = parser->base_dict;
      parser->root.limit  = cur + 9;

      cur   = parser->root.cursor;
      limit = parser->root.limit;

      while ( cur < limit )
      {
        if ( *cur == 'e' && ft_strncmp( (char*)cur, "eexec", 5 ) == 0 )
          goto Found;

        T1_Skip_PS_Token( parser );
        T1_Skip_Spaces  ( parser );
        cur = parser->root.cursor;
      }

      /* we haven't found the correct `eexec'; go back and continue */
      /* searching                                                  */

      cur   = limit;
      limit = parser->base_dict + parser->base_len;
      goto Again;

      /* now determine where to write the _encrypted_ binary private  */
      /* dictionary.  We overwrite the base dictionary for disk-based */
      /* resources and allocate a new block otherwise                 */

    Found:
      parser->root.limit = parser->base_dict + parser->base_len;

      T1_Skip_PS_Token( parser );
      cur = parser->root.cursor;
      if ( *cur == '\r' )
      {
        cur++;
        if ( *cur == '\n' )
          cur++;
      }
      else if ( *cur == '\n' )
        cur++;
      else
      {
        FT_ERROR(( "T1_Get_Private_Dict:" ));
        FT_ERROR(( " `eexec' not properly terminated\n" ));
        error = T1_Err_Invalid_File_Format;
        goto Exit;
      }

      size = (FT_Long)( parser->base_len - ( cur - parser->base_dict ) );

      if ( parser->in_memory )
      {
        /* note that we allocate one more byte to put a terminating `0' */
        if ( FT_ALLOC( parser->private_dict, size + 1 ) )
          goto Fail;
        parser->private_len = size;
      }
      else
      {
        parser->single_block = 1;
        parser->private_dict = parser->base_dict;
        parser->private_len  = size;
        parser->base_dict    = 0;
        parser->base_len     = 0;
      }

      /* now determine whether the private dictionary is encoded in binary */
      /* or hexadecimal ASCII format -- decode it accordingly              */

      /* we need to access the next 4 bytes (after the final \r following */
      /* the `eexec' keyword); if they all are hexadecimal digits, then   */
      /* we have a case of ASCII storage                                  */

      if ( ft_isxdigit( cur[0] ) && ft_isxdigit( cur[1] ) &&
           ft_isxdigit( cur[2] ) && ft_isxdigit( cur[3] ) )
      {
        /* ASCII hexadecimal encoding */
        FT_Long  len;


        parser->root.cursor = cur;
        (void)psaux->ps_parser_funcs->to_bytes( &parser->root,
                                                parser->private_dict,
                                                parser->private_len,
                                                &len,
                                                0 );
        parser->private_len = len;

        /* put a safeguard */
        parser->private_dict[len] = '\0';
      }
      else
        /* binary encoding -- copy the private dict */
        FT_MEM_MOVE( parser->private_dict, cur, size );
    }

    /* we now decrypt the encoded binary private dictionary */
    psaux->t1_decrypt( parser->private_dict, parser->private_len, 55665U );

    /* replace the four random bytes at the beginning with whitespace */
    parser->private_dict[0] = ' ';
    parser->private_dict[1] = ' ';
    parser->private_dict[2] = ' ';
    parser->private_dict[3] = ' ';

    parser->root.base   = parser->private_dict;
    parser->root.cursor = parser->private_dict;
    parser->root.limit  = parser->root.cursor + parser->private_len;

  Fail:
  Exit:
    return error;
  }


/* END */
