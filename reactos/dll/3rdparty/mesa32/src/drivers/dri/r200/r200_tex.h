/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_tex.h,v 1.1 2002/10/30 12:51:53 alanh Exp $ */
/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef __R200_TEX_H__
#define __R200_TEX_H__

extern void r200SetTexOffset(__DRIcontext *pDRICtx, GLint texname,
			     unsigned long long offset, GLint depth,
			     GLuint pitch);

extern void r200UpdateTextureState( GLcontext *ctx );

extern int r200UploadTexImages( r200ContextPtr rmesa, r200TexObjPtr t, GLuint face );

extern void r200DestroyTexObj( r200ContextPtr rmesa, r200TexObjPtr t );

extern void r200InitTextureFuncs( struct dd_function_table *functions );

extern void r200UpdateFragmentShader( GLcontext *ctx );

extern void set_re_cntl_d3d( GLcontext *ctx, int unit, GLboolean use_d3d );

#endif /* __R200_TEX_H__ */
