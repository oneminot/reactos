/*
 * Mesa 3-D graphics library
 * Version:  7.2
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "main/glheader.h"
#include "main/bufferobj.h"
#include "main/context.h"
#include "main/enums.h"
#include "main/state.h"
#include "main/macros.h"

#include "vbo_context.h"


static void vbo_exec_debug_verts( struct vbo_exec_context *exec )
{
   GLuint count = exec->vtx.vert_count;
   GLuint i;

   _mesa_printf("%s: %u vertices %d primitives, %d vertsize\n",
		__FUNCTION__,
		count,
		exec->vtx.prim_count,
		exec->vtx.vertex_size);

   for (i = 0 ; i < exec->vtx.prim_count ; i++) {
      struct _mesa_prim *prim = &exec->vtx.prim[i];
      _mesa_printf("   prim %d: %s%s %d..%d %s %s\n",
		   i, 
		   _mesa_lookup_enum_by_nr(prim->mode),
		   prim->weak ? " (weak)" : "",
		   prim->start, 
		   prim->start + prim->count,
		   prim->begin ? "BEGIN" : "(wrap)",
		   prim->end ? "END" : "(wrap)");
   }
}


/*
 * NOTE: Need to have calculated primitives by this point -- do it on the fly.
 * NOTE: Old 'parity' issue is gone.
 */
static GLuint vbo_copy_vertices( struct vbo_exec_context *exec )
{
   GLuint nr = exec->vtx.prim[exec->vtx.prim_count-1].count;
   GLuint ovf, i;
   GLuint sz = exec->vtx.vertex_size;
   GLfloat *dst = exec->vtx.copied.buffer;
   GLfloat *src = ((GLfloat *)exec->vtx.buffer_map + 
		   exec->vtx.prim[exec->vtx.prim_count-1].start * 
		   exec->vtx.vertex_size);


   switch( exec->ctx->Driver.CurrentExecPrimitive )
   {
   case GL_POINTS:
      return 0;
   case GL_LINES:
      ovf = nr&1;
      for (i = 0 ; i < ovf ; i++)
	 _mesa_memcpy( dst+i*sz, src+(nr-ovf+i)*sz, sz * sizeof(GLfloat) );
      return i;
   case GL_TRIANGLES:
      ovf = nr%3;
      for (i = 0 ; i < ovf ; i++)
	 _mesa_memcpy( dst+i*sz, src+(nr-ovf+i)*sz, sz * sizeof(GLfloat) );
      return i;
   case GL_QUADS:
      ovf = nr&3;
      for (i = 0 ; i < ovf ; i++)
	 _mesa_memcpy( dst+i*sz, src+(nr-ovf+i)*sz, sz * sizeof(GLfloat) );
      return i;
   case GL_LINE_STRIP:
      if (nr == 0) 
	 return 0;
      else {
	 _mesa_memcpy( dst, src+(nr-1)*sz, sz * sizeof(GLfloat) );
	 return 1;
      }
   case GL_LINE_LOOP:
   case GL_TRIANGLE_FAN:
   case GL_POLYGON:
      if (nr == 0) 
	 return 0;
      else if (nr == 1) {
	 _mesa_memcpy( dst, src+0, sz * sizeof(GLfloat) );
	 return 1;
      } else {
	 _mesa_memcpy( dst, src+0, sz * sizeof(GLfloat) );
	 _mesa_memcpy( dst+sz, src+(nr-1)*sz, sz * sizeof(GLfloat) );
	 return 2;
      }
   case GL_TRIANGLE_STRIP:
      /* no parity issue, but need to make sure the tri is not drawn twice */
      if (nr & 1) {
	 exec->vtx.prim[exec->vtx.prim_count-1].count--;
      }
      /* fallthrough */
   case GL_QUAD_STRIP:
      switch (nr) {
      case 0: ovf = 0; break;
      case 1: ovf = 1; break;
      default: ovf = 2 + (nr&1); break;
      }
      for (i = 0 ; i < ovf ; i++)
	 _mesa_memcpy( dst+i*sz, src+(nr-ovf+i)*sz, sz * sizeof(GLfloat) );
      return i;
   case PRIM_OUTSIDE_BEGIN_END:
      return 0;
   default:
      assert(0);
      return 0;
   }
}



/* TODO: populate these as the vertex is defined:
 */
static void vbo_exec_bind_arrays( GLcontext *ctx )
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct gl_client_array *arrays = exec->vtx.arrays;
   GLuint count = exec->vtx.vert_count;
   GLubyte *data = exec->vtx.buffer_map;
   const GLuint *map;
   GLuint attr;

   /* Install the default (ie Current) attributes first, then overlay
    * all active ones.
    */
   switch (get_program_mode(exec->ctx)) {
   case VP_NONE:
      for (attr = 0; attr < 16; attr++) {
         exec->vtx.inputs[attr] = &vbo->legacy_currval[attr];
      }
      for (attr = 0; attr < MAT_ATTRIB_MAX; attr++) {
         exec->vtx.inputs[attr + 16] = &vbo->mat_currval[attr];
      }
      map = vbo->map_vp_none;
      break;
   case VP_NV:
   case VP_ARB:
      /* The aliasing of attributes for NV vertex programs has already
       * occurred.  NV vertex programs cannot access material values,
       * nor attributes greater than VERT_ATTRIB_TEX7.  
       */
      for (attr = 0; attr < 16; attr++) {
         exec->vtx.inputs[attr] = &vbo->legacy_currval[attr];
         exec->vtx.inputs[attr + 16] = &vbo->generic_currval[attr];
      }
      map = vbo->map_vp_arb;
      break;
   }

   /* Make all active attributes (including edgeflag) available as
    * arrays of floats.
    */
   for (attr = 0; attr < VERT_ATTRIB_MAX ; attr++) {
      const GLuint src = map[attr];

      if (exec->vtx.attrsz[src]) {
         /* override the default array set above */
         exec->vtx.inputs[attr] = &arrays[attr];

         if (exec->vtx.bufferobj->Name) {
            /* a real buffer obj: Ptr is an offset, not a pointer*/
            int offset;
            assert(exec->vtx.bufferobj->Pointer);  /* buf should be mapped */
            offset = (GLbyte *) data - (GLbyte *) exec->vtx.bufferobj->Pointer;
            assert(offset >= 0);
            arrays[attr].Ptr = (void *) offset;
         }
         else {
            /* Ptr into ordinary app memory */
            arrays[attr].Ptr = (void *) data;
         }
	 arrays[attr].Size = exec->vtx.attrsz[src];
	 arrays[attr].StrideB = exec->vtx.vertex_size * sizeof(GLfloat);
	 arrays[attr].Stride = exec->vtx.vertex_size * sizeof(GLfloat);
	 arrays[attr].Type = GL_FLOAT;
	 arrays[attr].Enabled = 1;
         _mesa_reference_buffer_object(ctx,
                                       &arrays[attr].BufferObj,
                                       exec->vtx.bufferobj);
	 arrays[attr]._MaxElement = count; /* ??? */

	 data += exec->vtx.attrsz[src] * sizeof(GLfloat);
      }
   }
}


/**
 * Execute the buffer and save copied verts.
 */
void vbo_exec_vtx_flush( struct vbo_exec_context *exec )
{
   if (0)
      vbo_exec_debug_verts( exec );


   if (exec->vtx.prim_count && 
       exec->vtx.vert_count) {

      exec->vtx.copied.nr = vbo_copy_vertices( exec ); 

      if (exec->vtx.copied.nr != exec->vtx.vert_count) {
	 GLcontext *ctx = exec->ctx;

	 GLenum target = GL_ARRAY_BUFFER_ARB;
	 GLenum access = GL_READ_WRITE_ARB;
	 GLenum usage = GL_STREAM_DRAW_ARB;
	 GLsizei size = VBO_VERT_BUFFER_SIZE * sizeof(GLfloat);
	 
	 /* Before the unmap (why?)
	  */
	 vbo_exec_bind_arrays( ctx );

         /* if using a real VBO, unmap it before drawing */
         if (exec->vtx.bufferobj->Name) {
            ctx->Driver.UnmapBuffer(ctx, target, exec->vtx.bufferobj);
            exec->vtx.buffer_map = NULL;
         }

	 vbo_context(ctx)->draw_prims( ctx, 
				       exec->vtx.inputs, 
				       exec->vtx.prim, 
				       exec->vtx.prim_count,
				       NULL,
				       0,
				       exec->vtx.vert_count - 1);

	 /* If using a real VBO, get new storage */
         if (exec->vtx.bufferobj->Name) {
            ctx->Driver.BufferData(ctx, target, size, NULL, usage, exec->vtx.bufferobj);
            exec->vtx.buffer_map = 
               ctx->Driver.MapBuffer(ctx, target, access, exec->vtx.bufferobj);
         }
      }
   }

   exec->vtx.prim_count = 0;
   exec->vtx.vert_count = 0;
   exec->vtx.vbptr = (GLfloat *)exec->vtx.buffer_map;
}
