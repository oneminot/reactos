/*
 * Pixel and vertex shaders implementation using ARB_vertex_program
 * and ARB_fragment_program GL extensions.
 *
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Ivan Gyurdiev
 * Copyright 2006 Jason Green
 * Copyright 2006 Henri Verbeet
 * Copyright 2007-2008 Stefan D�singer for CodeWeavers
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
 */

#include "config.h"

#include <math.h>
#include <stdio.h>

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);
WINE_DECLARE_DEBUG_CHANNEL(d3d_constants);
WINE_DECLARE_DEBUG_CHANNEL(d3d_caps);

#define GLINFO_LOCATION      (*gl_info)

/* Internally used shader constants. Applications can use constants 0 to GL_LIMITS(vshader_constantsF) - 1,
 * so upload them above that
 */
#define ARB_SHADER_PRIVCONST_BASE GL_LIMITS(vshader_constantsF)
#define ARB_SHADER_PRIVCONST_POS ARB_SHADER_PRIVCONST_BASE + 0

/* ARB_program_shader private data */
struct shader_arb_priv {
    GLuint                  current_vprogram_id;
    GLuint                  current_fprogram_id;
    GLuint                  depth_blt_vprogram_id;
    GLuint                  depth_blt_fprogram_id;
    BOOL                    use_arbfp_fixed_func;
    struct hash_table_t     *fragment_shaders;
};

/********************************************************
 * ARB_[vertex/fragment]_program helper functions follow
 ********************************************************/

/** 
 * Loads floating point constants into the currently set ARB_vertex/fragment_program.
 * When constant_list == NULL, it will load all the constants.
 *  
 * @target_type should be either GL_VERTEX_PROGRAM_ARB (for vertex shaders)
 *  or GL_FRAGMENT_PROGRAM_ARB (for pixel shaders)
 */
static unsigned int shader_arb_load_constantsF(IWineD3DBaseShaderImpl* This, WineD3D_GL_Info *gl_info, GLuint target_type,
        unsigned int max_constants, float* constants, char *dirty_consts) {
    local_constant* lconst;
    DWORD i, j;
    unsigned int ret;

    if (TRACE_ON(d3d_shader)) {
        for(i = 0; i < max_constants; i++) {
            if(!dirty_consts[i]) continue;
            TRACE_(d3d_constants)("Loading constants %i: %f, %f, %f, %f\n", i,
                        constants[i * 4 + 0], constants[i * 4 + 1],
                        constants[i * 4 + 2], constants[i * 4 + 3]);
        }
    }
    /* In 1.X pixel shaders constants are implicitly clamped in the range [-1;1] */
    if(target_type == GL_FRAGMENT_PROGRAM_ARB &&
       WINED3DSHADER_VERSION_MAJOR(This->baseShader.hex_version) == 1) {
        float lcl_const[4];
        for(i = 0; i < max_constants; i++) {
            if(!dirty_consts[i]) continue;
            dirty_consts[i] = 0;

            j = 4 * i;
            if(constants[j + 0] > 1.0) lcl_const[0] = 1.0;
            else if(constants[j + 0] < -1.0) lcl_const[0] = -1.0;
            else lcl_const[0] = constants[j + 0];

            if(constants[j + 1] > 1.0) lcl_const[1] = 1.0;
            else if(constants[j + 1] < -1.0) lcl_const[1] = -1.0;
            else lcl_const[1] = constants[j + 1];

            if(constants[j + 2] > 1.0) lcl_const[2] = 1.0;
            else if(constants[j + 2] < -1.0) lcl_const[2] = -1.0;
            else lcl_const[2] = constants[j + 2];

            if(constants[j + 3] > 1.0) lcl_const[3] = 1.0;
            else if(constants[j + 3] < -1.0) lcl_const[3] = -1.0;
            else lcl_const[3] = constants[j + 3];

            GL_EXTCALL(glProgramEnvParameter4fvARB(target_type, i, lcl_const));
        }
    } else {
        if(GL_SUPPORT(EXT_GPU_PROGRAM_PARAMETERS)) {
            /* TODO: Benchmark if we're better of with finding the dirty constants ourselves,
             * or just reloading *all* constants at once
             *
            GL_EXTCALL(glProgramEnvParameters4fvEXT(target_type, 0, max_constants, constants));
             */
            for(i = 0; i < max_constants; i++) {
                if(!dirty_consts[i]) continue;

                /* Find the next block of dirty constants */
                dirty_consts[i] = 0;
                j = i;
                for(i++; (i < max_constants) && dirty_consts[i]; i++) {
                    dirty_consts[i] = 0;
                }

                GL_EXTCALL(glProgramEnvParameters4fvEXT(target_type, j, i - j, constants + (j * 4)));
            }
        } else {
            for(i = 0; i < max_constants; i++) {
                if(dirty_consts[i]) {
                    dirty_consts[i] = 0;
                    GL_EXTCALL(glProgramEnvParameter4fvARB(target_type, i, constants + (i * 4)));
                }
            }
        }
    }
    checkGLcall("glProgramEnvParameter4fvARB()");

    /* Load immediate constants */
    if(This->baseShader.load_local_constsF) {
        if (TRACE_ON(d3d_shader)) {
            LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
                GLfloat* values = (GLfloat*)lconst->value;
                TRACE_(d3d_constants)("Loading local constants %i: %f, %f, %f, %f\n", lconst->idx,
                        values[0], values[1], values[2], values[3]);
            }
        }
        /* Immediate constants are clamped for 1.X shaders at loading times */
        ret = 0;
        LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
            dirty_consts[lconst->idx] = 1; /* Dirtify so the non-immediate constant overwrites it next time */
            ret = max(ret, lconst->idx + 1);
            GL_EXTCALL(glProgramEnvParameter4fvARB(target_type, lconst->idx, (GLfloat*)lconst->value));
        }
        checkGLcall("glProgramEnvParameter4fvARB()");
        return ret; /* The loaded immediate constants need reloading for the next shader */
    } else {
        return 0; /* No constants are dirty now */
    }
}

/**
 * Loads the app-supplied constants into the currently set ARB_[vertex/fragment]_programs.
 * 
 * We only support float constants in ARB at the moment, so don't 
 * worry about the Integers or Booleans
 */
static void shader_arb_load_constants(
    IWineD3DDevice* device,
    char usePixelShader,
    char useVertexShader) {
   
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) device; 
    IWineD3DStateBlockImpl* stateBlock = deviceImpl->stateBlock;
    WineD3D_GL_Info *gl_info = &deviceImpl->adapter->gl_info;
    unsigned char i;

    if (useVertexShader) {
        IWineD3DBaseShaderImpl* vshader = (IWineD3DBaseShaderImpl*) stateBlock->vertexShader;

        /* Load DirectX 9 float constants for vertex shader */
        deviceImpl->highest_dirty_vs_const = shader_arb_load_constantsF(
                vshader, gl_info, GL_VERTEX_PROGRAM_ARB,
                deviceImpl->highest_dirty_vs_const,
                stateBlock->vertexShaderConstantF,
                deviceImpl->activeContext->vshader_const_dirty);

        /* Upload the position fixup */
        GL_EXTCALL(glProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, ARB_SHADER_PRIVCONST_POS, deviceImpl->posFixup));
    }

    if (usePixelShader) {

        IWineD3DBaseShaderImpl* pshader = (IWineD3DBaseShaderImpl*) stateBlock->pixelShader;
        IWineD3DPixelShaderImpl *psi = (IWineD3DPixelShaderImpl *) pshader;

        /* Load DirectX 9 float constants for pixel shader */
        deviceImpl->highest_dirty_ps_const = shader_arb_load_constantsF(
                pshader, gl_info, GL_FRAGMENT_PROGRAM_ARB,
                deviceImpl->highest_dirty_ps_const,
                stateBlock->pixelShaderConstantF,
                deviceImpl->activeContext->pshader_const_dirty);

        for(i = 0; i < psi->numbumpenvmatconsts; i++) {
            /* The state manager takes care that this function is always called if the bump env matrix changes
             */
            float *data = (float *) &stateBlock->textureState[(int) psi->bumpenvmatconst[i].texunit][WINED3DTSS_BUMPENVMAT00];
            GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, psi->bumpenvmatconst[i].const_num, data));
            deviceImpl->activeContext->pshader_const_dirty[psi->bumpenvmatconst[i].const_num] = 1;

            if(psi->luminanceconst[i].const_num != -1) {
                /* WINED3DTSS_BUMPENVLSCALE and WINED3DTSS_BUMPENVLOFFSET are next to each other.
                 * point gl to the scale, and load 4 floats. x = scale, y = offset, z and w are junk, we
                 * don't care about them. The pointers are valid for sure because the stateblock is bigger.
                 * (they're WINED3DTSS_TEXTURETRANSFORMFLAGS and WINED3DTSS_ADDRESSW, so most likely 0 or NaN
                 */
                float *scale = (float *) &stateBlock->textureState[(int) psi->luminanceconst[i].texunit][WINED3DTSS_BUMPENVLSCALE];
                GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, psi->luminanceconst[i].const_num, scale));
                deviceImpl->activeContext->pshader_const_dirty[psi->luminanceconst[i].const_num] = 1;
            }
        }

        if(((IWineD3DPixelShaderImpl *) pshader)->srgb_enabled &&
           !((IWineD3DPixelShaderImpl *) pshader)->srgb_mode_hardcoded) {
            float comparison[4];
            float mul_low[4];

            if(stateBlock->renderState[WINED3DRS_SRGBWRITEENABLE]) {
                comparison[0] = srgb_cmp; comparison[1] = srgb_cmp;
                comparison[2] = srgb_cmp; comparison[3] = srgb_cmp;

                mul_low[0] = srgb_mul_low; mul_low[1] = srgb_mul_low;
                mul_low[2] = srgb_mul_low; mul_low[3] = srgb_mul_low;
            } else {
                comparison[0] = 1.0 / 0.0; comparison[1] = 1.0 / 0.0;
                comparison[2] = 1.0 / 0.0; comparison[3] = 1.0 / 0.0;

                mul_low[0] = 1.0; mul_low[1] = 1.0;
                mul_low[2] = 1.0; mul_low[3] = 1.0;
            }
            GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, psi->srgb_cmp_const, comparison));
            GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, psi->srgb_low_const, mul_low));
            checkGLcall("Load sRGB correction constants\n");
            deviceImpl->activeContext->pshader_const_dirty[psi->srgb_low_const] = 1;
            deviceImpl->activeContext->pshader_const_dirty[psi->srgb_cmp_const] = 1;

        }
    }
}

/* Generate the variable & register declarations for the ARB_vertex_program output target */
static void shader_generate_arb_declarations(
    IWineD3DBaseShader *iface,
    shader_reg_maps* reg_maps,
    SHADER_BUFFER* buffer,
    WineD3D_GL_Info* gl_info) {

    IWineD3DBaseShaderImpl* This = (IWineD3DBaseShaderImpl*) iface;
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) This->baseShader.device;
    DWORD i, cur;
    char pshader = shader_is_pshader_version(This->baseShader.hex_version);
    unsigned max_constantsF = min(This->baseShader.limits.constant_float, 
            (pshader ? GL_LIMITS(pshader_constantsF) : GL_LIMITS(vshader_constantsF)));
    UINT extra_constants_needed = 0;
    local_constant* lconst;

    /* Temporary Output register */
    shader_addline(buffer, "TEMP TMP_OUT;\n");

    for(i = 0; i < This->baseShader.limits.temporary; i++) {
        if (reg_maps->temporary[i])
            shader_addline(buffer, "TEMP R%u;\n", i);
    }

    for (i = 0; i < This->baseShader.limits.address; i++) {
        if (reg_maps->address[i])
            shader_addline(buffer, "ADDRESS A%d;\n", i);
    }

    for(i = 0; i < This->baseShader.limits.texcoord; i++) {
        if (reg_maps->texcoord[i])
            shader_addline(buffer,"TEMP T%u;\n", i);
    }

    /* Texture coordinate registers must be pre-loaded */
    for (i = 0; i < This->baseShader.limits.texcoord; i++) {
        if (reg_maps->texcoord[i])
            shader_addline(buffer, "MOV T%u, fragment.texcoord[%u];\n", i, i);
    }

    for(i = 0; i < (sizeof(reg_maps->bumpmat) / sizeof(reg_maps->bumpmat[0])); i++) {
        IWineD3DPixelShaderImpl *ps = (IWineD3DPixelShaderImpl *) This;
        if(!reg_maps->bumpmat[i]) continue;

        cur = ps->numbumpenvmatconsts;
        ps->bumpenvmatconst[cur].const_num = -1;
        ps->bumpenvmatconst[cur].texunit = i;
        ps->luminanceconst[cur].const_num = -1;
        ps->luminanceconst[cur].texunit = i;

        /* If the shader does not use all available constants, use the next free constant to load the bump mapping environment matrix from
         * the stateblock into the shader. If no constant is available don't load, texbem will then just sample the texture without applying
         * bump mapping.
         */
        if(max_constantsF + extra_constants_needed < GL_LIMITS(pshader_constantsF)) {
            ps->bumpenvmatconst[cur].const_num = max_constantsF + extra_constants_needed;
            shader_addline(buffer, "PARAM bumpenvmat%d = program.env[%d];\n",
                           i, ps->bumpenvmatconst[cur].const_num);
            extra_constants_needed++;

            if(reg_maps->luminanceparams && max_constantsF + extra_constants_needed < GL_LIMITS(pshader_constantsF)) {
                ((IWineD3DPixelShaderImpl *)This)->luminanceconst[cur].const_num = max_constantsF + extra_constants_needed;
                shader_addline(buffer, "PARAM luminance%d = program.env[%d];\n",
                               i, ps->luminanceconst[cur].const_num);
                extra_constants_needed++;
            } else if(reg_maps->luminanceparams) {
                FIXME("No free constant to load the luminance parameters\n");
            }
        } else {
            FIXME("No free constant found to load environemnt bump mapping matrix into the shader. texbem instruction will not apply bump mapping\n");
        }

        ps->numbumpenvmatconsts = cur + 1;
    }

    if(device->stateBlock->renderState[WINED3DRS_SRGBWRITEENABLE] && pshader) {
        IWineD3DPixelShaderImpl *ps_impl = (IWineD3DPixelShaderImpl *) This;
        /* If there are 2 constants left to use, use them to pass the sRGB correction values in. This way
         * srgb write correction can be turned on and off dynamically without recompilation. Otherwise
         * hardcode them. The drawback of hardcoding is that the shader needs recompilation to turn sRGB
         * off again
         */
        if(max_constantsF + extra_constants_needed + 1 < GL_LIMITS(pshader_constantsF) && FALSE) {
            /* The idea is that if srgb is enabled, then disabled, the constant loading code
             * can effectively disable sRGB correction by passing 1.0 and INF as the multiplication
             * and comparison constants. If it disables it that way, the shader won't be recompiled
             * and the code will stay in, so sRGB writing can be turned on again by setting the
             * constants from the spec
             */
            ps_impl->srgb_mode_hardcoded = 0;
            ps_impl->srgb_low_const = GL_LIMITS(pshader_constantsF) - extra_constants_needed;
            ps_impl->srgb_cmp_const = GL_LIMITS(pshader_constantsF) - extra_constants_needed - 1;
            shader_addline(buffer, "PARAM srgb_mul_low = program.env[%d];\n", ps_impl->srgb_low_const);
            shader_addline(buffer, "PARAM srgb_comparison = program.env[%d];\n", ps_impl->srgb_cmp_const);
        } else {
            shader_addline(buffer, "PARAM srgb_mul_low = {%f, %f, %f, 1.0};\n",
                           srgb_mul_low, srgb_mul_low, srgb_mul_low);
            shader_addline(buffer, "PARAM srgb_comparison =  {%f, %f, %f, %f};\n",
                           srgb_cmp, srgb_cmp, srgb_cmp, srgb_cmp);
            ps_impl->srgb_mode_hardcoded = 1;
        }
        /* These can be hardcoded, they do not cause any harm because no fragment will enter the high
         * path if the comparison value is set to INF
         */
        shader_addline(buffer, "PARAM srgb_pow =  {%f, %f, %f, 1.0};\n",
                       srgb_pow, srgb_pow, srgb_pow);
        shader_addline(buffer, "PARAM srgb_mul_hi =  {%f, %f, %f, 1.0};\n",
                       srgb_mul_high, srgb_mul_high, srgb_mul_high);
        shader_addline(buffer, "PARAM srgb_sub_hi =  {%f, %f, %f, 0.0};\n",
                       srgb_sub_high, srgb_sub_high, srgb_sub_high);
        ps_impl->srgb_enabled = 1;
    } else if(pshader) {
        IWineD3DPixelShaderImpl *ps_impl = (IWineD3DPixelShaderImpl *) This;

        /* Do not write any srgb fixup into the shader to save shader size and processing time.
         * As a consequence, we can't toggle srgb write on without recompilation
         */
        ps_impl->srgb_enabled = 0;
        ps_impl->srgb_mode_hardcoded = 1;
    }

    /* Load local constants using the program-local space,
     * this avoids reloading them each time the shader is used
     */
    if(!This->baseShader.load_local_constsF) {
        LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
            shader_addline(buffer, "PARAM C%u = program.local[%u];\n", lconst->idx,
                           lconst->idx);
        }
    }

    /* we use the array-based constants array if the local constants are marked for loading,
     * because then we use indirect addressing, or when the local constant list is empty,
     * because then we don't know if we're using indirect addressing or not. If we're hardcoding
     * local constants do not declare the loaded constants as an array because ARB compilers usually
     * do not optimize unused constants away
     */
    if(This->baseShader.load_local_constsF || list_empty(&This->baseShader.constantsF)) {
        /* Need to PARAM the environment parameters (constants) so we can use relative addressing */
        shader_addline(buffer, "PARAM C[%d] = { program.env[0..%d] };\n",
                    max_constantsF, max_constantsF - 1);
    } else {
        for(i = 0; i < max_constantsF; i++) {
            if(!shader_constant_is_local(This, i)) {
                shader_addline(buffer, "PARAM C%d = program.env[%d];\n",i, i);
            }
        }
    }
}

static const char * const shift_tab[] = {
    "dummy",     /*  0 (none) */
    "coefmul.x", /*  1 (x2)   */
    "coefmul.y", /*  2 (x4)   */
    "coefmul.z", /*  3 (x8)   */
    "coefmul.w", /*  4 (x16)  */
    "dummy",     /*  5 (x32)  */
    "dummy",     /*  6 (x64)  */
    "dummy",     /*  7 (x128) */
    "dummy",     /*  8 (d256) */
    "dummy",     /*  9 (d128) */
    "dummy",     /* 10 (d64)  */
    "dummy",     /* 11 (d32)  */
    "coefdiv.w", /* 12 (d16)  */
    "coefdiv.z", /* 13 (d8)   */
    "coefdiv.y", /* 14 (d4)   */
    "coefdiv.x"  /* 15 (d2)   */
};

static void shader_arb_get_write_mask(SHADER_OPCODE_ARG* arg, const DWORD param, char *write_mask) {
    IWineD3DBaseShaderImpl *This = (IWineD3DBaseShaderImpl *) arg->shader;
    char *ptr = write_mask;
    char vshader = shader_is_vshader_version(This->baseShader.hex_version);

    if(vshader && shader_get_regtype(param) == WINED3DSPR_ADDR) {
        *ptr++ = '.';
        *ptr++ = 'x';
    } else if ((param & WINED3DSP_WRITEMASK_ALL) != WINED3DSP_WRITEMASK_ALL) {
        *ptr++ = '.';
        if (param & WINED3DSP_WRITEMASK_0) *ptr++ = 'x';
        if (param & WINED3DSP_WRITEMASK_1) *ptr++ = 'y';
        if (param & WINED3DSP_WRITEMASK_2) *ptr++ = 'z';
        if (param & WINED3DSP_WRITEMASK_3) *ptr++ = 'w';
    }

    *ptr = '\0';
}

static void shader_arb_get_swizzle(const DWORD param, BOOL fixup, char *swizzle_str) {
    /* For registers of type WINED3DDECLTYPE_D3DCOLOR, data is stored as "bgra",
     * but addressed as "rgba". To fix this we need to swap the register's x
     * and z components. */
    const char *swizzle_chars = fixup ? "zyxw" : "xyzw";
    char *ptr = swizzle_str;

    /* swizzle bits fields: wwzzyyxx */
    DWORD swizzle = (param & WINED3DSP_SWIZZLE_MASK) >> WINED3DSP_SWIZZLE_SHIFT;
    DWORD swizzle_x = swizzle & 0x03;
    DWORD swizzle_y = (swizzle >> 2) & 0x03;
    DWORD swizzle_z = (swizzle >> 4) & 0x03;
    DWORD swizzle_w = (swizzle >> 6) & 0x03;

    /* If the swizzle is the default swizzle (ie, "xyzw"), we don't need to
     * generate a swizzle string. Unless we need to our own swizzling. */
    if ((WINED3DSP_NOSWIZZLE >> WINED3DSP_SWIZZLE_SHIFT) != swizzle || fixup) {
        *ptr++ = '.';
        if (swizzle_x == swizzle_y && swizzle_x == swizzle_z && swizzle_x == swizzle_w) {
            *ptr++ = swizzle_chars[swizzle_x];
        } else {
            *ptr++ = swizzle_chars[swizzle_x];
            *ptr++ = swizzle_chars[swizzle_y];
            *ptr++ = swizzle_chars[swizzle_z];
            *ptr++ = swizzle_chars[swizzle_w];
        }
    }

    *ptr = '\0';
}

static void pshader_get_register_name(IWineD3DBaseShader* iface,
    const DWORD param, char* regstr) {

    DWORD reg = param & WINED3DSP_REGNUM_MASK;
    DWORD regtype = shader_get_regtype(param);
    IWineD3DBaseShaderImpl *This = (IWineD3DBaseShaderImpl *) iface;

    switch (regtype) {
    case WINED3DSPR_TEMP:
        sprintf(regstr, "R%u", reg);
    break;
    case WINED3DSPR_INPUT:
        if (reg==0) {
            strcpy(regstr, "fragment.color.primary");
        } else {
            strcpy(regstr, "fragment.color.secondary");
        }
    break;
    case WINED3DSPR_CONST:
        if(This->baseShader.load_local_constsF || list_empty(&This->baseShader.constantsF)) {
            sprintf(regstr, "C[%u]", reg);
        } else {
            sprintf(regstr, "C%u", reg);
        }
    break;
    case WINED3DSPR_TEXTURE: /* case WINED3DSPR_ADDR: */
        sprintf(regstr,"T%u", reg);
    break;
    case WINED3DSPR_COLOROUT:
        if (reg == 0)
            sprintf(regstr, "TMP_COLOR");
        else {
            /* TODO: See GL_ARB_draw_buffers */
            FIXME("Unsupported write to render target %u\n", reg);
            sprintf(regstr, "unsupported_register");
        }
    break;
    case WINED3DSPR_DEPTHOUT:
        sprintf(regstr, "result.depth");
    break;
    case WINED3DSPR_ATTROUT:
        sprintf(regstr, "oD[%u]", reg);
    break;
    case WINED3DSPR_TEXCRDOUT:
        sprintf(regstr, "oT[%u]", reg);
    break;
    default:
        FIXME("Unhandled register name Type(%d)\n", regtype);
        sprintf(regstr, "unrecognized_register");
    break;
    }
}

/* TODO: merge with pixel shader */
static void vshader_program_add_param(SHADER_OPCODE_ARG *arg, const DWORD param, BOOL is_input, char *hwLine) {

  IWineD3DVertexShaderImpl* This = (IWineD3DVertexShaderImpl*) arg->shader;

  /* oPos, oFog and oPts in D3D */
  static const char * const hwrastout_reg_names[] = { "TMP_OUT", "result.fogcoord", "result.pointsize" };

  DWORD reg = param & WINED3DSP_REGNUM_MASK;
  DWORD regtype = shader_get_regtype(param);
  char  tmpReg[255];
  BOOL is_color = FALSE;

  if ((param & WINED3DSP_SRCMOD_MASK) == WINED3DSPSM_NEG) {
      strcat(hwLine, " -");
  } else {
      strcat(hwLine, " ");
  }

  switch (regtype) {
  case WINED3DSPR_TEMP:
    sprintf(tmpReg, "R%u", reg);
    strcat(hwLine, tmpReg);
    break;
  case WINED3DSPR_INPUT:

    if (vshader_input_is_color((IWineD3DVertexShader*) This, reg))
        is_color = TRUE;

    sprintf(tmpReg, "vertex.attrib[%u]", reg);
    strcat(hwLine, tmpReg);
    break;
  case WINED3DSPR_CONST:
      if(param & WINED3DSHADER_ADDRMODE_RELATIVE) {
          if(reg >= This->rel_offset) {
              sprintf(tmpReg, "C[A0.x + %u]", reg - This->rel_offset);
          } else {
              sprintf(tmpReg, "C[A0.x - %u]", -reg + This->rel_offset);
          }
      } else {
          if(This->baseShader.load_local_constsF || list_empty(&This->baseShader.constantsF)) {
              sprintf(tmpReg, "C[%u]", reg);
          } else {
              sprintf(tmpReg, "C%u", reg);
          }
      }
    strcat(hwLine, tmpReg);
    break;
  case WINED3DSPR_ADDR: /*case D3DSPR_TEXTURE:*/
    sprintf(tmpReg, "A%u", reg);
    strcat(hwLine, tmpReg);
    break;
  case WINED3DSPR_RASTOUT:
    sprintf(tmpReg, "%s", hwrastout_reg_names[reg]);
    strcat(hwLine, tmpReg);
    break;
  case WINED3DSPR_ATTROUT:
    if (reg==0) {
       strcat(hwLine, "result.color.primary");
    } else {
       strcat(hwLine, "result.color.secondary");
    }
    break;
  case WINED3DSPR_TEXCRDOUT:
    sprintf(tmpReg, "result.texcoord[%u]", reg);
    strcat(hwLine, tmpReg);
    break;
  default:
    FIXME("Unknown reg type %d %d\n", regtype, reg);
    strcat(hwLine, "unrecognized_register");
    break;
  }

  if (!is_input) {
    char write_mask[6];
    shader_arb_get_write_mask(arg, param, write_mask);
    strcat(hwLine, write_mask);
  } else {
    char swizzle[6];
    shader_arb_get_swizzle(param, is_color, swizzle);
    strcat(hwLine, swizzle);
  }
}

static void shader_hw_sample(SHADER_OPCODE_ARG* arg, DWORD sampler_idx, const char *dst_str, const char *coord_reg, BOOL projected, BOOL bias) {
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD sampler_type = arg->reg_maps->samplers[sampler_idx] & WINED3DSP_TEXTURETYPE_MASK;
    const char *tex_type;

    switch(sampler_type) {
        case WINED3DSTT_1D:
            tex_type = "1D";
            break;

        case WINED3DSTT_2D:
        {
            IWineD3DBaseShaderImpl *This = (IWineD3DBaseShaderImpl *) arg->shader;
            IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) This->baseShader.device;
            if(device->stateBlock->textures[sampler_idx] &&
               IWineD3DBaseTexture_GetTextureDimensions(device->stateBlock->textures[sampler_idx]) == GL_TEXTURE_RECTANGLE_ARB) {
                tex_type = "RECT";
            } else {
                tex_type = "2D";
            }
            break;
        }

        case WINED3DSTT_VOLUME:
            tex_type = "3D";
            break;

        case WINED3DSTT_CUBE:
            tex_type = "CUBE";
            break;

        default:
            ERR("Unexpected texture type %d\n", sampler_type);
            tex_type = "";
    }

    if (bias) {
        /* Shouldn't be possible, but let's check for it */
        if(projected) FIXME("Biased and Projected texture sampling\n");
        /* TXB takes the 4th component of the source vector automatically, as d3d. Nothing more to do */
        shader_addline(buffer, "TXB %s, %s, texture[%u], %s;\n", dst_str, coord_reg, sampler_idx, tex_type);
    } else if (projected) {
        shader_addline(buffer, "TXP %s, %s, texture[%u], %s;\n", dst_str, coord_reg, sampler_idx, tex_type);
    } else {
        shader_addline(buffer, "TEX %s, %s, texture[%u], %s;\n", dst_str, coord_reg, sampler_idx, tex_type);
    }
}

static void gen_color_correction(SHADER_BUFFER *buffer, const char *reg, const char *writemask,
                                 const char *one, const char *two, WINED3DFORMAT fmt,
                                 WineD3D_GL_Info *gl_info) {
    switch(fmt) {
        case WINED3DFMT_V8U8:
        case WINED3DFMT_V16U16:
            if(GL_SUPPORT(NV_TEXTURE_SHADER) ||
              (GL_SUPPORT(ATI_ENVMAP_BUMPMAP) && fmt == WINED3DFMT_V8U8)) {
                if(0) {
                    /* The 3rd channel returns 1.0 in d3d, but 0.0 in gl. Fix this while we're at it :-)
                     * disabled until an application that needs it is found because it causes unneeded
                     * shader recompilation in some game
                     */
                    if(strlen(writemask) >= 4) {
                        shader_addline(buffer, "MOV %s.%c, %s;\n", reg, writemask[3], one);
                    }
                }
            } else {
                /* Correct the sign, but leave the blue as it is - it was loaded correctly already
                 * ARB shaders are a bit picky wrt writemasks and swizzles. If we're free to scale
                 * all registers, do so, this saves an instruction.
                 */
                if(strlen(writemask) >= 5) {
                    shader_addline(buffer, "MAD %s, %s, %s, -%s;\n", reg, reg, two, one);
                } else if(strlen(writemask) >= 3) {
                    shader_addline(buffer, "MAD %s.%c, %s.%c, %s, -%s;\n",
                                   reg, writemask[1],
                                   reg, writemask[1],
                                   two, one);
                    shader_addline(buffer, "MAD %s.%c, %s.%c, %s, -%s;\n",
                                   reg, writemask[2],
                                   reg, writemask[2],
                                   two, one);
                } else if(strlen(writemask) == 2) {
                    shader_addline(buffer, "MAD %s.%c, %s.%c, %s, -%s;\n", reg, writemask[1],
                                   reg, writemask[1], two, one);
                }
            }
            break;

        case WINED3DFMT_X8L8V8U8:
            if(!GL_SUPPORT(NV_TEXTURE_SHADER)) {
                /* Red and blue are the signed channels, fix them up; Blue(=L) is correct already,
                 * and a(X) is always 1.0. Cannot do a full conversion due to L(blue)
                 */
                if(strlen(writemask) >= 3) {
                    shader_addline(buffer, "MAD %s.%c, %s.%c, %s, -%s;\n",
                                   reg, writemask[1],
                                   reg, writemask[1],
                                   two, one);
                    shader_addline(buffer, "MAD %s.%c, %s.%c, %s, -%s;\n",
                                   reg, writemask[2],
                                   reg, writemask[2],
                                   two, one);
                } else if(strlen(writemask) == 2) {
                    shader_addline(buffer, "MAD %s.%c, %s.%c, %s, -%s;\n",
                                   reg, writemask[1],
                                   reg, writemask[1],
                                   two, one);
                }
            }
            break;

        case WINED3DFMT_L6V5U5:
            if(!GL_SUPPORT(NV_TEXTURE_SHADER)) {
                if(strlen(writemask) >= 4) {
                    /* Swap y and z (U and L), and do a sign conversion on x and the new y(V and U) */
                    shader_addline(buffer, "MOV TMP.g, %s.%c;\n",
                                   reg, writemask[2]);
                    shader_addline(buffer, "MAD %s.%c%c, %s.%c%c, %s, -%s;\n",
                                   reg, writemask[1], writemask[1],
                                   reg, writemask[1], writemask[3],
                                   two, one);
                    shader_addline(buffer, "MOV %s.%c, TMP.g;\n", reg,
                                   writemask[3]);
                } else if(strlen(writemask) == 3) {
                    /* This is bad: We have VL, but we need VU */
                    FIXME("2 components sampled from a converted L6V5U5 texture\n");
                } else {
                    shader_addline(buffer, "MAD %s.%c, %s.%c, %s, -%s;\n",
                                   reg, writemask[1],
                                   reg, writemask[1],
                                   two, one);
                }
            }
            break;

        case WINED3DFMT_Q8W8V8U8:
            if(!GL_SUPPORT(NV_TEXTURE_SHADER)) {
                /* Correct the sign in all channels */
                switch(strlen(writemask)) {
                    case 4:
                        shader_addline(buffer, "MAD %s.%c, %s.%c, coefmul.x, -one;\n",
                                       reg, writemask[3],
                                       reg, writemask[3]);
                        /* drop through */
                    case 3:
                        shader_addline(buffer, "MAD %s.%c, %s.%c, coefmul.x, -one;\n",
                                       reg, writemask[2],
                                       reg, writemask[2]);
                        /* drop through */
                    case 2:
                        shader_addline(buffer, "MAD %s.%c, %s.%c, coefmul.x, -one;\n",
                                       reg, writemask[1],
                                       reg, writemask[1]);
                        break;

                        /* Should not occur, since it's at minimum '.' and a letter */
                    case 1:
                        ERR("Unexpected writemask: \"%s\"\n", writemask);
                        break;

                    case 5:
                    default:
                        shader_addline(buffer, "MAD %s, %s, coefmul.x, -one;\n", reg, reg);
                }
            }
            break;

        case WINED3DFMT_ATI2N:
            /* GL_ATI_texture_compression_3dc returns the two channels as luminance-alpha,
             * which means the first one is replicated across .rgb, and the 2nd one is in
             * .a. We need the 2nd in .g
             *
             * GL_EXT_texture_compression_rgtc returns the values in .rg, however, they
             * are swapped compared to d3d. So swap red and green.
             */
            if(GL_SUPPORT(EXT_TEXTURE_COMPRESSION_RGTC)) {
                shader_addline(buffer, "SWZ %s, %s, %c, %c, 1, 0;\n",
                               reg, reg, writemask[2], writemask[1]);
            } else {
                if(strlen(writemask) == 5) {
                    shader_addline(buffer, "MOV %s.%c, %s.%c;\n",
                                reg, writemask[2], reg, writemask[4]);
                } else if(strlen(writemask) == 2) {
                    /* Nothing to do */
                } else {
                    /* This is bad: We have VL, but we need VU */
                    FIXME("2 or 3 components sampled from a converted ATI2N texture\n");
                }
            }
            break;

            /* stupid compiler */
        default:
            break;
    }
}

static void shader_arb_color_correction(SHADER_OPCODE_ARG* arg) {
    IWineD3DBaseShaderImpl* shader = (IWineD3DBaseShaderImpl*) arg->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) shader->baseShader.device;
    WineD3D_GL_Info *gl_info = &deviceImpl->adapter->gl_info;
    WINED3DFORMAT fmt;
    WINED3DFORMAT conversion_group;
    IWineD3DBaseTextureImpl *texture;
    UINT i;
    BOOL recorded = FALSE;
    DWORD sampler_idx;
    DWORD hex_version = shader->baseShader.hex_version;
    char reg[256];
    char writemask[6];

    switch(arg->opcode->opcode) {
        case WINED3DSIO_TEX:
            if (hex_version < WINED3DPS_VERSION(2,0)) {
                sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;
            } else {
                sampler_idx = arg->src[1] & WINED3DSP_REGNUM_MASK;
            }
            break;

        case WINED3DSIO_TEXLDL:
            FIXME("Add color fixup for vertex texture WINED3DSIO_TEXLDL\n");
            return;

        case WINED3DSIO_TEXDP3TEX:
        case WINED3DSIO_TEXM3x3TEX:
        case WINED3DSIO_TEXM3x3SPEC:
        case WINED3DSIO_TEXM3x3VSPEC:
        case WINED3DSIO_TEXBEM:
        case WINED3DSIO_TEXREG2AR:
        case WINED3DSIO_TEXREG2GB:
        case WINED3DSIO_TEXREG2RGB:
            sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;
            break;

        default:
            /* Not a texture sampling instruction, nothing to do */
            return;
    };

    texture = (IWineD3DBaseTextureImpl *) deviceImpl->stateBlock->textures[sampler_idx];
    if(texture) {
        fmt = texture->resource.format;
        conversion_group = texture->baseTexture.shader_conversion_group;
    } else {
        fmt = WINED3DFMT_UNKNOWN;
        conversion_group = WINED3DFMT_UNKNOWN;
    }

    /* before doing anything, record the sampler with the format in the format conversion list,
     * but check if it's not there already
     */
    for(i = 0; i < shader->baseShader.num_sampled_samplers; i++) {
        if(shader->baseShader.sampled_samplers[i] == sampler_idx) {
            recorded = TRUE;
        }
    }
    if(!recorded) {
        shader->baseShader.sampled_samplers[shader->baseShader.num_sampled_samplers] = sampler_idx;
        shader->baseShader.num_sampled_samplers++;
        shader->baseShader.sampled_format[sampler_idx] = conversion_group;
    }

    pshader_get_register_name(arg->shader, arg->dst, reg);
    shader_arb_get_write_mask(arg, arg->dst, writemask);
    if(strlen(writemask) == 0) strcpy(writemask, ".xyzw");

    gen_color_correction(arg->buffer, reg, writemask, "one", "coefmul.x", fmt, gl_info);

}


static void pshader_gen_input_modifier_line (
    IWineD3DBaseShader *iface,
    SHADER_BUFFER* buffer,
    const DWORD instr,
    int tmpreg,
    char *outregstr) {

    /* Generate a line that does the input modifier computation and return the input register to use */
    char regstr[256];
    char swzstr[20];
    int insert_line;

    /* Assume a new line will be added */
    insert_line = 1;

    /* Get register name */
    pshader_get_register_name(iface, instr, regstr);
    shader_arb_get_swizzle(instr, FALSE, swzstr);

    switch (instr & WINED3DSP_SRCMOD_MASK) {
    case WINED3DSPSM_NONE:
        sprintf(outregstr, "%s%s", regstr, swzstr);
        insert_line = 0;
        break;
    case WINED3DSPSM_NEG:
        sprintf(outregstr, "-%s%s", regstr, swzstr);
        insert_line = 0;
        break;
    case WINED3DSPSM_BIAS:
        shader_addline(buffer, "ADD T%c, %s, -coefdiv.x;\n", 'A' + tmpreg, regstr);
        break;
    case WINED3DSPSM_BIASNEG:
        shader_addline(buffer, "ADD T%c, -%s, coefdiv.x;\n", 'A' + tmpreg, regstr);
        break;
    case WINED3DSPSM_SIGN:
        shader_addline(buffer, "MAD T%c, %s, coefmul.x, -one.x;\n", 'A' + tmpreg, regstr);
        break;
    case WINED3DSPSM_SIGNNEG:
        shader_addline(buffer, "MAD T%c, %s, -coefmul.x, one.x;\n", 'A' + tmpreg, regstr);
        break;
    case WINED3DSPSM_COMP:
        shader_addline(buffer, "SUB T%c, one.x, %s;\n", 'A' + tmpreg, regstr);
        break;
    case WINED3DSPSM_X2:
        shader_addline(buffer, "ADD T%c, %s, %s;\n", 'A' + tmpreg, regstr, regstr);
        break;
    case WINED3DSPSM_X2NEG:
        shader_addline(buffer, "ADD T%c, -%s, -%s;\n", 'A' + tmpreg, regstr, regstr);
        break;
    case WINED3DSPSM_DZ:
        shader_addline(buffer, "RCP T%c, %s.z;\n", 'A' + tmpreg, regstr);
        shader_addline(buffer, "MUL T%c, %s, T%c;\n", 'A' + tmpreg, regstr, 'A' + tmpreg);
        break;
    case WINED3DSPSM_DW:
        shader_addline(buffer, "RCP T%c, %s.w;\n", 'A' + tmpreg, regstr);
        shader_addline(buffer, "MUL T%c, %s, T%c;\n", 'A' + tmpreg, regstr, 'A' + tmpreg);
        break;
    default:
        sprintf(outregstr, "%s%s", regstr, swzstr);
        insert_line = 0;
    }

    /* Return modified or original register, with swizzle */
    if (insert_line)
        sprintf(outregstr, "T%c%s", 'A' + tmpreg, swzstr);
}

static inline void pshader_gen_output_modifier_line(
    SHADER_BUFFER* buffer,
    int saturate,
    char *write_mask,
    int shift,
    char *regstr) {

    /* Generate a line that does the output modifier computation */
    shader_addline(buffer, "MUL%s %s%s, %s, %s;\n", saturate ? "_SAT" : "",
        regstr, write_mask, regstr, shift_tab[shift]);
}

void pshader_hw_bem(SHADER_OPCODE_ARG* arg) {
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;

    SHADER_BUFFER* buffer = arg->buffer;
    char dst_name[50];
    char src_name[2][50];
    char dst_wmask[20];
    DWORD sampler_code = arg->dst & WINED3DSP_REGNUM_MASK;
    BOOL has_bumpmat = FALSE;
    int i;

    for(i = 0; i < This->numbumpenvmatconsts; i++) {
        if(This->bumpenvmatconst[i].const_num != -1 && This->bumpenvmatconst[i].texunit == sampler_code) {
            has_bumpmat = TRUE;
            break;
        }
    }

    pshader_get_register_name(arg->shader, arg->dst, dst_name);
    shader_arb_get_write_mask(arg, arg->dst, dst_wmask);
    strcat(dst_name, dst_wmask);

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src_name[0]);
    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[1], 1, src_name[1]);

    if(has_bumpmat) {
        /* Sampling the perturbation map in Tsrc was done already, including the signedness correction if needed */
        shader_addline(buffer, "SWZ TMP2, bumpenvmat%d, x, z, 0, 0;\n", sampler_code);
        shader_addline(buffer, "DP3 TMP.r, TMP2, %s;\n", src_name[1]);
        shader_addline(buffer, "SWZ TMP2, bumpenvmat%d, y, w, 0, 0;\n", sampler_code);
        shader_addline(buffer, "DP3 TMP.g, TMP2, %s;\n", src_name[1]);

        shader_addline(buffer, "ADD %s, %s, TMP;\n", dst_name, src_name[0]);
    } else {
        shader_addline(buffer, "MOV %s, %s;\n", dst_name, src_name[0]);
    }
}

void pshader_hw_cnd(SHADER_OPCODE_ARG* arg) {

    IWineD3DBaseShaderImpl* shader = (IWineD3DBaseShaderImpl*) arg->shader;
    SHADER_BUFFER* buffer = arg->buffer;
    char dst_wmask[20];
    char dst_name[50];
    char src_name[3][50];
    BOOL sat = (arg->dst & WINED3DSP_DSTMOD_MASK) & WINED3DSPDM_SATURATE;
    DWORD shift = (arg->dst & WINED3DSP_DSTSHIFT_MASK) >> WINED3DSP_DSTSHIFT_SHIFT;

    /* FIXME: support output modifiers */

    /* Handle output register */
    pshader_get_register_name(arg->shader, arg->dst, dst_name);
    shader_arb_get_write_mask(arg, arg->dst, dst_wmask);

    /* Generate input register names (with modifiers) */
    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src_name[0]);
    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[1], 1, src_name[1]);
    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[2], 2, src_name[2]);

    /* The coissue flag changes the semantic of the cnd instruction in <= 1.3 shaders */
    if (shader->baseShader.hex_version <= WINED3DPS_VERSION(1, 3) &&
        arg->opcode_token & WINED3DSI_COISSUE) {
        shader_addline(buffer, "MOV%s %s%s, %s;\n", sat ? "_SAT" : "", dst_name, dst_wmask, src_name[1]);
    } else {
        shader_addline(buffer, "ADD TMP, -%s, coefdiv.x;\n", src_name[0]);
        shader_addline(buffer, "CMP%s %s%s, TMP, %s, %s;\n",
                                sat ? "_SAT" : "", dst_name, dst_wmask, src_name[1], src_name[2]);
    }
    if (shift != 0)
        pshader_gen_output_modifier_line(buffer, FALSE, dst_wmask, shift, dst_name);
}

void pshader_hw_cmp(SHADER_OPCODE_ARG* arg) {

    SHADER_BUFFER* buffer = arg->buffer;
    char dst_wmask[20];
    char dst_name[50];
    char src_name[3][50];
    DWORD shift = (arg->dst & WINED3DSP_DSTSHIFT_MASK) >> WINED3DSP_DSTSHIFT_SHIFT;
    BOOL sat = (arg->dst & WINED3DSP_DSTMOD_MASK) & WINED3DSPDM_SATURATE;

    /* FIXME: support output modifiers */

    /* Handle output register */
    pshader_get_register_name(arg->shader, arg->dst, dst_name);
    shader_arb_get_write_mask(arg, arg->dst, dst_wmask);

    /* Generate input register names (with modifiers) */
    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src_name[0]);
    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[1], 1, src_name[1]);
    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[2], 2, src_name[2]);

    shader_addline(buffer, "CMP%s %s%s, %s, %s, %s;\n", sat ? "_SAT" : "", dst_name, dst_wmask,
                   src_name[0], src_name[2], src_name[1]);

    if (shift != 0)
        pshader_gen_output_modifier_line(buffer, FALSE, dst_wmask, shift, dst_name);
}

/** Process the WINED3DSIO_DP2ADD instruction in ARB.
 * dst = dot2(src0, src1) + src2 */
void pshader_hw_dp2add(SHADER_OPCODE_ARG* arg) {
    SHADER_BUFFER* buffer = arg->buffer;
    char dst_wmask[20];
    char dst_name[50];
    char src_name[3][50];
    DWORD shift = (arg->dst & WINED3DSP_DSTSHIFT_MASK) >> WINED3DSP_DSTSHIFT_SHIFT;
    BOOL sat = (arg->dst & WINED3DSP_DSTMOD_MASK) & WINED3DSPDM_SATURATE;

    pshader_get_register_name(arg->shader, arg->dst, dst_name);
    shader_arb_get_write_mask(arg, arg->dst, dst_wmask);

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src_name[0]);
    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[1], 1, src_name[1]);
    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[2], 2, src_name[2]);

    /* Emulate a DP2 with a DP3 and 0.0 */
    shader_addline(buffer, "MOV TMP, %s;\n", src_name[0]);
    shader_addline(buffer, "MOV TMP.z, 0.0;\n");
    shader_addline(buffer, "DP3 TMP2, TMP, %s;\n", src_name[1]);
    shader_addline(buffer, "ADD%s %s%s, TMP2, %s;\n", sat ? "_SAT" : "", dst_name, dst_wmask, src_name[2]);

    if (shift != 0)
        pshader_gen_output_modifier_line(buffer, FALSE, dst_wmask, shift, dst_name);
}

/* Map the opcode 1-to-1 to the GL code */
void pshader_hw_map2gl(SHADER_OPCODE_ARG* arg) {

     CONST SHADER_OPCODE* curOpcode = arg->opcode;
     SHADER_BUFFER* buffer = arg->buffer;
     DWORD dst = arg->dst;
     DWORD* src = arg->src;

     unsigned int i;
     char tmpLine[256];

     /* Output token related */
     char output_rname[256];
     char output_wmask[20];
     BOOL saturate = FALSE;
     BOOL centroid = FALSE;
     BOOL partialprecision = FALSE;
     DWORD shift;

     strcpy(tmpLine, curOpcode->glname);

     /* Process modifiers */
     if (0 != (dst & WINED3DSP_DSTMOD_MASK)) {
         DWORD mask = dst & WINED3DSP_DSTMOD_MASK;

         saturate = mask & WINED3DSPDM_SATURATE;
         centroid = mask & WINED3DSPDM_MSAMPCENTROID;
         partialprecision = mask & WINED3DSPDM_PARTIALPRECISION;
         mask &= ~(WINED3DSPDM_MSAMPCENTROID | WINED3DSPDM_PARTIALPRECISION | WINED3DSPDM_SATURATE);
         if (mask)
            FIXME("Unrecognized modifier(%#x)\n", mask >> WINED3DSP_DSTMOD_SHIFT);

         if (centroid)
             FIXME("Unhandled modifier(%#x)\n", mask >> WINED3DSP_DSTMOD_SHIFT);
     }
     shift = (dst & WINED3DSP_DSTSHIFT_MASK) >> WINED3DSP_DSTSHIFT_SHIFT;

      /* Generate input and output registers */
      if (curOpcode->num_params > 0) {
          char operands[4][100];

          /* Generate input register names (with modifiers) */
          for (i = 1; i < curOpcode->num_params; ++i)
              pshader_gen_input_modifier_line(arg->shader, buffer, src[i-1], i-1, operands[i]);

          /* Handle output register */
          pshader_get_register_name(arg->shader, dst, output_rname);
          strcpy(operands[0], output_rname);
          shader_arb_get_write_mask(arg, dst, output_wmask);
          strcat(operands[0], output_wmask);

          if (saturate && (shift == 0))
             strcat(tmpLine, "_SAT");
          strcat(tmpLine, " ");
          strcat(tmpLine, operands[0]);
          for (i = 1; i < curOpcode->num_params; i++) {
              strcat(tmpLine, ", ");
              strcat(tmpLine, operands[i]);
          }
          strcat(tmpLine,";\n");
          shader_addline(buffer, tmpLine);

          /* A shift requires another line. */
          if (shift != 0)
              pshader_gen_output_modifier_line(buffer, saturate, output_wmask, shift, output_rname);
      }
}

void pshader_hw_texkill(SHADER_OPCODE_ARG* arg) {
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD hex_version = This->baseShader.hex_version;
    SHADER_BUFFER* buffer = arg->buffer;
    char reg_dest[40];

    /* No swizzles are allowed in d3d's texkill. PS 1.x ignores the 4th component as documented,
     * but >= 2.0 honors it(undocumented, but tested by the d3d9 testsuit)
     */
    pshader_get_register_name(arg->shader, arg->dst, reg_dest);

    if(hex_version >= WINED3DPS_VERSION(2,0)) {
        /* The arb backend doesn't claim ps 2.0 support, but try to eat what the app feeds to us */
        shader_addline(buffer, "KIL %s;\n", reg_dest);
    } else {
        /* ARB fp doesn't like swizzles on the parameter of the KIL instruction. To mask the 4th component,
         * copy the register into our general purpose TMP variable, overwrite .w and pass TMP to KIL
         */
        shader_addline(buffer, "MOV TMP, %s;\n", reg_dest);
        shader_addline(buffer, "MOV TMP.w, one.w;\n");
        shader_addline(buffer, "KIL TMP;\n");
    }
}

void pshader_hw_tex(SHADER_OPCODE_ARG* arg) {
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;

    DWORD dst = arg->dst;
    DWORD* src = arg->src;
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD hex_version = This->baseShader.hex_version;
    BOOL projected = FALSE, bias = FALSE;

    char reg_dest[40];
    char reg_coord[40];
    DWORD reg_dest_code;
    DWORD reg_sampler_code;

    /* All versions have a destination register */
    reg_dest_code = dst & WINED3DSP_REGNUM_MASK;
    pshader_get_register_name(arg->shader, dst, reg_dest);

    /* 1.0-1.3: Use destination register as coordinate source.
       1.4+: Use provided coordinate source register. */
   if (hex_version < WINED3DPS_VERSION(1,4))
      strcpy(reg_coord, reg_dest);
   else
      pshader_gen_input_modifier_line(arg->shader, buffer, src[0], 0, reg_coord);

  /* 1.0-1.4: Use destination register number as texture code.
     2.0+: Use provided sampler number as texure code. */
  if (hex_version < WINED3DPS_VERSION(2,0))
     reg_sampler_code = reg_dest_code;
  else
     reg_sampler_code = src[1] & WINED3DSP_REGNUM_MASK;

  /* projection flag:
   * 1.1, 1.2, 1.3: Use WINED3DTSS_TEXTURETRANSFORMFLAGS
   * 1.4: Use WINED3DSPSM_DZ or WINED3DSPSM_DW on src[0]
   * 2.0+: Use WINED3DSI_TEXLD_PROJECT on the opcode
   */
  if(hex_version < WINED3DPS_VERSION(1,4)) {
      DWORD flags = 0;
      if(reg_sampler_code < MAX_TEXTURES) {
        flags = deviceImpl->stateBlock->textureState[reg_sampler_code][WINED3DTSS_TEXTURETRANSFORMFLAGS];
      }
      if (flags & WINED3DTTFF_PROJECTED) {
          projected = TRUE;
      }
  } else if(hex_version < WINED3DPS_VERSION(2,0)) {
      DWORD src_mod = arg->src[0] & WINED3DSP_SRCMOD_MASK;
      if (src_mod == WINED3DSPSM_DZ) {
          projected = TRUE;
      } else if(src_mod == WINED3DSPSM_DW) {
          projected = TRUE;
      }
  } else {
      if(arg->opcode_token & WINED3DSI_TEXLD_PROJECT) {
          projected = TRUE;
      }
      if(arg->opcode_token & WINED3DSI_TEXLD_BIAS) {
          bias = TRUE;
      }
  }
  shader_hw_sample(arg, reg_sampler_code, reg_dest, reg_coord, projected, bias);
}

void pshader_hw_texcoord(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD dst = arg->dst;
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD hex_version = This->baseShader.hex_version;

    char tmp[20];
    shader_arb_get_write_mask(arg, dst, tmp);
    if (hex_version != WINED3DPS_VERSION(1,4)) {
        DWORD reg = dst & WINED3DSP_REGNUM_MASK;
        shader_addline(buffer, "MOV_SAT T%u%s, fragment.texcoord[%u];\n", reg, tmp, reg);
    } else {
        DWORD reg1 = dst & WINED3DSP_REGNUM_MASK;
        char reg_src[40];

        pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, reg_src);
        shader_addline(buffer, "MOV R%u%s, %s;\n", reg1, tmp, reg_src);
   }
}

void pshader_hw_texreg2ar(SHADER_OPCODE_ARG* arg) {

     SHADER_BUFFER* buffer = arg->buffer;
     IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
     IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
     DWORD flags;

     DWORD reg1 = arg->dst & WINED3DSP_REGNUM_MASK;
     char dst_str[8];
     char src_str[50];

     sprintf(dst_str, "T%u", reg1);
     pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src_str);
     shader_addline(buffer, "MOV TMP.r, %s.a;\n", src_str);
     shader_addline(buffer, "MOV TMP.g, %s.r;\n", src_str);
     flags = reg1 < MAX_TEXTURES ? deviceImpl->stateBlock->textureState[reg1][WINED3DTSS_TEXTURETRANSFORMFLAGS] : 0;
     shader_hw_sample(arg, reg1, dst_str, "TMP", flags & WINED3DTTFF_PROJECTED, FALSE);
}

void pshader_hw_texreg2gb(SHADER_OPCODE_ARG* arg) {

     SHADER_BUFFER* buffer = arg->buffer;

     DWORD reg1 = arg->dst & WINED3DSP_REGNUM_MASK;
     char dst_str[8];
     char src_str[50];

     sprintf(dst_str, "T%u", reg1);
     pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src_str);
     shader_addline(buffer, "MOV TMP.r, %s.g;\n", src_str);
     shader_addline(buffer, "MOV TMP.g, %s.b;\n", src_str);
     shader_hw_sample(arg, reg1, dst_str, "TMP", FALSE, FALSE);
}

void pshader_hw_texreg2rgb(SHADER_OPCODE_ARG* arg) {

    SHADER_BUFFER* buffer = arg->buffer;
    DWORD reg1 = arg->dst & WINED3DSP_REGNUM_MASK;
    char dst_str[8];
    char src_str[50];

    sprintf(dst_str, "T%u", reg1);
    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src_str);
    shader_hw_sample(arg, reg1, dst_str, src_str, FALSE, FALSE);
}

void pshader_hw_texbem(SHADER_OPCODE_ARG* arg) {
    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    BOOL has_bumpmat = FALSE;
    BOOL has_luminance = FALSE;
    int i;

    DWORD dst = arg->dst;
    DWORD src = arg->src[0] & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;

    char reg_coord[40];
    DWORD reg_dest_code;

    /* All versions have a destination register */
    reg_dest_code = dst & WINED3DSP_REGNUM_MASK;
    /* Can directly use the name because texbem is only valid for <= 1.3 shaders */
    pshader_get_register_name(arg->shader, dst, reg_coord);

    for(i = 0; i < This->numbumpenvmatconsts; i++) {
        if(This->bumpenvmatconst[i].const_num != -1 && reg_dest_code == This->bumpenvmatconst[i].texunit) {
            has_bumpmat = TRUE;
            break;
        }
    }
    for(i = 0; i < This->numbumpenvmatconsts; i++) {
        if(This->luminanceconst[i].const_num != -1 && reg_dest_code == This->luminanceconst[i].texunit) {
            has_luminance = TRUE;
            break;
        }
    }

    if(has_bumpmat) {
        /* Sampling the perturbation map in Tsrc was done already, including the signedness correction if needed */

        shader_addline(buffer, "SWZ TMP2, bumpenvmat%d, x, z, 0, 0;\n", reg_dest_code);
        shader_addline(buffer, "DP3 TMP.r, TMP2, T%u;\n", src);
        shader_addline(buffer, "SWZ TMP2, bumpenvmat%d, y, w, 0, 0;\n", reg_dest_code);
        shader_addline(buffer, "DP3 TMP.g, TMP2, T%u;\n", src);

        /* with projective textures, texbem only divides the static texture coord, not the displacement,
         * so we can't let the GL handle this.
         */
        if (((IWineD3DDeviceImpl*) This->baseShader.device)->stateBlock->textureState[reg_dest_code][WINED3DTSS_TEXTURETRANSFORMFLAGS]
              & WINED3DTTFF_PROJECTED) {
            shader_addline(buffer, "RCP TMP2.a, %s.a;\n", reg_coord);
            shader_addline(buffer, "MUL TMP2.rg, %s, TMP2.a;\n", reg_coord);
            shader_addline(buffer, "ADD TMP.rg, TMP, TMP2;\n");
        } else {
            shader_addline(buffer, "ADD TMP.rg, TMP, %s;\n", reg_coord);
        }

        shader_hw_sample(arg, reg_dest_code, reg_coord, "TMP", FALSE, FALSE);

        if(arg->opcode->opcode == WINED3DSIO_TEXBEML && has_luminance) {
            shader_addline(buffer, "MAD TMP, T%u.z, luminance%d.x, luminance%d.y;\n",
                           src, reg_dest_code, reg_dest_code);
            shader_addline(buffer, "MUL %s, %s, TMP;\n", reg_coord, reg_coord);
        }

    } else {
        DWORD tf;
        if(reg_dest_code < MAX_TEXTURES) {
            tf = ((IWineD3DDeviceImpl*) This->baseShader.device)->stateBlock->textureState[reg_dest_code][WINED3DTSS_TEXTURETRANSFORMFLAGS];
        } else {
            tf = 0;
        }
        /* Without a bump matrix loaded, just sample with the unmodified coordinates */
        shader_hw_sample(arg, reg_dest_code, reg_coord, reg_coord, tf & WINED3DTTFF_PROJECTED, FALSE);
    }
}

void pshader_hw_texm3x2pad(SHADER_OPCODE_ARG* arg) {

    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    char src0_name[50];

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 TMP.x, T%u, %s;\n", reg, src0_name);
}

void pshader_hw_texm3x2tex(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
    DWORD flags;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    char dst_str[8];
    char src0_name[50];

    sprintf(dst_str, "T%u", reg);
    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 TMP.y, T%u, %s;\n", reg, src0_name);
    flags = reg < MAX_TEXTURES ? deviceImpl->stateBlock->textureState[reg][WINED3DTSS_TEXTURETRANSFORMFLAGS] : 0;
    shader_hw_sample(arg, reg, dst_str, "TMP", flags & WINED3DTTFF_PROJECTED, FALSE);
}

void pshader_hw_texm3x3pad(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE* current_state = &This->baseShader.parse_state;
    char src0_name[50];

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 TMP.%c, T%u, %s;\n", 'x' + current_state->current_row, reg, src0_name);
    current_state->texcoord_w[current_state->current_row++] = reg;
}

void pshader_hw_texm3x3tex(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
    DWORD flags;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE* current_state = &This->baseShader.parse_state;
    char dst_str[8];
    char src0_name[50];

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 TMP.z, T%u, %s;\n", reg, src0_name);

    /* Sample the texture using the calculated coordinates */
    sprintf(dst_str, "T%u", reg);
    flags = reg < MAX_TEXTURES ? deviceImpl->stateBlock->textureState[reg][WINED3DTSS_TEXTURETRANSFORMFLAGS] : 0;
    shader_hw_sample(arg, reg, dst_str, "TMP", flags & WINED3DTTFF_PROJECTED, FALSE);
    current_state->current_row = 0;
}

void pshader_hw_texm3x3vspec(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
    DWORD flags;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;
    SHADER_PARSE_STATE* current_state = &This->baseShader.parse_state;
    char dst_str[8];
    char src0_name[50];

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 TMP.z, T%u, %s;\n", reg, src0_name);

    /* Construct the eye-ray vector from w coordinates */
    shader_addline(buffer, "MOV TMP2.x, fragment.texcoord[%u].w;\n", current_state->texcoord_w[0]);
    shader_addline(buffer, "MOV TMP2.y, fragment.texcoord[%u].w;\n", current_state->texcoord_w[1]);
    shader_addline(buffer, "MOV TMP2.z, fragment.texcoord[%u].w;\n", reg);

    /* Calculate reflection vector
     */
    shader_addline(buffer, "DP3 TMP.w, TMP, TMP2;\n");
    /* The .w is ignored when sampling, so I can use TMP2.w to calculate dot(N, N) */
    shader_addline(buffer, "DP3 TMP2.w, TMP, TMP;\n");
    shader_addline(buffer, "RCP TMP2.w, TMP2.w;\n");
    shader_addline(buffer, "MUL TMP.w, TMP.w, TMP2.w;\n");
    shader_addline(buffer, "MUL TMP, TMP.w, TMP;\n");
    shader_addline(buffer, "MAD TMP, coefmul.x, TMP, -TMP2;\n");

    /* Sample the texture using the calculated coordinates */
    sprintf(dst_str, "T%u", reg);
    flags = reg < MAX_TEXTURES ? deviceImpl->stateBlock->textureState[reg][WINED3DTSS_TEXTURETRANSFORMFLAGS] : 0;
    shader_hw_sample(arg, reg, dst_str, "TMP", flags & WINED3DTTFF_PROJECTED, FALSE);
    current_state->current_row = 0;
}

void pshader_hw_texm3x3spec(SHADER_OPCODE_ARG* arg) {

    IWineD3DPixelShaderImpl* This = (IWineD3DPixelShaderImpl*) arg->shader;
    IWineD3DDeviceImpl* deviceImpl = (IWineD3DDeviceImpl*) This->baseShader.device;
    DWORD flags;
    DWORD reg = arg->dst & WINED3DSP_REGNUM_MASK;
    DWORD reg3 = arg->src[1] & WINED3DSP_REGNUM_MASK;
    SHADER_PARSE_STATE* current_state = &This->baseShader.parse_state;
    SHADER_BUFFER* buffer = arg->buffer;
    char dst_str[8];
    char src0_name[50];

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src0_name);
    shader_addline(buffer, "DP3 TMP.z, T%u, %s;\n", reg, src0_name);

    /* Calculate reflection vector.
     *
     *               dot(N, E)
     * TMP.xyz = 2 * --------- * N - E
     *               dot(N, N)
     *
     * Which normalizes the normal vector
     */
    shader_addline(buffer, "DP3 TMP.w, TMP, C[%u];\n", reg3);
    shader_addline(buffer, "DP3 TMP2.w, TMP, TMP;\n");
    shader_addline(buffer, "RCP TMP2.w, TMP2.w;\n");
    shader_addline(buffer, "MUL TMP.w, TMP.w, TMP2.w;\n");
    shader_addline(buffer, "MUL TMP, TMP.w, TMP;\n");
    shader_addline(buffer, "MAD TMP, coefmul.x, TMP, -C[%u];\n", reg3);

    /* Sample the texture using the calculated coordinates */
    sprintf(dst_str, "T%u", reg);
    flags = reg < MAX_TEXTURES ? deviceImpl->stateBlock->textureState[reg][WINED3DTSS_TEXTURETRANSFORMFLAGS] : 0;
    shader_hw_sample(arg, reg, dst_str, "TMP", flags & WINED3DTTFF_PROJECTED, FALSE);
    current_state->current_row = 0;
}

void pshader_hw_texdepth(SHADER_OPCODE_ARG* arg) {
    SHADER_BUFFER* buffer = arg->buffer;
    char dst_name[50];

    /* texdepth has an implicit destination, the fragment depth value. It's only parameter,
     * which is essentially an input, is the destination register because it is the first
     * parameter. According to the msdn, this must be register r5, but let's keep it more flexible
     * here
     */
    pshader_get_register_name(arg->shader, arg->dst, dst_name);

    /* According to the msdn, the source register(must be r5) is unusable after
     * the texdepth instruction, so we're free to modify it
     */
    shader_addline(buffer, "MIN %s.g, %s.g, one.g;\n", dst_name, dst_name);

    /* How to deal with the special case dst_name.g == 0? if r != 0, then
     * the r * (1 / 0) will give infinity, which is clamped to 1.0, the correct
     * result. But if r = 0.0, then 0 * inf = 0, which is incorrect.
     */
    shader_addline(buffer, "RCP %s.g, %s.g;\n", dst_name, dst_name);
    shader_addline(buffer, "MUL TMP.x, %s.r, %s.g;\n", dst_name, dst_name);
    shader_addline(buffer, "MIN TMP.x, TMP.x, one.r;\n");
    shader_addline(buffer, "MAX result.depth, TMP.x, 0.0;\n");
}

/** Process the WINED3DSIO_TEXDP3TEX instruction in ARB:
 * Take a 3-component dot product of the TexCoord[dstreg] and src,
 * then perform a 1D texture lookup from stage dstregnum, place into dst. */
void pshader_hw_texdp3tex(SHADER_OPCODE_ARG* arg) {
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD sampler_idx = arg->dst & WINED3DSP_REGNUM_MASK;
    char src0[50];
    char dst_str[8];

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src0);
    shader_addline(buffer, "MOV TMP, 0.0;\n");
    shader_addline(buffer, "DP3 TMP.x, T%u, %s;\n", sampler_idx, src0);

    sprintf(dst_str, "T%u", sampler_idx);
    shader_hw_sample(arg, sampler_idx, dst_str, "TMP", FALSE /* Only one coord, can't be projected */, FALSE);
}

/** Process the WINED3DSIO_TEXDP3 instruction in ARB:
 * Take a 3-component dot product of the TexCoord[dstreg] and src. */
void pshader_hw_texdp3(SHADER_OPCODE_ARG* arg) {
    char src0[50];
    char dst_str[50];
    char dst_mask[6];
    DWORD dstreg = arg->dst & WINED3DSP_REGNUM_MASK;
    SHADER_BUFFER* buffer = arg->buffer;

    /* Handle output register */
    pshader_get_register_name(arg->shader, arg->dst, dst_str);
    shader_arb_get_write_mask(arg, arg->dst, dst_mask);

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src0);
    shader_addline(buffer, "DP3 %s%s, T%u, %s;\n", dst_str, dst_mask, dstreg, src0);

    /* TODO: Handle output modifiers */
}

/** Process the WINED3DSIO_TEXM3X3 instruction in ARB
 * Perform the 3rd row of a 3x3 matrix multiply */
void pshader_hw_texm3x3(SHADER_OPCODE_ARG* arg) {
    SHADER_BUFFER* buffer = arg->buffer;
    char dst_str[50];
    char dst_mask[6];
    char src0[50];
    DWORD dst_reg = arg->dst & WINED3DSP_REGNUM_MASK;

    pshader_get_register_name(arg->shader, arg->dst, dst_str);
    shader_arb_get_write_mask(arg, arg->dst, dst_mask);

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src0);
    shader_addline(buffer, "DP3 TMP.z, T%u, %s;\n", dst_reg, src0);
    shader_addline(buffer, "MOV %s%s, TMP;\n", dst_str, dst_mask);

    /* TODO: Handle output modifiers */
}

/** Process the WINED3DSIO_TEXM3X2DEPTH instruction in ARB:
 * Last row of a 3x2 matrix multiply, use the result to calculate the depth:
 * Calculate tmp0.y = TexCoord[dstreg] . src.xyz;  (tmp0.x has already been calculated)
 * depth = (tmp0.y == 0.0) ? 1.0 : tmp0.x / tmp0.y
 */
void pshader_hw_texm3x2depth(SHADER_OPCODE_ARG* arg) {
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD dst_reg = arg->dst & WINED3DSP_REGNUM_MASK;
    char src0[50];

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src0);
    shader_addline(buffer, "DP3 TMP.y, T%u, %s;\n", dst_reg, src0);

    /* How to deal with the special case dst_name.g == 0? if r != 0, then
     * the r * (1 / 0) will give infinity, which is clamped to 1.0, the correct
     * result. But if r = 0.0, then 0 * inf = 0, which is incorrect.
     */
    shader_addline(buffer, "RCP TMP.y, TMP.y;\n");
    shader_addline(buffer, "MUL TMP.x, TMP.x, TMP.y;\n");
    shader_addline(buffer, "MIN TMP.x, TMP.x, one.r;\n");
    shader_addline(buffer, "MAX result.depth, TMP.x, 0.0;\n");
}

/** Handles transforming all WINED3DSIO_M?x? opcodes for
    Vertex/Pixel shaders to ARB_vertex_program codes */
void shader_hw_mnxn(SHADER_OPCODE_ARG* arg) {

    int i;
    int nComponents = 0;
    SHADER_OPCODE_ARG tmpArg;

    memset(&tmpArg, 0, sizeof(SHADER_OPCODE_ARG));

    /* Set constants for the temporary argument */
    tmpArg.shader      = arg->shader;
    tmpArg.buffer      = arg->buffer;
    tmpArg.src[0]      = arg->src[0];
    tmpArg.src_addr[0] = arg->src_addr[0];
    tmpArg.src_addr[1] = arg->src_addr[1];
    tmpArg.reg_maps = arg->reg_maps;

    switch(arg->opcode->opcode) {
    case WINED3DSIO_M4x4:
        nComponents = 4;
        tmpArg.opcode = shader_get_opcode(arg->shader, WINED3DSIO_DP4);
        break;
    case WINED3DSIO_M4x3:
        nComponents = 3;
        tmpArg.opcode = shader_get_opcode(arg->shader, WINED3DSIO_DP4);
        break;
    case WINED3DSIO_M3x4:
        nComponents = 4;
        tmpArg.opcode = shader_get_opcode(arg->shader, WINED3DSIO_DP3);
        break;
    case WINED3DSIO_M3x3:
        nComponents = 3;
        tmpArg.opcode = shader_get_opcode(arg->shader, WINED3DSIO_DP3);
        break;
    case WINED3DSIO_M3x2:
        nComponents = 2;
        tmpArg.opcode = shader_get_opcode(arg->shader, WINED3DSIO_DP3);
        break;
    default:
        break;
    }

    for (i = 0; i < nComponents; i++) {
        tmpArg.dst = ((arg->dst) & ~WINED3DSP_WRITEMASK_ALL)|(WINED3DSP_WRITEMASK_0<<i);
        tmpArg.src[1] = arg->src[1]+i;
        vshader_hw_map2gl(&tmpArg);
    }
}

void vshader_hw_rsq_rcp(SHADER_OPCODE_ARG* arg) {
    CONST SHADER_OPCODE* curOpcode = arg->opcode;
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD dst = arg->dst;
    DWORD src = arg->src[0];
    DWORD swizzle = (src & WINED3DSP_SWIZZLE_MASK) >> WINED3DSP_SWIZZLE_SHIFT;

    char tmpLine[256];

    strcpy(tmpLine, curOpcode->glname); /* Opcode */
    vshader_program_add_param(arg, dst, FALSE, tmpLine); /* Destination */
    strcat(tmpLine, ",");
    vshader_program_add_param(arg, src, TRUE, tmpLine);
    if ((WINED3DSP_NOSWIZZLE >> WINED3DSP_SWIZZLE_SHIFT) == swizzle) {
        /* Dx sdk says .x is used if no swizzle is given, but our test shows that
         * .w is used
         */
        strcat(tmpLine, ".w");
    }

    shader_addline(buffer, "%s;\n", tmpLine);
}

void shader_hw_nrm(SHADER_OPCODE_ARG* arg) {
    SHADER_BUFFER* buffer = arg->buffer;
    char dst_name[50];
    char src_name[50];
    char dst_wmask[20];
    DWORD shift = (arg->dst & WINED3DSP_DSTSHIFT_MASK) >> WINED3DSP_DSTSHIFT_SHIFT;
    BOOL sat = (arg->dst & WINED3DSP_DSTMOD_MASK) & WINED3DSPDM_SATURATE;

    pshader_get_register_name(arg->shader, arg->dst, dst_name);
    shader_arb_get_write_mask(arg, arg->dst, dst_wmask);

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src_name);
    shader_addline(buffer, "DP3 TMP, %s, %s;\n", src_name, src_name);
    shader_addline(buffer, "RSQ TMP, TMP.x;\n");
    /* dst.w = src[0].w * 1 / (src.x^2 + src.y^2 + src.z^2)^(1/2) according to msdn*/
    shader_addline(buffer, "MUL%s %s%s, %s, TMP;\n", sat ? "_SAT" : "", dst_name, dst_wmask,
                   src_name);

    if (shift != 0)
        pshader_gen_output_modifier_line(buffer, FALSE, dst_wmask, shift, dst_name);
}

void shader_hw_sincos(SHADER_OPCODE_ARG* arg) {
    /* This instruction exists in ARB, but the d3d instruction takes two extra parameters which
     * must contain fixed constants. So we need a separate function to filter those constants and
     * can't use map2gl
     */
    SHADER_BUFFER* buffer = arg->buffer;
    char dst_name[50];
    char src_name[50];
    char dst_wmask[20];
    DWORD shift = (arg->dst & WINED3DSP_DSTSHIFT_MASK) >> WINED3DSP_DSTSHIFT_SHIFT;
    BOOL sat = (arg->dst & WINED3DSP_DSTMOD_MASK) & WINED3DSPDM_SATURATE;

    pshader_get_register_name(arg->shader, arg->dst, dst_name);
    shader_arb_get_write_mask(arg, arg->dst, dst_wmask);

    pshader_gen_input_modifier_line(arg->shader, buffer, arg->src[0], 0, src_name);
    shader_addline(buffer, "SCS%s %s%s, %s;\n", sat ? "_SAT" : "", dst_name, dst_wmask,
                   src_name);

    if (shift != 0)
        pshader_gen_output_modifier_line(buffer, FALSE, dst_wmask, shift, dst_name);

}

/* TODO: merge with pixel shader */
/* Map the opcode 1-to-1 to the GL code */
void vshader_hw_map2gl(SHADER_OPCODE_ARG* arg) {

    IWineD3DVertexShaderImpl *shader = (IWineD3DVertexShaderImpl*) arg->shader;
    CONST SHADER_OPCODE* curOpcode = arg->opcode;
    SHADER_BUFFER* buffer = arg->buffer;
    DWORD dst = arg->dst;
    DWORD* src = arg->src;

    DWORD dst_regtype = shader_get_regtype(dst);
    char tmpLine[256];
    unsigned int i;

    if ((curOpcode->opcode == WINED3DSIO_MOV && dst_regtype == WINED3DSPR_ADDR) || curOpcode->opcode == WINED3DSIO_MOVA) {
        memset(tmpLine, 0, sizeof(tmpLine));
        if(shader->rel_offset) {
            vshader_program_add_param(arg, src[0], TRUE, tmpLine);
            shader_addline(buffer, "ADD TMP.x, %s, helper_const.z;\n", tmpLine);
            shader_addline(buffer, "ARL A0.x, TMP.x;\n");
        } else {
            /* Apple's ARB_vertex_program implementation does not accept an ARL source argument
             * with more than one component. Thus replicate the first source argument over all
             * 4 components. For example, .xyzw -> .x (or better: .xxxx), .zwxy -> .z, etc)
             */
            DWORD parm = src[0] & ~(WINED3DVS_SWIZZLE_MASK);
                   if((src[0] & WINED3DVS_X_W) == WINED3DVS_X_W) {
                parm |= WINED3DVS_X_W | WINED3DVS_Y_W | WINED3DVS_Z_W | WINED3DVS_W_W;
            } else if((src[0] & WINED3DVS_X_Z) == WINED3DVS_X_Z) {
                parm |= WINED3DVS_X_Z | WINED3DVS_Y_Z | WINED3DVS_Z_Z | WINED3DVS_W_Z;
            } else if((src[0] & WINED3DVS_X_Y) == WINED3DVS_X_Y) {
                parm |= WINED3DVS_X_Y | WINED3DVS_Y_Y | WINED3DVS_Z_Y | WINED3DVS_W_Y;
            } else if((src[0] & WINED3DVS_X_X) == WINED3DVS_X_X) {
                parm |= WINED3DVS_X_X | WINED3DVS_Y_X | WINED3DVS_Z_X | WINED3DVS_W_X;
            }
            vshader_program_add_param(arg, parm, TRUE, tmpLine);
            shader_addline(buffer, "ARL A0.x, %s;\n", tmpLine);
        }
        return;
    } else
        strcpy(tmpLine, curOpcode->glname);

    if (curOpcode->num_params > 0) {
        vshader_program_add_param(arg, dst, FALSE, tmpLine);
        for (i = 1; i < curOpcode->num_params; ++i) {
           strcat(tmpLine, ",");
           vshader_program_add_param(arg, src[i-1], TRUE, tmpLine);
        }
    }
   shader_addline(buffer, "%s;\n", tmpLine);
}

static GLuint create_arb_blt_vertex_program(WineD3D_GL_Info *gl_info) {
    GLuint program_id = 0;
    const char *blt_vprogram =
        "!!ARBvp1.0\n"
        "PARAM c[1] = { { 1, 0.5 } };\n"
        "MOV result.position, vertex.position;\n"
        "MOV result.color, c[0].x;\n"
        "MAD result.texcoord[0].y, -vertex.position, c[0], c[0];\n"
        "MAD result.texcoord[0].x, vertex.position, c[0].y, c[0].y;\n"
        "END\n";

    GL_EXTCALL(glGenProgramsARB(1, &program_id));
    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, program_id));
    GL_EXTCALL(glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(blt_vprogram), blt_vprogram));

    if (glGetError() == GL_INVALID_OPERATION) {
        GLint pos;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &pos);
        FIXME("Vertex program error at position %d: %s\n", pos,
            debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
    }

    return program_id;
}

static GLuint create_arb_blt_fragment_program(WineD3D_GL_Info *gl_info) {
    GLuint program_id = 0;
    const char *blt_fprogram =
        "!!ARBfp1.0\n"
        "TEMP R0;\n"
        "TEX R0.x, fragment.texcoord[0], texture[0], 2D;\n"
        "MOV result.depth.z, R0.x;\n"
        "END\n";

    GL_EXTCALL(glGenProgramsARB(1, &program_id));
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, program_id));
    GL_EXTCALL(glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(blt_fprogram), blt_fprogram));

    if (glGetError() == GL_INVALID_OPERATION) {
        GLint pos;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &pos);
        FIXME("Fragment program error at position %d: %s\n", pos,
            debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
    }

    return program_id;
}

static void shader_arb_select(IWineD3DDevice *iface, BOOL usePS, BOOL useVS) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    struct shader_arb_priv *priv = (struct shader_arb_priv *) This->shader_priv;
    WineD3D_GL_Info *gl_info = &This->adapter->gl_info;

    if (useVS) {
        TRACE("Using vertex shader\n");

        priv->current_vprogram_id = ((IWineD3DVertexShaderImpl *)This->stateBlock->vertexShader)->baseShader.prgId;

        /* Bind the vertex program */
        GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, priv->current_vprogram_id));
        checkGLcall("glBindProgramARB(GL_VERTEX_PROGRAM_ARB, vertexShader->prgId);");

        /* Enable OpenGL vertex programs */
        glEnable(GL_VERTEX_PROGRAM_ARB);
        checkGLcall("glEnable(GL_VERTEX_PROGRAM_ARB);");
        TRACE("(%p) : Bound vertex program %u and enabled GL_VERTEX_PROGRAM_ARB\n", This, priv->current_vprogram_id);
    } else if(GL_SUPPORT(ARB_VERTEX_PROGRAM)) {
        priv->current_vprogram_id = 0;
        glDisable(GL_VERTEX_PROGRAM_ARB);
        checkGLcall("glDisable(GL_VERTEX_PROGRAM_ARB)");
    }

    if (usePS) {
        TRACE("Using pixel shader\n");

        priv->current_fprogram_id = ((IWineD3DPixelShaderImpl *)This->stateBlock->pixelShader)->baseShader.prgId;

        /* Bind the fragment program */
        GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, priv->current_fprogram_id));
        checkGLcall("glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, pixelShader->prgId);");

        /* Enable OpenGL fragment programs */
        glEnable(GL_FRAGMENT_PROGRAM_ARB);
        checkGLcall("glEnable(GL_FRAGMENT_PROGRAM_ARB);");
        TRACE("(%p) : Bound fragment program %u and enabled GL_FRAGMENT_PROGRAM_ARB\n", This, priv->current_fprogram_id);
    } else {
        priv->current_fprogram_id = 0;

        if(GL_SUPPORT(ARB_FRAGMENT_PROGRAM) && !priv->use_arbfp_fixed_func) {
            /* Disable only if we're not using arbfp fixed function fragment processing. If this is used,
             * keep GL_FRAGMENT_PROGRAM_ARB enabled, and the fixed function pipeline will bind the fixed function
             * replacement shader
             */
            glDisable(GL_FRAGMENT_PROGRAM_ARB);
            checkGLcall("glDisable(GL_FRAGMENT_PROGRAM_ARB)");
        }
    }
}

static void shader_arb_select_depth_blt(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    struct shader_arb_priv *priv = (struct shader_arb_priv *) This->shader_priv;
    WineD3D_GL_Info *gl_info = &This->adapter->gl_info;

    if (!priv->depth_blt_vprogram_id) priv->depth_blt_vprogram_id = create_arb_blt_vertex_program(gl_info);
    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, priv->depth_blt_vprogram_id));
    glEnable(GL_VERTEX_PROGRAM_ARB);

    if (!priv->depth_blt_fprogram_id) priv->depth_blt_fprogram_id = create_arb_blt_fragment_program(gl_info);
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, priv->depth_blt_fprogram_id));
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
}

static void shader_arb_deselect_depth_blt(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    struct shader_arb_priv *priv = (struct shader_arb_priv *) This->shader_priv;
    WineD3D_GL_Info *gl_info = &This->adapter->gl_info;

    if (priv->current_vprogram_id) {
        GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, priv->current_vprogram_id));
        checkGLcall("glBindProgramARB(GL_VERTEX_PROGRAM_ARB, vertexShader->prgId);");

        glEnable(GL_VERTEX_PROGRAM_ARB);
        checkGLcall("glEnable(GL_VERTEX_PROGRAM_ARB);");

        TRACE("(%p) : Bound vertex program %u and enabled GL_VERTEX_PROGRAM_ARB\n", This, priv->current_vprogram_id);
    } else {
        glDisable(GL_VERTEX_PROGRAM_ARB);
        checkGLcall("glDisable(GL_VERTEX_PROGRAM_ARB)");
    }

    if (priv->current_fprogram_id) {
        GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, priv->current_fprogram_id));
        checkGLcall("glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, pixelShader->prgId);");

        glEnable(GL_FRAGMENT_PROGRAM_ARB);
        checkGLcall("glEnable(GL_FRAGMENT_PROGRAM_ARB);");

        TRACE("(%p) : Bound fragment program %u and enabled GL_FRAGMENT_PROGRAM_ARB\n", This, priv->current_fprogram_id);
    } else if(!priv->use_arbfp_fixed_func) {
        glDisable(GL_FRAGMENT_PROGRAM_ARB);
        checkGLcall("glDisable(GL_FRAGMENT_PROGRAM_ARB)");
    }
}

static void shader_arb_cleanup(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WineD3D_GL_Info *gl_info = &This->adapter->gl_info;
    if (GL_SUPPORT(ARB_VERTEX_PROGRAM)) glDisable(GL_VERTEX_PROGRAM_ARB);
    if (GL_SUPPORT(ARB_FRAGMENT_PROGRAM)) glDisable(GL_FRAGMENT_PROGRAM_ARB);
}

static void shader_arb_destroy(IWineD3DBaseShader *iface) {
    IWineD3DBaseShaderImpl *This = (IWineD3DBaseShaderImpl *) iface;
    WineD3D_GL_Info *gl_info = &((IWineD3DDeviceImpl *) This->baseShader.device)->adapter->gl_info;

    ENTER_GL();
    GL_EXTCALL(glDeleteProgramsARB(1, &This->baseShader.prgId));
    checkGLcall("GL_EXTCALL(glDeleteProgramsARB(1, &This->baseShader.prgId))");
    LEAVE_GL();
    This->baseShader.prgId = 0;
    This->baseShader.is_compiled = FALSE;
}

static HRESULT shader_arb_alloc(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    This->shader_priv = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct shader_arb_priv));
    return WINED3D_OK;
}

static void shader_arb_free(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WineD3D_GL_Info *gl_info = &This->adapter->gl_info;
    struct shader_arb_priv *priv = (struct shader_arb_priv *) This->shader_priv;

    if(priv->depth_blt_vprogram_id) {
        GL_EXTCALL(glDeleteProgramsARB(1, &priv->depth_blt_vprogram_id));
    }
    if(priv->depth_blt_fprogram_id) {
        GL_EXTCALL(glDeleteProgramsARB(1, &priv->depth_blt_fprogram_id));
    }

    HeapFree(GetProcessHeap(), 0, This->shader_priv);
}

static BOOL shader_arb_dirty_const(IWineD3DDevice *iface) {
    return TRUE;
}

static void arbfp_add_sRGB_correction(SHADER_BUFFER *buffer, const char *fragcolor, const char *tmp1,
                                      const char *tmp2, const char *tmp3, const char *tmp4) {
    /* Perform sRGB write correction. See GLX_EXT_framebuffer_sRGB */

    /* Calculate the > 0.0031308 case */
    shader_addline(buffer, "POW %s.x, %s.x, srgb_pow.x;\n", tmp1, fragcolor);
    shader_addline(buffer, "POW %s.y, %s.y, srgb_pow.y;\n", tmp1, fragcolor);
    shader_addline(buffer, "POW %s.z, %s.z, srgb_pow.z;\n", tmp1, fragcolor);
    shader_addline(buffer, "MUL %s, %s, srgb_mul_hi;\n", tmp1, tmp1);
    shader_addline(buffer, "SUB %s, %s, srgb_sub_hi;\n", tmp1, tmp1);
    /* Calculate the < case */
    shader_addline(buffer, "MUL %s, srgb_mul_low, %s;\n", tmp2, fragcolor);
    /* Get 1.0 / 0.0 masks for > 0.0031308 and < 0.0031308 */
    shader_addline(buffer, "SLT %s, srgb_comparison, %s;\n", tmp3, fragcolor);
    shader_addline(buffer, "SGE %s, srgb_comparison, %s;\n", tmp4, fragcolor);
    /* Store the components > 0.0031308 in the destination */
    shader_addline(buffer, "MUL %s, %s, %s;\n", fragcolor, tmp1, tmp3);
    /* Add the components that are < 0.0031308 */
    shader_addline(buffer, "MAD result.color.xyz, %s, %s, %s;\n", tmp2, tmp4, fragcolor);
    /* [0.0;1.0] clamping. Not needed, this is done implicitly */
}

static void shader_arb_generate_pshader(IWineD3DPixelShader *iface, SHADER_BUFFER *buffer) {
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    shader_reg_maps* reg_maps = &This->baseShader.reg_maps;
    CONST DWORD *function = This->baseShader.function;
    const char *fragcolor;
    WineD3D_GL_Info *gl_info = &((IWineD3DDeviceImpl *)This->baseShader.device)->adapter->gl_info;
    local_constant* lconst;

    /*  Create the hw ARB shader */
    shader_addline(buffer, "!!ARBfp1.0\n");

    shader_addline(buffer, "TEMP TMP;\n");     /* Used in matrix ops */
    shader_addline(buffer, "TEMP TMP2;\n");    /* Used in matrix ops */
    shader_addline(buffer, "TEMP TA;\n");      /* Used for modifiers */
    shader_addline(buffer, "TEMP TB;\n");      /* Used for modifiers */
    shader_addline(buffer, "TEMP TC;\n");      /* Used for modifiers */
    shader_addline(buffer, "PARAM coefdiv = { 0.5, 0.25, 0.125, 0.0625 };\n");
    shader_addline(buffer, "PARAM coefmul = { 2, 4, 8, 16 };\n");
    shader_addline(buffer, "PARAM one = { 1.0, 1.0, 1.0, 1.0 };\n");

    /* Base Declarations */
    shader_generate_arb_declarations( (IWineD3DBaseShader*) This, reg_maps, buffer, &GLINFO_LOCATION);

    /* We need two variables for fog blending */
    shader_addline(buffer, "TEMP TMP_FOG;\n");
    if (This->baseShader.hex_version >= WINED3DPS_VERSION(2,0)) {
        shader_addline(buffer, "TEMP TMP_COLOR;\n");
    }

    /* Base Shader Body */
    shader_generate_main( (IWineD3DBaseShader*) This, buffer, reg_maps, function);

    /* calculate fog and blend it
     * NOTE: state.fog.params.y and state.fog.params.z don't hold fog start s and end e but
     * -1/(e-s) and e/(e-s) respectively.
     */
    shader_addline(buffer, "MAD_SAT TMP_FOG, fragment.fogcoord, state.fog.params.y, state.fog.params.z;\n");

    if (This->baseShader.hex_version < WINED3DPS_VERSION(2,0)) {
        fragcolor = "R0";
    } else {
        fragcolor = "TMP_COLOR";
    }
    if(This->srgb_enabled) {
        arbfp_add_sRGB_correction(buffer, fragcolor, "TMP", "TMP2", "TA", "TB");
    }
    if (This->baseShader.hex_version < WINED3DPS_VERSION(3,0)) {
        shader_addline(buffer, "LRP result.color.rgb, TMP_FOG.x, %s, state.fog.color;\n", fragcolor);
        shader_addline(buffer, "MOV result.color.a, %s.a;\n", fragcolor);
    }

    shader_addline(buffer, "END\n");

    /* TODO: change to resource.glObjectHandle or something like that */
    GL_EXTCALL(glGenProgramsARB(1, &This->baseShader.prgId));

    TRACE("Creating a hw pixel shader, prg=%d\n", This->baseShader.prgId);
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, This->baseShader.prgId));

    TRACE("Created hw pixel shader, prg=%d\n", This->baseShader.prgId);
    /* Create the program and check for errors */
    GL_EXTCALL(glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
               buffer->bsize, buffer->buffer));

    if (glGetError() == GL_INVALID_OPERATION) {
        GLint errPos;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
        FIXME("HW PixelShader Error at position %d: %s\n",
              errPos, debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
        This->baseShader.prgId = -1;
    }

    /* Load immediate constants */
    if(!This->baseShader.load_local_constsF) {
        LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
            float *value = (float *) lconst->value;
            GL_EXTCALL(glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, lconst->idx, value));
            checkGLcall("glProgramLocalParameter4fvARB");
        }
    }
}

static void shader_arb_generate_vshader(IWineD3DVertexShader *iface, SHADER_BUFFER *buffer) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    shader_reg_maps* reg_maps = &This->baseShader.reg_maps;
    CONST DWORD *function = This->baseShader.function;
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *)This->baseShader.device;
    WineD3D_GL_Info *gl_info = &device->adapter->gl_info;
    local_constant* lconst;

    /*  Create the hw ARB shader */
    shader_addline(buffer, "!!ARBvp1.0\n");
    shader_addline(buffer, "PARAM helper_const = { 2.0, -1.0, %d.0, 0.0 };\n", This->rel_offset);

    /* Mesa supports only 95 constants */
    if (GL_VEND(MESA) || GL_VEND(WINE))
        This->baseShader.limits.constant_float =
                min(95, This->baseShader.limits.constant_float);

    shader_addline(buffer, "TEMP TMP;\n");

    /* Base Declarations */
    shader_generate_arb_declarations( (IWineD3DBaseShader*) This, reg_maps, buffer, &GLINFO_LOCATION);

    /* We need a constant to fixup the final position */
    shader_addline(buffer, "PARAM posFixup = program.env[%d];\n", ARB_SHADER_PRIVCONST_POS);

    /* Initialize output parameters. GL_ARB_vertex_program does not require special initialization values
     * for output parameters. D3D in theory does not do that either, but some applications depend on a
     * proper initialization of the secondary color, and programs using the fixed function pipeline without
     * a replacement shader depend on the texcoord.w being set properly.
     *
     * GL_NV_vertex_program defines that all output values are initialized to {0.0, 0.0, 0.0, 1.0}. This
     * assertion is in effect even when using GL_ARB_vertex_program without any NV specific additions. So
     * skip this if NV_vertex_program is supported. Otherwise, initialize the secondary color. For the tex-
     * coords, we have a flag in the opengl caps. Many cards do not require the texcoord being set, and
     * this can eat a number of instructions, so skip it unless this cap is set as well
     */
    if(!GL_SUPPORT(NV_VERTEX_PROGRAM)) {
        shader_addline(buffer, "MOV result.color.secondary, -helper_const.wwwy;\n");

        if((GLINFO_LOCATION).set_texcoord_w && !device->frag_pipe->ffp_proj_control) {
            int i;
            for(i = 0; i < min(8, MAX_REG_TEXCRD); i++) {
                if(This->baseShader.reg_maps.texcoord_mask[i] != 0 &&
                This->baseShader.reg_maps.texcoord_mask[i] != WINED3DSP_WRITEMASK_ALL) {
                    shader_addline(buffer, "MOV result.texcoord[%u].w, -helper_const.y;\n", i);
                }
            }
        }
    }

    /* Base Shader Body */
    shader_generate_main( (IWineD3DBaseShader*) This, buffer, reg_maps, function);

    /* If this shader doesn't use fog copy the z coord to the fog coord so that we can use table fog */
    if (!reg_maps->fog)
        shader_addline(buffer, "MOV result.fogcoord, TMP_OUT.z;\n");

    /* Write the final position.
     *
     * OpenGL coordinates specify the center of the pixel while d3d coords specify
     * the corner. The offsets are stored in z and w in posFixup. posFixup.y contains
     * 1.0 or -1.0 to turn the rendering upside down for offscreen rendering. PosFixup.x
     * contains 1.0 to allow a mad, but arb vs swizzles are too restricted for that.
     */
    shader_addline(buffer, "MUL TMP, posFixup, TMP_OUT.w;\n");
    shader_addline(buffer, "ADD TMP_OUT.x, TMP_OUT.x, TMP.z;\n");
    shader_addline(buffer, "MAD TMP_OUT.y, TMP_OUT.y, posFixup.y, TMP.w;\n");

    /* Z coord [0;1]->[-1;1] mapping, see comment in transform_projection in state.c
     * and the glsl equivalent
     */
    shader_addline(buffer, "MAD TMP_OUT.z, TMP_OUT.z, helper_const.x, -TMP_OUT.w;\n");

    shader_addline(buffer, "MOV result.position, TMP_OUT;\n");

    shader_addline(buffer, "END\n");

    /* TODO: change to resource.glObjectHandle or something like that */
    GL_EXTCALL(glGenProgramsARB(1, &This->baseShader.prgId));

    TRACE("Creating a hw vertex shader, prg=%d\n", This->baseShader.prgId);
    GL_EXTCALL(glBindProgramARB(GL_VERTEX_PROGRAM_ARB, This->baseShader.prgId));

    TRACE("Created hw vertex shader, prg=%d\n", This->baseShader.prgId);
    /* Create the program and check for errors */
    GL_EXTCALL(glProgramStringARB(GL_VERTEX_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
               buffer->bsize, buffer->buffer));

    if (glGetError() == GL_INVALID_OPERATION) {
        GLint errPos;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errPos);
        FIXME("HW VertexShader Error at position %d: %s\n",
              errPos, debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
        This->baseShader.prgId = -1;
    }

    /* Load immediate constants */
    if(!This->baseShader.load_local_constsF) {
        LIST_FOR_EACH_ENTRY(lconst, &This->baseShader.constantsF, local_constant, entry) {
            float *value = (float *) lconst->value;
            GL_EXTCALL(glProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, lconst->idx, value));
        }
    }
}

static void shader_arb_get_caps(WINED3DDEVTYPE devtype, WineD3D_GL_Info *gl_info, struct shader_caps *pCaps) {
    /* We don't have an ARB fixed function pipeline yet, so let the none backend set its caps,
     * then overwrite the shader specific ones
     */
    none_shader_backend.shader_get_caps(devtype, gl_info, pCaps);

    if(GL_SUPPORT(ARB_VERTEX_PROGRAM)) {
        pCaps->VertexShaderVersion = WINED3DVS_VERSION(1,1);
        TRACE_(d3d_caps)("Hardware vertex shader version 1.1 enabled (ARB_PROGRAM)\n");
        pCaps->MaxVertexShaderConst = GL_LIMITS(vshader_constantsF);
    }

    if(GL_SUPPORT(ARB_FRAGMENT_PROGRAM)) {
        pCaps->PixelShaderVersion    = WINED3DPS_VERSION(1,4);
        pCaps->PixelShader1xMaxValue = 8.0;
        TRACE_(d3d_caps)("Hardware pixel shader version 1.4 enabled (ARB_PROGRAM)\n");
    }
}

static BOOL shader_arb_conv_supported(WINED3DFORMAT fmt) {
    TRACE("Checking shader format support for format %s:", debug_d3dformat(fmt));
    switch(fmt) {
        case WINED3DFMT_V8U8:
        case WINED3DFMT_V16U16:
        case WINED3DFMT_X8L8V8U8:
        case WINED3DFMT_L6V5U5:
        case WINED3DFMT_Q8W8V8U8:
        case WINED3DFMT_ATI2N:
            TRACE("[OK]\n");
            return TRUE;
        default:
            TRACE("[FAILED\n");
            return FALSE;
    }
}

const shader_backend_t arb_program_shader_backend = {
    shader_arb_select,
    shader_arb_select_depth_blt,
    shader_arb_deselect_depth_blt,
    shader_arb_load_constants,
    shader_arb_cleanup,
    shader_arb_color_correction,
    shader_arb_destroy,
    shader_arb_alloc,
    shader_arb_free,
    shader_arb_dirty_const,
    shader_arb_generate_pshader,
    shader_arb_generate_vshader,
    shader_arb_get_caps,
    shader_arb_conv_supported,
};

/* ARB_fragment_program fixed function pipeline replacement definitions */
#define ARB_FFP_CONST_TFACTOR           0
#define ARB_FFP_CONST_SPECULAR_ENABLE   ((ARB_FFP_CONST_TFACTOR) + 1)
#define ARB_FFP_CONST_CONSTANT(i)       ((ARB_FFP_CONST_SPECULAR_ENABLE) + 1 + i)
#define ARB_FFP_CONST_BUMPMAT(i)        ((ARB_FFP_CONST_CONSTANT(7)) + 1 + i)
#define ARB_FFP_CONST_LUMINANCE(i)      ((ARB_FFP_CONST_BUMPMAT(7)) + 1 + i)

struct arbfp_ffp_desc
{
    struct ffp_desc parent;
    GLuint shader;
    unsigned int num_textures_used;
};

static void arbfp_enable(IWineD3DDevice *iface, BOOL enable) {
    if(enable) {
        glEnable(GL_FRAGMENT_PROGRAM_ARB);
        checkGLcall("glEnable(GL_FRAGMENT_PROGRAM_ARB)");
    } else {
        glDisable(GL_FRAGMENT_PROGRAM_ARB);
        checkGLcall("glDisable(GL_FRAGMENT_PROGRAM_ARB)");
    }
}

static HRESULT arbfp_alloc(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    struct shader_arb_priv *priv;
    /* Share private data between the shader backend and the pipeline replacement, if both
     * are the arb implementation. This is needed to figure out whether ARBfp should be disabled
     * if no pixel shader is bound or not
     */
    if(This->shader_backend == &arb_program_shader_backend) {
        This->fragment_priv = This->shader_priv;
    } else {
        This->fragment_priv = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct shader_arb_priv));
        if(!This->fragment_priv) return E_OUTOFMEMORY;
    }
    priv = (struct shader_arb_priv *) This->fragment_priv;
    priv->fragment_shaders = hash_table_create(ffp_program_key_hash, ffp_program_key_compare);
    priv->use_arbfp_fixed_func = TRUE;
    return WINED3D_OK;
}

static void arbfp_free_ffpshader(void *value, void *gli) {
    WineD3D_GL_Info *gl_info = gli;
    struct arbfp_ffp_desc *entry_arb = value;

    ENTER_GL();
    GL_EXTCALL(glDeleteProgramsARB(1, &entry_arb->shader));
    checkGLcall("glDeleteProgramsARB(1, &entry_arb->shader)");
    HeapFree(GetProcessHeap(), 0, entry_arb);
    LEAVE_GL();
}

static void arbfp_free(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *) iface;
    struct shader_arb_priv *priv = (struct shader_arb_priv *) This->fragment_priv;

    hash_table_destroy(priv->fragment_shaders, arbfp_free_ffpshader, &This->adapter->gl_info);
    priv->use_arbfp_fixed_func = FALSE;

    if(This->shader_backend != &arb_program_shader_backend) {
        HeapFree(GetProcessHeap(), 0, This->fragment_priv);
    }
}

static void arbfp_get_caps(WINED3DDEVTYPE devtype, WineD3D_GL_Info *gl_info, struct fragment_caps *caps) {
    caps->TextureOpCaps =  WINED3DTEXOPCAPS_DISABLE                     |
                           WINED3DTEXOPCAPS_SELECTARG1                  |
                           WINED3DTEXOPCAPS_SELECTARG2                  |
                           WINED3DTEXOPCAPS_MODULATE4X                  |
                           WINED3DTEXOPCAPS_MODULATE2X                  |
                           WINED3DTEXOPCAPS_MODULATE                    |
                           WINED3DTEXOPCAPS_ADDSIGNED2X                 |
                           WINED3DTEXOPCAPS_ADDSIGNED                   |
                           WINED3DTEXOPCAPS_ADD                         |
                           WINED3DTEXOPCAPS_SUBTRACT                    |
                           WINED3DTEXOPCAPS_ADDSMOOTH                   |
                           WINED3DTEXOPCAPS_BLENDCURRENTALPHA           |
                           WINED3DTEXOPCAPS_BLENDFACTORALPHA            |
                           WINED3DTEXOPCAPS_BLENDTEXTUREALPHA           |
                           WINED3DTEXOPCAPS_BLENDDIFFUSEALPHA           |
                           WINED3DTEXOPCAPS_BLENDTEXTUREALPHAPM         |
                           WINED3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR      |
                           WINED3DTEXOPCAPS_MODULATECOLOR_ADDALPHA      |
                           WINED3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA   |
                           WINED3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR   |
                           WINED3DTEXOPCAPS_DOTPRODUCT3                 |
                           WINED3DTEXOPCAPS_MULTIPLYADD                 |
                           WINED3DTEXOPCAPS_LERP                        |
                           WINED3DTEXOPCAPS_BUMPENVMAP                  |
                           WINED3DTEXOPCAPS_BUMPENVMAPLUMINANCE;

    /* TODO: Implement WINED3DTEXOPCAPS_PREMODULATE */

    caps->MaxTextureBlendStages   = 8;
    caps->MaxSimultaneousTextures = min(GL_LIMITS(fragment_samplers), 8);

    caps->PrimitiveMiscCaps |= WINED3DPMISCCAPS_TSSARGTEMP;
}
#undef GLINFO_LOCATION

#define GLINFO_LOCATION stateblock->wineD3DDevice->adapter->gl_info
static void state_texfactor_arbfp(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    float col[4];
    IWineD3DDeviceImpl *device = stateblock->wineD3DDevice;

    /* Do not overwrite pixel shader constants if a pshader is in use */
    if(use_ps(device)) return;

    D3DCOLORTOGLFLOAT4(stateblock->renderState[WINED3DRS_TEXTUREFACTOR], col);
    GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_TFACTOR, col));
    checkGLcall("glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_TFACTOR, col)");

    if(device->shader_backend == &arb_program_shader_backend) {
        device = stateblock->wineD3DDevice;
        device->activeContext->pshader_const_dirty[ARB_FFP_CONST_TFACTOR] = 1;
        device->highest_dirty_ps_const = max(device->highest_dirty_ps_const, ARB_FFP_CONST_TFACTOR + 1);
    }
}

static void state_arb_specularenable(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    float col[4];
    IWineD3DDeviceImpl *device = stateblock->wineD3DDevice;

    /* Do not overwrite pixel shader constants if a pshader is in use */
    if(use_ps(device)) return;

    if(stateblock->renderState[WINED3DRS_SPECULARENABLE]) {
        /* The specular color has no alpha */
        col[0] = 1.0; col[1] = 1.0;
        col[2] = 1.0; col[3] = 0.0;
    } else {
        col[0] = 0.0; col[1] = 0.0;
        col[2] = 0.0; col[3] = 0.0;
    }
    GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_SPECULAR_ENABLE, col));
    checkGLcall("glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_SPECULAR_ENABLE, col)");

    if(device->shader_backend == &arb_program_shader_backend) {
        device = stateblock->wineD3DDevice;
        device->activeContext->pshader_const_dirty[ARB_FFP_CONST_SPECULAR_ENABLE] = 1;
        device->highest_dirty_ps_const = max(device->highest_dirty_ps_const, ARB_FFP_CONST_SPECULAR_ENABLE + 1);
    }
}

static void set_bumpmat_arbfp(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    DWORD stage = (state - STATE_TEXTURESTAGE(0, 0)) / WINED3D_HIGHEST_TEXTURE_STATE;
    IWineD3DDeviceImpl *device = stateblock->wineD3DDevice;
    float mat[2][2];

    if(use_ps(device)) {
        if(stage != 0 &&
           ((IWineD3DPixelShaderImpl *) stateblock->pixelShader)->baseShader.reg_maps.bumpmat[stage]) {
            /* The pixel shader has to know the bump env matrix. Do a constants update if it isn't scheduled
             * anyway
             */
            if(!isStateDirty(context, STATE_PIXELSHADERCONSTANT)) {
                device->StateTable[STATE_PIXELSHADERCONSTANT].apply(STATE_PIXELSHADERCONSTANT, stateblock, context);
            }
        }
        /* Exit now, don't set the bumpmat below, otherwise we may overwrite pixel shader constants */
        return;
    }

    mat[0][0] = *((float *) &stateblock->textureState[stage][WINED3DTSS_BUMPENVMAT00]);
    mat[0][1] = *((float *) &stateblock->textureState[stage][WINED3DTSS_BUMPENVMAT01]);
    mat[1][0] = *((float *) &stateblock->textureState[stage][WINED3DTSS_BUMPENVMAT10]);
    mat[1][1] = *((float *) &stateblock->textureState[stage][WINED3DTSS_BUMPENVMAT11]);

    GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_BUMPMAT(stage), &mat[0][0]));
    checkGLcall("glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_BUMPMAT(stage), &mat[0][0])");

    if(device->shader_backend == &arb_program_shader_backend) {
        device->activeContext->pshader_const_dirty[ARB_FFP_CONST_BUMPMAT(stage)] = 1;
        device->highest_dirty_ps_const = max(device->highest_dirty_ps_const, ARB_FFP_CONST_BUMPMAT(stage) + 1);
    }
}

static void tex_bumpenvlum_arbfp(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    DWORD stage = (state - STATE_TEXTURESTAGE(0, 0)) / WINED3D_HIGHEST_TEXTURE_STATE;
    IWineD3DDeviceImpl *device = stateblock->wineD3DDevice;
    float param[4];

    if(use_ps(device)) {
        if(stage != 0 &&
           ((IWineD3DPixelShaderImpl *) stateblock->pixelShader)->baseShader.reg_maps.luminanceparams[stage]) {
            /* The pixel shader has to know the luminance offset. Do a constants update if it
             * isn't scheduled anyway
             */
            if(!isStateDirty(context, STATE_PIXELSHADERCONSTANT)) {
                device->StateTable[STATE_PIXELSHADERCONSTANT].apply(STATE_PIXELSHADERCONSTANT, stateblock, context);
            }
        }
        /* Exit now, don't set the bumpmat below, otherwise we may overwrite pixel shader constants */
        return;
    }

    param[0] = *((float *) &stateblock->textureState[stage][WINED3DTSS_BUMPENVLSCALE]);
    param[1] = *((float *) &stateblock->textureState[stage][WINED3DTSS_BUMPENVLOFFSET]);
    param[2] = 0.0;
    param[3] = 0.0;

    GL_EXTCALL(glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_LUMINANCE(stage), param));
    checkGLcall("glProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, ARB_FFP_CONST_LUMINANCE(stage), param)");

    if(device->shader_backend == &arb_program_shader_backend) {
        device->activeContext->pshader_const_dirty[ARB_FFP_CONST_LUMINANCE(stage)] = 1;
        device->highest_dirty_ps_const = max(device->highest_dirty_ps_const, ARB_FFP_CONST_LUMINANCE(stage) + 1);
    }
}

static const char *get_argreg(SHADER_BUFFER *buffer, DWORD argnum, unsigned int stage, DWORD arg) {
    const char *ret;

    if(arg == ARG_UNUSED) return "unused"; /* This is the marker for unused registers */

    switch(arg & WINED3DTA_SELECTMASK) {
        case WINED3DTA_DIFFUSE:
            ret = "fragment.color.primary"; break;

        case WINED3DTA_CURRENT:
            if(stage == 0) ret = "fragment.color.primary";
            else ret = "ret";
            break;

        case WINED3DTA_TEXTURE:
            switch(stage) {
                case 0: ret = "tex0"; break;
                case 1: ret = "tex1"; break;
                case 2: ret = "tex2"; break;
                case 3: ret = "tex3"; break;
                case 4: ret = "tex4"; break;
                case 5: ret = "tex5"; break;
                case 6: ret = "tex6"; break;
                case 7: ret = "tex7"; break;
                default: ret = "unknown texture";
            }
            break;

        case WINED3DTA_TFACTOR:
            ret = "tfactor"; break;

        case WINED3DTA_SPECULAR:
            ret = "fragment.color.secondary"; break;

        case WINED3DTA_TEMP:
            ret = "tempreg"; break;

        case WINED3DTA_CONSTANT:
            FIXME("Implement perstage constants\n");
            switch(stage) {
                case 0: ret = "const0"; break;
                case 1: ret = "const1"; break;
                case 2: ret = "const2"; break;
                case 3: ret = "const3"; break;
                case 4: ret = "const4"; break;
                case 5: ret = "const5"; break;
                case 6: ret = "const6"; break;
                case 7: ret = "const7"; break;
            }
        default:
            return "unknown";
    }

    if(arg & WINED3DTA_COMPLEMENT) {
        shader_addline(buffer, "SUB arg%u, const.x, %s;\n", argnum, ret);
        if(argnum == 0) ret = "arg0";
        if(argnum == 1) ret = "arg1";
        if(argnum == 2) ret = "arg2";
    }
    if(arg & WINED3DTA_ALPHAREPLICATE) {
        shader_addline(buffer, "MOV arg%u, %s.a;\n", argnum, ret);
        if(argnum == 0) ret = "arg0";
        if(argnum == 1) ret = "arg1";
        if(argnum == 2) ret = "arg2";
    }
    return ret;
}

static void gen_ffp_instr(SHADER_BUFFER *buffer, unsigned int stage, BOOL color, BOOL alpha,
                          DWORD dst, DWORD op, DWORD dw_arg0, DWORD dw_arg1, DWORD dw_arg2) {
    const char *dstmask, *dstreg, *arg0, *arg1, *arg2;
    unsigned int mul = 1;
    BOOL mul_final_dest = FALSE;

    if(color && alpha) dstmask = "";
    else if(color) dstmask = ".rgb";
    else dstmask = ".a";

    if(dst == tempreg) dstreg = "tempreg";
    else dstreg = "ret";

    arg0 = get_argreg(buffer, 0, stage, dw_arg0);
    arg1 = get_argreg(buffer, 1, stage, dw_arg1);
    arg2 = get_argreg(buffer, 2, stage, dw_arg2);

    switch(op) {
        case WINED3DTOP_DISABLE:
            if(stage == 1) shader_addline(buffer, "MOV %s%s, fragment.color.primary;\n", dstreg, dstmask);
            break;

        case WINED3DTOP_SELECTARG2:
            arg1 = arg2;
        case WINED3DTOP_SELECTARG1:
            shader_addline(buffer, "MOV %s%s, %s;\n", dstreg, dstmask, arg1);
            break;

        case WINED3DTOP_MODULATE4X:
            mul = 2;
        case WINED3DTOP_MODULATE2X:
            mul *= 2;
            if(strcmp(dstreg, "result.color") == 0) {
                dstreg = "ret";
                mul_final_dest = TRUE;
            }
        case WINED3DTOP_MODULATE:
            shader_addline(buffer, "MUL %s%s, %s, %s;\n", dstreg, dstmask, arg1, arg2);
            break;

        case WINED3DTOP_ADDSIGNED2X:
            mul = 2;
            if(strcmp(dstreg, "result.color") == 0) {
                dstreg = "ret";
                mul_final_dest = TRUE;
            }
        case WINED3DTOP_ADDSIGNED:
            shader_addline(buffer, "SUB arg2, %s, const.w;\n", arg2);
            arg2 = "arg2";
        case WINED3DTOP_ADD:
            shader_addline(buffer, "ADD_SAT %s%s, %s, %s;\n", dstreg, dstmask, arg1, arg2);
            break;

        case WINED3DTOP_SUBTRACT:
            shader_addline(buffer, "SUB_SAT %s%s, %s, %s;\n", dstreg, dstmask, arg1, arg2);
            break;

        case WINED3DTOP_ADDSMOOTH:
            shader_addline(buffer, "SUB arg1, const.x, %s;\n", arg1);
            shader_addline(buffer, "MAD_SAT %s%s, arg1, %s, %s;\n", dstreg, dstmask, arg2, arg1);
            break;

        case WINED3DTOP_BLENDCURRENTALPHA:
            arg0 = get_argreg(buffer, 0, stage, WINED3DTA_CURRENT);
            shader_addline(buffer, "LRP %s%s, %s.a, %s, %s;\n", dstreg, dstmask, arg0, arg1, arg2);
            break;
        case WINED3DTOP_BLENDFACTORALPHA:
            arg0 = get_argreg(buffer, 0, stage, WINED3DTA_TFACTOR);
            shader_addline(buffer, "LRP %s%s, %s.a, %s, %s;\n", dstreg, dstmask, arg0, arg1, arg2);
            break;
        case WINED3DTOP_BLENDTEXTUREALPHA:
            arg0 = get_argreg(buffer, 0, stage, WINED3DTA_TEXTURE);
            shader_addline(buffer, "LRP %s%s, %s.a, %s, %s;\n", dstreg, dstmask, arg0, arg1, arg2);
            break;
        case WINED3DTOP_BLENDDIFFUSEALPHA:
            arg0 = get_argreg(buffer, 0, stage, WINED3DTA_DIFFUSE);
            shader_addline(buffer, "LRP %s%s, %s.a, %s, %s;\n", dstreg, dstmask, arg0, arg1, arg2);
            break;

        case WINED3DTOP_BLENDTEXTUREALPHAPM:
            arg0 = get_argreg(buffer, 0, stage, WINED3DTA_TEXTURE);
            shader_addline(buffer, "SUB arg0.a, const.x, %s.a;\n", arg0);
            shader_addline(buffer, "MAD_SAT %s%s, %s, arg0.a, %s;\n", dstreg, dstmask, arg2, arg1);
            break;

        /* D3DTOP_PREMODULATE ???? */

        case WINED3DTOP_MODULATEINVALPHA_ADDCOLOR:
            shader_addline(buffer, "SUB arg0.a, const.x, %s;\n", arg1);
            shader_addline(buffer, "MAD_SAT %s%s, arg0.a, %s, %s;\n", dstreg, dstmask, arg2, arg1);
            break;
        case WINED3DTOP_MODULATEALPHA_ADDCOLOR:
            shader_addline(buffer, "MAD_SAT %s%s, %s.a, %s, %s;\n", dstreg, dstmask, arg1, arg2, arg1);
            break;
        case WINED3DTOP_MODULATEINVCOLOR_ADDALPHA:
            shader_addline(buffer, "SUB arg0, const.x, %s;\n", arg1);
            shader_addline(buffer, "MAD_SAT %s%s, arg0, %s, %s.a;\n", dstreg, dstmask, arg2, arg1);
            break;
        case WINED3DTOP_MODULATECOLOR_ADDALPHA:
            shader_addline(buffer, "MAD_SAT %s%s, %s, %s, %s.a;\n", dstreg, dstmask, arg1, arg2, arg1);
            break;

        case WINED3DTOP_DOTPRODUCT3:
            mul = 4;
            if(strcmp(dstreg, "result.color") == 0) {
                dstreg = "ret";
                mul_final_dest = TRUE;
            }
            shader_addline(buffer, "SUB arg1, %s, const.w;\n", arg1);
            shader_addline(buffer, "SUB arg2, %s, const.w;\n", arg2);
            shader_addline(buffer, "DP3_SAT %s%s, arg1, arg2;\n", dstreg, dstmask);
            break;

        case WINED3DTOP_MULTIPLYADD:
            shader_addline(buffer, "MAD_SAT %s%s, %s, %s, %s;\n", dstreg, dstmask, arg1, arg2, arg0);
            break;

        case WINED3DTOP_LERP:
            /* The msdn is not quite right here */
            shader_addline(buffer, "LRP %s%s, %s, %s, %s;\n", dstreg, dstmask, arg0, arg1, arg2);
            break;

        case WINED3DTOP_BUMPENVMAP:
        case WINED3DTOP_BUMPENVMAPLUMINANCE:
            /* Those are handled in the first pass of the shader(generation pass 1 and 2) already */
            break;

        default:
            FIXME("Unhandled texture op %08x\n", op);
    }

    if(mul == 2) {
        shader_addline(buffer, "MUL_SAT %s%s, %s, const.y;\n", mul_final_dest ? "result.color" : dstreg, dstmask, dstreg);
    } else if(mul == 4) {
        shader_addline(buffer, "MUL_SAT %s%s, %s, const.z;\n", mul_final_dest ? "result.color" : dstreg, dstmask, dstreg);
    }
}

/* The stateblock is passed for GLINFO_LOCATION */
static GLuint gen_arbfp_ffp_shader(struct ffp_settings *settings, IWineD3DStateBlockImpl *stateblock) {
    unsigned int stage;
    SHADER_BUFFER buffer;
    BOOL tex_read[MAX_TEXTURES] = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
    BOOL bump_used[MAX_TEXTURES] = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
    BOOL luminance_used[MAX_TEXTURES] = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
    const char *textype;
    const char *instr, *sat;
    char colorcor_dst[8];
    GLuint ret;
    DWORD arg0, arg1, arg2;
    BOOL tempreg_used = FALSE, tfactor_used = FALSE;
    BOOL op_equal;
    const char *final_combiner_src = "ret";

    /* Find out which textures are read */
    for(stage = 0; stage < MAX_TEXTURES; stage++) {
        if(settings->op[stage].cop == WINED3DTOP_DISABLE) break;
        arg0 = settings->op[stage].carg0 & WINED3DTA_SELECTMASK;
        arg1 = settings->op[stage].carg1 & WINED3DTA_SELECTMASK;
        arg2 = settings->op[stage].carg2 & WINED3DTA_SELECTMASK;
        if(arg0 == WINED3DTA_TEXTURE) tex_read[stage] = TRUE;
        if(arg1 == WINED3DTA_TEXTURE) tex_read[stage] = TRUE;
        if(arg2 == WINED3DTA_TEXTURE) tex_read[stage] = TRUE;

        if(settings->op[stage].cop == WINED3DTOP_BLENDTEXTUREALPHA) tex_read[stage] = TRUE;
        if(settings->op[stage].cop == WINED3DTOP_BLENDTEXTUREALPHAPM) tex_read[stage] = TRUE;
        if(settings->op[stage].cop == WINED3DTOP_BUMPENVMAP) {
            bump_used[stage] = TRUE;
            tex_read[stage] = TRUE;
        }
        if(settings->op[stage].cop == WINED3DTOP_BUMPENVMAPLUMINANCE) {
            bump_used[stage] = TRUE;
            tex_read[stage] = TRUE;
            luminance_used[stage] = TRUE;
        } else if(settings->op[stage].cop == WINED3DTOP_BLENDFACTORALPHA) {
            tfactor_used = TRUE;
        }

        if(arg0 == WINED3DTA_TFACTOR || arg1 == WINED3DTA_TFACTOR || arg2 == WINED3DTA_TFACTOR) {
            tfactor_used = TRUE;
        }

        if(settings->op[stage].dst == tempreg) tempreg_used = TRUE;
        if(arg0 == WINED3DTA_TEMP || arg1 == WINED3DTA_TEMP || arg2 == WINED3DTA_TEMP) {
            tempreg_used = TRUE;
        }

        if(settings->op[stage].aop == WINED3DTOP_DISABLE) continue;
        arg0 = settings->op[stage].aarg0 & WINED3DTA_SELECTMASK;
        arg1 = settings->op[stage].aarg1 & WINED3DTA_SELECTMASK;
        arg2 = settings->op[stage].aarg2 & WINED3DTA_SELECTMASK;
        if(arg0 == WINED3DTA_TEXTURE) tex_read[stage] = TRUE;
        if(arg1 == WINED3DTA_TEXTURE) tex_read[stage] = TRUE;
        if(arg2 == WINED3DTA_TEXTURE) tex_read[stage] = TRUE;

        if(arg0 == WINED3DTA_TEMP || arg1 == WINED3DTA_TEMP || arg2 == WINED3DTA_TEMP) {
            tempreg_used = TRUE;
        }
        if(arg0 == WINED3DTA_TFACTOR || arg1 == WINED3DTA_TFACTOR || arg2 == WINED3DTA_TFACTOR) {
            tfactor_used = TRUE;
        }
    }

    /* Shader header */
    buffer.bsize = 0;
    buffer.lineNo = 0;
    buffer.newline = TRUE;
    buffer.buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, SHADER_PGMSIZE);

    shader_addline(&buffer, "!!ARBfp1.0\n");

    switch(settings->fog) {
        case FOG_OFF:                                                         break;
        case FOG_LINEAR: shader_addline(&buffer, "OPTION ARB_fog_linear;\n"); break;
        case FOG_EXP:    shader_addline(&buffer, "OPTION ARB_fog_exp;\n");    break;
        case FOG_EXP2:   shader_addline(&buffer, "OPTION ARB_fog_exp2;\n");   break;
        default: FIXME("Unexpected fog setting %d\n", settings->fog);
    }

    shader_addline(&buffer, "PARAM const = {1, 2, 4, 0.5};\n");
    shader_addline(&buffer, "TEMP ret;\n");
    if(tempreg_used || settings->sRGB_write) shader_addline(&buffer, "TEMP tempreg;\n");
    shader_addline(&buffer, "TEMP arg0;\n");
    shader_addline(&buffer, "TEMP arg1;\n");
    shader_addline(&buffer, "TEMP arg2;\n");
    for(stage = 0; stage < MAX_TEXTURES; stage++) {
        if(!tex_read[stage]) continue;
        shader_addline(&buffer, "TEMP tex%u;\n", stage);
        if(!bump_used[stage]) continue;
        shader_addline(&buffer, "PARAM bumpmat%u = program.env[%u];\n", stage, ARB_FFP_CONST_BUMPMAT(stage));
        if(!luminance_used[stage]) continue;
        shader_addline(&buffer, "PARAM luminance%u = program.env[%u];\n", stage, ARB_FFP_CONST_LUMINANCE(stage));
    }
    if(tfactor_used) {
        shader_addline(&buffer, "PARAM tfactor = program.env[%u];\n", ARB_FFP_CONST_TFACTOR);
    }
        shader_addline(&buffer, "PARAM specular_enable = program.env[%u];\n", ARB_FFP_CONST_SPECULAR_ENABLE);

    if(settings->sRGB_write) {
        shader_addline(&buffer, "PARAM srgb_mul_low = {%f, %f, %f, 1.0};\n",
                       srgb_mul_low, srgb_mul_low, srgb_mul_low);
        shader_addline(&buffer, "PARAM srgb_comparison =  {%f, %f, %f, %f};\n",
                       srgb_cmp, srgb_cmp, srgb_cmp, srgb_cmp);
        shader_addline(&buffer, "PARAM srgb_pow =  {%f, %f, %f, 1.0};\n",
                       srgb_pow, srgb_pow, srgb_pow);
        shader_addline(&buffer, "PARAM srgb_mul_hi =  {%f, %f, %f, 1.0};\n",
                       srgb_mul_high, srgb_mul_high, srgb_mul_high);
        shader_addline(&buffer, "PARAM srgb_sub_hi =  {%f, %f, %f, 0.0};\n",
                       srgb_sub_high, srgb_sub_high, srgb_sub_high);
    }

    /* Generate texture sampling instructions) */
    for(stage = 0; stage < MAX_TEXTURES && settings->op[stage].cop != WINED3DTOP_DISABLE; stage++) {
        if(!tex_read[stage]) continue;

        switch(settings->op[stage].tex_type) {
            case tex_1d:                    textype = "1D";     break;
            case tex_2d:                    textype = "2D";     break;
            case tex_3d:                    textype = "3D";     break;
            case tex_cube:                  textype = "CUBE";   break;
            case tex_rect:                  textype = "RECT";   break;
            default: textype = "unexpected_textype";   break;
        }

        if(settings->op[stage].cop == WINED3DTOP_BUMPENVMAP ||
           settings->op[stage].cop == WINED3DTOP_BUMPENVMAPLUMINANCE) {
            sat = "";
        } else {
            sat = "_SAT";
        }

        if(settings->op[stage].projected == proj_none) {
            instr = "TEX";
        } else if(settings->op[stage].projected == proj_count4 ||
                  settings->op[stage].projected == proj_count3) {
            instr = "TXP";
        } else {
            FIXME("Unexpected projection mode %d\n", settings->op[stage].projected);
            instr = "TXP";
        }

        if(stage > 0 &&
           (settings->op[stage - 1].cop == WINED3DTOP_BUMPENVMAP ||
            settings->op[stage - 1].cop == WINED3DTOP_BUMPENVMAPLUMINANCE)) {
            shader_addline(&buffer, "SWZ arg1, bumpmat%u, x, z, 0, 0;\n", stage - 1);
            shader_addline(&buffer, "DP3 ret.r, arg1, tex%u;\n", stage - 1);
            shader_addline(&buffer, "SWZ arg1, bumpmat%u, y, w, 0, 0;\n", stage - 1);
            shader_addline(&buffer, "DP3 ret.g, arg1, tex%u;\n", stage - 1);

            /* with projective textures, texbem only divides the static texture coord, not the displacement,
             * so multiply the displacement with the dividing parameter before passing it to TXP
             */
            if (settings->op[stage].projected != proj_none) {
                if(settings->op[stage].projected == proj_count4) {
                    shader_addline(&buffer, "MOV ret.a, fragment.texcoord[%u].a;\n", stage);
                    shader_addline(&buffer, "MUL ret.rgb, ret, fragment.texcoord[%u].a, fragment.texcoord[%u];\n", stage, stage);
                } else {
                    shader_addline(&buffer, "MOV ret.a, fragment.texcoord[%u].b;\n", stage);
                    shader_addline(&buffer, "MAD ret.rgb, ret, fragment.texcoord[%u].b, fragment.texcoord[%u];\n", stage, stage);
                }
            } else {
                shader_addline(&buffer, "ADD ret, ret, fragment.texcoord[%u];\n", stage);
            }

            shader_addline(&buffer, "%s%s tex%u, ret, texture[%u], %s;\n",
                           instr, sat, stage, stage, textype);
            if(settings->op[stage - 1].cop == WINED3DTOP_BUMPENVMAPLUMINANCE) {
                shader_addline(&buffer, "MAD_SAT ret.r, tex%u.b, luminance%u.r, luminance%u.g;\n",
                               stage - 1, stage - 1, stage - 1);
                shader_addline(&buffer, "MUL tex%u, tex%u, ret.r;\n", stage, stage);
            }
        } else if(settings->op[stage].projected == proj_count3) {
            shader_addline(&buffer, "MOV ret, fragment.texcoord[%u];\n", stage);
            shader_addline(&buffer, "MOV ret.a, ret.b;\n");
            shader_addline(&buffer, "%s%s tex%u, ret, texture[%u], %s;\n",
                            instr, sat, stage, stage, textype);
        } else {
            shader_addline(&buffer, "%s%s tex%u, fragment.texcoord[%u], texture[%u], %s;\n",
                            instr, sat, stage, stage, stage, textype);
        }

        sprintf(colorcor_dst, "tex%u", stage);
        gen_color_correction(&buffer, colorcor_dst, ".rgba", "const.x", "const.y",
                                settings->op[stage].color_correction, &GLINFO_LOCATION);
    }

    /* Generate the main shader */
    for(stage = 0; stage < MAX_TEXTURES; stage++) {
        if(settings->op[stage].cop == WINED3DTOP_DISABLE) {
            if(stage == 0) {
                final_combiner_src = "fragment.color.primary";
            }
            break;
        }

        if(settings->op[stage].cop == WINED3DTOP_SELECTARG1 &&
           settings->op[stage].aop == WINED3DTOP_SELECTARG1) {
            op_equal = settings->op[stage].carg1 == settings->op[stage].aarg1;
        } else if(settings->op[stage].cop == WINED3DTOP_SELECTARG1 &&
                  settings->op[stage].aop == WINED3DTOP_SELECTARG2) {
            op_equal = settings->op[stage].carg1 == settings->op[stage].aarg2;
        } else if(settings->op[stage].cop == WINED3DTOP_SELECTARG2 &&
                  settings->op[stage].aop == WINED3DTOP_SELECTARG1) {
            op_equal = settings->op[stage].carg2 == settings->op[stage].aarg1;
        } else if(settings->op[stage].cop == WINED3DTOP_SELECTARG2 &&
                  settings->op[stage].aop == WINED3DTOP_SELECTARG2) {
            op_equal = settings->op[stage].carg2 == settings->op[stage].aarg2;
        } else {
            op_equal = settings->op[stage].aop   == settings->op[stage].cop &&
                       settings->op[stage].carg0 == settings->op[stage].aarg0 &&
                       settings->op[stage].carg1 == settings->op[stage].aarg1 &&
                       settings->op[stage].carg2 == settings->op[stage].aarg2;
        }

        if(settings->op[stage].aop == WINED3DTOP_DISABLE) {
            gen_ffp_instr(&buffer, stage, TRUE, FALSE, settings->op[stage].dst,
                          settings->op[stage].cop, settings->op[stage].carg0,
                          settings->op[stage].carg1, settings->op[stage].carg2);
            if(stage == 0) {
                shader_addline(&buffer, "MOV ret.a, fragment.color.primary.a;\n");
            }
        } else if(op_equal) {
            gen_ffp_instr(&buffer, stage, TRUE, TRUE, settings->op[stage].dst,
                          settings->op[stage].cop, settings->op[stage].carg0,
                          settings->op[stage].carg1, settings->op[stage].carg2);
        } else {
            gen_ffp_instr(&buffer, stage, TRUE, FALSE, settings->op[stage].dst,
                          settings->op[stage].cop, settings->op[stage].carg0,
                          settings->op[stage].carg1, settings->op[stage].carg2);
            gen_ffp_instr(&buffer, stage, FALSE, TRUE, settings->op[stage].dst,
                          settings->op[stage].aop, settings->op[stage].aarg0,
                          settings->op[stage].aarg1, settings->op[stage].aarg2);
        }
    }

    if(settings->sRGB_write) {
        shader_addline(&buffer, "MAD ret, fragment.color.secondary, specular_enable, %s;\n", final_combiner_src);
        arbfp_add_sRGB_correction(&buffer, "ret", "arg0", "arg1", "arg2", "tempreg");
        shader_addline(&buffer, "MOV result.color.a, ret.a;\n");
    } else {
        shader_addline(&buffer, "MAD result.color, fragment.color.secondary, specular_enable, %s;\n", final_combiner_src);
    }

    /* Footer */
    shader_addline(&buffer, "END\n");

    /* Generate the shader */
    GL_EXTCALL(glGenProgramsARB(1, &ret));
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, ret));
    GL_EXTCALL(glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(buffer.buffer), buffer.buffer));

    if (glGetError() == GL_INVALID_OPERATION) {
        GLint pos;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &pos);
        FIXME("Fragment program error at position %d: %s\n", pos,
              debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
    }
    HeapFree(GetProcessHeap(), 0, buffer.buffer);
    return ret;
}

static void fragment_prog_arbfp(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    IWineD3DDeviceImpl *device = stateblock->wineD3DDevice;
    struct shader_arb_priv *priv = (struct shader_arb_priv *) device->fragment_priv;
    BOOL use_pshader = use_ps(device);
    BOOL use_vshader = use_vs(device);
    struct ffp_settings settings;
    struct arbfp_ffp_desc *desc;
    unsigned int i;

    if(isStateDirty(context, STATE_RENDER(WINED3DRS_FOGENABLE))) {
        if(use_pshader) {
            IWineD3DPixelShader_CompileShader(stateblock->pixelShader);
        } else if(device->shader_backend == &arb_program_shader_backend && context->last_was_pshader) {
            /* Reload fixed function constants since they collide with the pixel shader constants */
            for(i = 0; i < MAX_TEXTURES; i++) {
                set_bumpmat_arbfp(STATE_TEXTURESTAGE(i, WINED3DTSS_BUMPENVMAT00), stateblock, context);
            }
            state_texfactor_arbfp(STATE_RENDER(WINED3DRS_TEXTUREFACTOR), stateblock, context);
            state_arb_specularenable(STATE_RENDER(WINED3DRS_SPECULARENABLE), stateblock, context);
        }
        return;
    }

    if(use_pshader) {
        IWineD3DPixelShader_CompileShader(stateblock->pixelShader);
    } else {
        /* Find or create a shader implementing the fixed function pipeline settings, then activate it */
        gen_ffp_op(stateblock, &settings, FALSE);
        desc = (struct arbfp_ffp_desc *) find_ffp_shader(priv->fragment_shaders, &settings);
        if(!desc) {
            desc = HeapAlloc(GetProcessHeap(), 0, sizeof(*desc));
            if(!desc) {
                ERR("Out of memory\n");
                return;
            }
            desc->num_textures_used = 0;
            for(i = 0; i < GL_LIMITS(texture_stages); i++) {
                if(settings.op[i].cop == WINED3DTOP_DISABLE) break;
                desc->num_textures_used = i;
            }

            memcpy(&desc->parent.settings, &settings, sizeof(settings));
            desc->shader = gen_arbfp_ffp_shader(&settings, stateblock);
            add_ffp_shader(priv->fragment_shaders, &desc->parent);
            TRACE("Allocated fixed function replacement shader descriptor %p\n", desc);
        }

        /* Now activate the replacement program. GL_FRAGMENT_PROGRAM_ARB is already active(however, note the
         * comment above the shader_select call below). If e.g. GLSL is active, the shader_select call will
         * deactivate it.
         */
        GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, desc->shader));
        checkGLcall("glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, desc->shader)");

        if(device->shader_backend == &arb_program_shader_backend && context->last_was_pshader) {
            /* Reload fixed function constants since they collide with the pixel shader constants */
            for(i = 0; i < MAX_TEXTURES; i++) {
                set_bumpmat_arbfp(STATE_TEXTURESTAGE(i, WINED3DTSS_BUMPENVMAT00), stateblock, context);
            }
            state_texfactor_arbfp(STATE_RENDER(WINED3DRS_TEXTUREFACTOR), stateblock, context);
            state_arb_specularenable(STATE_RENDER(WINED3DRS_SPECULARENABLE), stateblock, context);
        }
    }

    /* Finally, select the shader. If a pixel shader is used, it will be set and enabled by the shader backend.
     * If this shader backend is arbfp(most likely), then it will simply overwrite the last fixed function replace-
     * ment shader. If the shader backend is not ARB, it currently is important that the opengl implementation
     * type overwrites GL_ARB_fragment_program. This is currently the case with GLSL. If we really want to use
     * atifs or nvrc pixel shaders with arb fragment programs we'd have to disable GL_FRAGMENT_PROGRAM_ARB here
     *
     * Don't call shader_select if the vertex shader is dirty, because some shader backends(GLSL) need both shaders
     * to be compiled before activating them(needs some cleanups in the shader backend interface)
     */
    if(!isStateDirty(context, device->StateTable[STATE_VSHADER].representative)) {
        device->shader_backend->shader_select((IWineD3DDevice *)stateblock->wineD3DDevice, use_pshader, use_vshader);

        if (!isStateDirty(context, STATE_VERTEXSHADERCONSTANT) && (use_vshader || use_pshader)) {
            device->StateTable[STATE_VERTEXSHADERCONSTANT].apply(STATE_VERTEXSHADERCONSTANT, stateblock, context);
        }
    }
    if(use_pshader) {
        device->StateTable[STATE_PIXELSHADERCONSTANT].apply(STATE_PIXELSHADERCONSTANT, stateblock, context);
    }
}

/* We can't link the fog states to the fragment state directly since the vertex pipeline links them
 * to FOGENABLE. A different linking in different pipeline parts can't be expressed in the combined
 * state table, so we need to handle that with a forwarding function. The other invisible side effect
 * is that changing the fog start and fog end(which links to FOGENABLE in vertex) results in the
 * fragment_prog_arbfp function being called because FOGENABLE is dirty, which calls this function here
 */
static void state_arbfp_fog(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(!isStateDirty(context, STATE_PIXELSHADER)) {
        fragment_prog_arbfp(state, stateblock, context);
    }
}

static void textransform(DWORD state, IWineD3DStateBlockImpl *stateblock, WineD3DContext *context) {
    if(!isStateDirty(context, STATE_PIXELSHADER)) {
        fragment_prog_arbfp(state, stateblock, context);
    }
}

#undef GLINFO_LOCATION

static const struct StateEntryTemplate arbfp_fragmentstate_template[] = {
    {STATE_RENDER(WINED3DRS_TEXTUREFACTOR),               { STATE_RENDER(WINED3DRS_TEXTUREFACTOR),              state_texfactor_arbfp   }, 0                               },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_COLOROP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_COLORARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_COLORARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_COLORARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_ALPHAOP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_ALPHAARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_ALPHAARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_ALPHAARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_RESULTARG),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT00),      { STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT01),      { STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT10),      { STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT11),      { STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVLSCALE),     { STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, 0                               },
    {STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVLOFFSET),    { STATE_TEXTURESTAGE(0, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, 0                               },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_COLOROP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_COLORARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_COLORARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_COLORARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_ALPHAOP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_ALPHAARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_ALPHAARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_ALPHAARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_RESULTARG),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT00),      { STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT01),      { STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT10),      { STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT11),      { STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVLSCALE),     { STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, 0                               },
    {STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVLOFFSET),    { STATE_TEXTURESTAGE(1, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, 0                               },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_COLOROP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_COLORARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_COLORARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_COLORARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_ALPHAOP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_ALPHAARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_ALPHAARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_ALPHAARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_RESULTARG),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT00),      { STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT01),      { STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT10),      { STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT11),      { STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVLSCALE),     { STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, 0                               },
    {STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVLOFFSET),    { STATE_TEXTURESTAGE(2, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, 0                               },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_COLOROP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_COLORARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_COLORARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_COLORARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_ALPHAOP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_ALPHAARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_ALPHAARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_ALPHAARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_RESULTARG),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT00),      { STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT01),      { STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT10),      { STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT11),      { STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVLSCALE),     { STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, 0                               },
    {STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVLOFFSET),    { STATE_TEXTURESTAGE(3, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, 0                               },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_COLOROP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_COLORARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_COLORARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_COLORARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_ALPHAOP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_ALPHAARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_ALPHAARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_ALPHAARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_RESULTARG),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT00),      { STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT01),      { STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT10),      { STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT11),      { STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVLSCALE),     { STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, 0                               },
    {STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVLOFFSET),    { STATE_TEXTURESTAGE(4, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, 0                               },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_COLOROP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_COLORARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_COLORARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_COLORARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_ALPHAOP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_ALPHAARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_ALPHAARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_ALPHAARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_RESULTARG),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT00),      { STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT01),      { STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT10),      { STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT11),      { STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVLSCALE),     { STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, 0                               },
    {STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVLOFFSET),    { STATE_TEXTURESTAGE(5, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, 0                               },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_COLOROP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_COLORARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_COLORARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_COLORARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_ALPHAOP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_ALPHAARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_ALPHAARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_ALPHAARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_RESULTARG),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT00),      { STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT01),      { STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT10),      { STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT11),      { STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVLSCALE),     { STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, 0                               },
    {STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVLOFFSET),    { STATE_TEXTURESTAGE(6, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, 0                               },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_COLOROP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_COLORARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_COLORARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_COLORARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_ALPHAOP),           { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_ALPHAARG1),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_ALPHAARG2),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_ALPHAARG0),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_RESULTARG),         { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT00),      { STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT01),      { STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT10),      { STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT11),      { STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVMAT00),     set_bumpmat_arbfp       }, 0                               },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVLSCALE),     { STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, 0                               },
    {STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVLOFFSET),    { STATE_TEXTURESTAGE(7, WINED3DTSS_BUMPENVLSCALE),    tex_bumpenvlum_arbfp    }, 0                               },
    { STATE_SAMPLER(0),                                   { STATE_SAMPLER(0),                                   sampler_texdim          }, 0                               },
    { STATE_SAMPLER(1),                                   { STATE_SAMPLER(1),                                   sampler_texdim          }, 0                               },
    { STATE_SAMPLER(2),                                   { STATE_SAMPLER(2),                                   sampler_texdim          }, 0                               },
    { STATE_SAMPLER(3),                                   { STATE_SAMPLER(3),                                   sampler_texdim          }, 0                               },
    { STATE_SAMPLER(4),                                   { STATE_SAMPLER(4),                                   sampler_texdim          }, 0                               },
    { STATE_SAMPLER(5),                                   { STATE_SAMPLER(5),                                   sampler_texdim          }, 0                               },
    { STATE_SAMPLER(6),                                   { STATE_SAMPLER(6),                                   sampler_texdim          }, 0                               },
    { STATE_SAMPLER(7),                                   { STATE_SAMPLER(7),                                   sampler_texdim          }, 0                               },
    { STATE_PIXELSHADER,                                  { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    { STATE_RENDER(WINED3DRS_FOGENABLE),                  { STATE_RENDER(WINED3DRS_FOGENABLE),                  state_arbfp_fog         }, 0                               },
    { STATE_RENDER(WINED3DRS_FOGTABLEMODE),               { STATE_RENDER(WINED3DRS_FOGENABLE),                  state_arbfp_fog         }, 0                               },
    { STATE_RENDER(WINED3DRS_FOGVERTEXMODE),              { STATE_RENDER(WINED3DRS_FOGENABLE),                  state_arbfp_fog         }, 0                               },
    { STATE_RENDER(WINED3DRS_SRGBWRITEENABLE),            { STATE_PIXELSHADER,                                  fragment_prog_arbfp     }, 0                               },
    {STATE_TEXTURESTAGE(0,WINED3DTSS_TEXTURETRANSFORMFLAGS),{STATE_TEXTURESTAGE(0, WINED3DTSS_TEXTURETRANSFORMFLAGS), textransform      }, 0                               },
    {STATE_TEXTURESTAGE(1,WINED3DTSS_TEXTURETRANSFORMFLAGS),{STATE_TEXTURESTAGE(1, WINED3DTSS_TEXTURETRANSFORMFLAGS), textransform      }, 0                               },
    {STATE_TEXTURESTAGE(2,WINED3DTSS_TEXTURETRANSFORMFLAGS),{STATE_TEXTURESTAGE(2, WINED3DTSS_TEXTURETRANSFORMFLAGS), textransform      }, 0                               },
    {STATE_TEXTURESTAGE(3,WINED3DTSS_TEXTURETRANSFORMFLAGS),{STATE_TEXTURESTAGE(3, WINED3DTSS_TEXTURETRANSFORMFLAGS), textransform      }, 0                               },
    {STATE_TEXTURESTAGE(4,WINED3DTSS_TEXTURETRANSFORMFLAGS),{STATE_TEXTURESTAGE(4, WINED3DTSS_TEXTURETRANSFORMFLAGS), textransform      }, 0                               },
    {STATE_TEXTURESTAGE(5,WINED3DTSS_TEXTURETRANSFORMFLAGS),{STATE_TEXTURESTAGE(5, WINED3DTSS_TEXTURETRANSFORMFLAGS), textransform      }, 0                               },
    {STATE_TEXTURESTAGE(6,WINED3DTSS_TEXTURETRANSFORMFLAGS),{STATE_TEXTURESTAGE(6, WINED3DTSS_TEXTURETRANSFORMFLAGS), textransform      }, 0                               },
    {STATE_TEXTURESTAGE(7,WINED3DTSS_TEXTURETRANSFORMFLAGS),{STATE_TEXTURESTAGE(7, WINED3DTSS_TEXTURETRANSFORMFLAGS), textransform      }, 0                               },
    { STATE_RENDER(WINED3DRS_SPECULARENABLE),             { STATE_RENDER(WINED3DRS_SPECULARENABLE),             state_arb_specularenable}, 0                               },
    {0 /* Terminate */,                                   { 0,                                                  0                       }, 0                               },
};

const struct fragment_pipeline arbfp_fragment_pipeline = {
    arbfp_enable,
    arbfp_get_caps,
    arbfp_alloc,
    arbfp_free,
    shader_arb_conv_supported,
    arbfp_fragmentstate_template,
    TRUE /* We can disable projected textures */
};

#define GLINFO_LOCATION device->adapter->gl_info

struct arbfp_blit_priv {
    GLenum yuy2_rect_shader, yuy2_2d_shader;
    GLenum uyvy_rect_shader, uyvy_2d_shader;
    GLenum yv12_rect_shader, yv12_2d_shader;
};

static HRESULT arbfp_blit_alloc(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) iface;
    device->blit_priv = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct arbfp_blit_priv));
    if(!device->blit_priv) {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }
    return WINED3D_OK;
}
static void arbfp_blit_free(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) iface;
    struct arbfp_blit_priv *priv = (struct arbfp_blit_priv *) device->blit_priv;

    ENTER_GL();
    GL_EXTCALL(glDeleteProgramsARB(1, &priv->yuy2_rect_shader));
    GL_EXTCALL(glDeleteProgramsARB(1, &priv->yuy2_2d_shader));
    GL_EXTCALL(glDeleteProgramsARB(1, &priv->uyvy_rect_shader));
    GL_EXTCALL(glDeleteProgramsARB(1, &priv->uyvy_2d_shader));
    GL_EXTCALL(glDeleteProgramsARB(1, &priv->yv12_rect_shader));
    GL_EXTCALL(glDeleteProgramsARB(1, &priv->yv12_2d_shader));
    checkGLcall("Delete yuv programs\n");
    LEAVE_GL();
}

static BOOL gen_planar_yuv_read(SHADER_BUFFER *buffer, WINED3DFORMAT fmt, GLenum textype, char *luminance) {
    char chroma;
    const char *tex, *texinstr;

    if(fmt == WINED3DFMT_UYVY) {
        chroma = 'r';
        *luminance = 'a';
    } else {
        chroma = 'a';
        *luminance = 'r';
    }
    switch(textype) {
        case GL_TEXTURE_2D:             tex = "2D";     texinstr = "TXP"; break;
        case GL_TEXTURE_RECTANGLE_ARB:  tex = "RECT";   texinstr = "TEX"; break;
        default:
            /* This is more tricky than just replacing the texture type - we have to navigate
             * properly in the texture to find the correct chroma values
             */
            FIXME("Implement yuv correction for non-2d, non-rect textures\n");
            return FALSE;
    }

    /* First we have to read the chroma values. This means we need at least two pixels(no filtering),
     * or 4 pixels(with filtering). To get the unmodified chromas, we have to rid ourselves of the
     * filtering when we sample the texture.
     *
     * These are the rules for reading the chroma:
     *
     * Even pixel: Cr
     * Even pixel: U
     * Odd pixel: V
     *
     * So we have to get the sampling x position in non-normalized coordinates in integers
     */
    if(textype != GL_TEXTURE_RECTANGLE_ARB) {
        shader_addline(buffer, "MUL texcrd.rg, fragment.texcoord[0], size.x;\n");
        shader_addline(buffer, "MOV texcrd.a, size.x;\n");
    } else {
        shader_addline(buffer, "MOV texcrd, fragment.texcoord[0];\n");
    }
    /* We must not allow filtering between pixel x and x+1, this would mix U and V
     * Vertical filtering is ok. However, bear in mind that the pixel center is at
     * 0.5, so add 0.5.
     */
    shader_addline(buffer, "FLR texcrd.x, texcrd.x;\n");
    shader_addline(buffer, "ADD texcrd.x, texcrd.x, coef.y;\n");

    /* Divide the x coordinate by 0.5 and get the fraction. This gives 0.25 and 0.75 for the
     * even and odd pixels respectively
     */
    shader_addline(buffer, "MUL texcrd2, texcrd, coef.y;\n");
    shader_addline(buffer, "FRC texcrd2, texcrd2;\n");

    /* Sample Pixel 1 */
    shader_addline(buffer, "%s luminance, texcrd, texture[0], %s;\n", texinstr, tex);

    /* Put the value into either of the chroma values */
    shader_addline(buffer, "SGE temp.x, texcrd2.x, coef.y;\n");
    shader_addline(buffer, "MUL chroma.r, luminance.%c, temp.x;\n", chroma);
    shader_addline(buffer, "SLT temp.x, texcrd2.x, coef.y;\n");
    shader_addline(buffer, "MUL chroma.g, luminance.%c, temp.x;\n", chroma);

    /* Sample pixel 2. If we read an even pixel(SLT above returned 1), sample
     * the pixel right to the current one. Otherwise, sample the left pixel.
     * Bias and scale the SLT result to -1;1 and add it to the texcrd.x.
     */
    shader_addline(buffer, "MAD temp.x, temp.x, coef.z, -coef.x;\n");
    shader_addline(buffer, "ADD texcrd.x, texcrd, temp.x;\n");
    shader_addline(buffer, "%s luminance, texcrd, texture[0], %s;\n", texinstr, tex);

    /* Put the value into the other chroma */
    shader_addline(buffer, "SGE temp.x, texcrd2.x, coef.y;\n");
    shader_addline(buffer, "MAD chroma.g, luminance.%c, temp.x, chroma.g;\n", chroma);
    shader_addline(buffer, "SLT temp.x, texcrd2.x, coef.y;\n");
    shader_addline(buffer, "MAD chroma.r, luminance.%c, temp.x, chroma.r;\n", chroma);

    /* TODO: If filtering is enabled, sample a 2nd pair of pixels left or right of
     * the current one and lerp the two U and V values
     */

    /* This gives the correctly filtered luminance value */
    shader_addline(buffer, "TEX luminance, fragment.texcoord[0], texture[0], %s;\n", tex);

    return TRUE;
}

static BOOL gen_yv12_read(SHADER_BUFFER *buffer, WINED3DFORMAT fmt, GLenum textype, char *luminance) {
    const char *tex;

    switch(textype) {
        case GL_TEXTURE_2D:             tex = "2D";     break;
        case GL_TEXTURE_RECTANGLE_ARB:  tex = "RECT";   break;
        default:
            FIXME("Implement yv12 correction for non-2d, non-rect textures\n");
            return FALSE;
    }

    /* YV12 surfaces contain a WxH sized luminance plane, followed by a (W/2)x(H/2)
     * V and a (W/2)x(H/2) U plane, each with 8 bit per pixel. So the effective
     * bitdepth is 12 bits per pixel. Since the U and V planes have only half the
     * pitch of the luminance plane, the packing into the gl texture is a bit
     * unfortunate. If the whole texture is interpreted as luminance data it looks
     * approximately like this:
     *
     *        +----------------------------------+----
     *        |                                  |
     *        |                                  |
     *        |                                  |
     *        |                                  |
     *        |                                  |   2
     *        |            LUMINANCE             |   -
     *        |                                  |   3
     *        |                                  |
     *        |                                  |
     *        |                                  |
     *        |                                  |
     *        +----------------+-----------------+----
     *        |                |                 |
     *        |  U even rows   |  U odd rows     |
     *        |                |                 |   1
     *        +----------------+------------------   -
     *        |                |                 |   3
     *        |  V even rows   |  V odd rows     |
     *        |                |                 |
     *        +----------------+-----------------+----
     *        |                |                 |
     *        |     0.5        |       0.5       |
     *
     * So it appears as if there are 4 chroma images, but in fact the odd rows
     * in the chroma images are in the same row as the even ones. So its is
     * kinda tricky to read
     *
     * When reading from rectangle textures, keep in mind that the input y coordinates
     * go from 0 to d3d_height, whereas the opengl texture height is 1.5 * d3d_height
     */
    shader_addline(buffer, "PARAM yv12_coef = {%f, %f, %f, %f};\n",
                   2.0 / 3.0, 1.0 / 6.0, (2.0 / 3.0) + (1.0 / 6.0), 1.0 / 3.0);

    shader_addline(buffer, "MOV texcrd, fragment.texcoord[0];\n");
    /* the chroma planes have only half the width */
    shader_addline(buffer, "MUL texcrd.x, texcrd.x, coef.y;\n");

    /* The first value is between 2/3 and 5/6th of the texture's height, so scale+bias
     * the coordinate. Also read the right side of the image when reading odd lines
     *
     * Don't forget to clamp the y values in into the range, otherwise we'll get filtering
     * bleeding
     */
    if(textype == GL_TEXTURE_2D) {

        shader_addline(buffer, "RCP chroma.w, size.y;\n");

        shader_addline(buffer, "MUL texcrd2.y, texcrd.y, size.y;\n");

        shader_addline(buffer, "FLR texcrd2.y, texcrd2.y;\n");
        shader_addline(buffer, "MAD texcrd.y, texcrd.y, yv12_coef.y, yv12_coef.x;\n");

        /* Read odd lines from the right side(add size * 0.5 to the x coordinate */
        shader_addline(buffer, "ADD texcrd2.x, texcrd2.y, yv12_coef.y;\n"); /* To avoid 0.5 == 0.5 comparisons */
        shader_addline(buffer, "FRC texcrd2.x, texcrd2.x;\n");
        shader_addline(buffer, "SGE texcrd2.x, texcrd2.x, coef.y;\n");
        shader_addline(buffer, "MAD texcrd.x, texcrd2.x, coef.y, texcrd.x;\n");

        /* clamp, keep the half pixel origin in mind */
        shader_addline(buffer, "MAD temp.y, coef.y, chroma.w, yv12_coef.x;\n");
        shader_addline(buffer, "MAX texcrd.y, temp.y, texcrd.y;\n");
        shader_addline(buffer, "MAD temp.y, -coef.y, chroma.w, yv12_coef.z;\n");
        shader_addline(buffer, "MIN texcrd.y, temp.y, texcrd.y;\n");
    } else {
        /* Read from [size - size+size/4] */
        shader_addline(buffer, "FLR texcrd.y, texcrd.y;\n");
        shader_addline(buffer, "MAD texcrd.y, texcrd.y, coef.w, size.y;\n");

        /* Read odd lines from the right side(add size * 0.5 to the x coordinate */
        shader_addline(buffer, "ADD texcrd2.x, texcrd.y, yv12_coef.y;\n"); /* To avoid 0.5 == 0.5 comparisons */
        shader_addline(buffer, "FRC texcrd2.x, texcrd2.x;\n");
        shader_addline(buffer, "SGE texcrd2.x, texcrd2.x, coef.y;\n");
        shader_addline(buffer, "MUL texcrd2.x, texcrd2.x, size.x;\n");
        shader_addline(buffer, "MAD texcrd.x, texcrd2.x, coef.y, texcrd.x;\n");

        /* Make sure to read exactly from the pixel center */
        shader_addline(buffer, "FLR texcrd.y, texcrd.y;\n");
        shader_addline(buffer, "ADD texcrd.y, texcrd.y, coef.y;\n");

        /* Clamp */
        shader_addline(buffer, "MAD temp.y, size.y, coef.w, size.y;\n");
        shader_addline(buffer, "ADD temp.y, temp.y, -coef.y;\n");
        shader_addline(buffer, "MIN texcrd.y, temp.y, texcrd.y;\n");
        shader_addline(buffer, "ADD temp.y, size.y, -coef.y;\n");
        shader_addline(buffer, "MAX texcrd.y, temp.y, texcrd.y;\n");
    }
    /* Read the texture, put the result into the output register */
    shader_addline(buffer, "TEX temp, texcrd, texture[0], %s;\n", tex);
    shader_addline(buffer, "MOV chroma.r, temp.a;\n");

    /* The other chroma value is 1/6th of the texture lower, from 5/6th to 6/6th
     * No need to clamp because we're just reusing the already clamped value from above
     */
    if(textype == GL_TEXTURE_2D) {
        shader_addline(buffer, "ADD texcrd.y, texcrd.y, yv12_coef.y;\n");
    } else {
        shader_addline(buffer, "MAD texcrd.y, size.y, coef.w, texcrd.y;\n");
    }
    shader_addline(buffer, "TEX temp, texcrd, texture[0], %s;\n", tex);
    shader_addline(buffer, "MOV chroma.g, temp.a;\n");

    /* Sample the luminance value. It is in the top 2/3rd of the texture, so scale the y coordinate.
     * Clamp the y coordinate to prevent the chroma values from bleeding into the sampled luminance
     * values due to filtering
     */
    shader_addline(buffer, "MOV texcrd, fragment.texcoord[0];\n");
    if(textype == GL_TEXTURE_2D) {
        /* Multiply the y coordinate by 2/3 and clamp it */
        shader_addline(buffer, "MUL texcrd.y, texcrd.y, yv12_coef.x;\n");
        shader_addline(buffer, "MAD temp.y, -coef.y, chroma.w, yv12_coef.x;\n");
        shader_addline(buffer, "MIN texcrd.y, temp.y, texcrd.y;\n");
        shader_addline(buffer, "TEX luminance, texcrd, texture[0], %s;\n", tex);
    } else {
        /* Reading from texture_rectangles is pretty streightforward, just use the unmodified
         * texture coordinate. It is still a good idea to clamp it though, since the opengl texture
         * is bigger
         */
        shader_addline(buffer, "ADD temp.x, size.y, -coef.y;\n");
        shader_addline(buffer, "MIN texcrd.y, texcrd.y, size.x;\n");
        shader_addline(buffer, "TEX luminance, texcrd, texture[0], %s;\n", tex);
    }
    *luminance = 'a';

    return TRUE;
}

static GLuint gen_yuv_shader(IWineD3DDeviceImpl *device, WINED3DFORMAT fmt, GLenum textype) {
    GLenum shader;
    SHADER_BUFFER buffer;
    char luminance_component;
    struct arbfp_blit_priv *priv = (struct arbfp_blit_priv *) device->blit_priv;

    /* Shader header */
    buffer.bsize = 0;
    buffer.lineNo = 0;
    buffer.newline = TRUE;
    buffer.buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, SHADER_PGMSIZE);

    GL_EXTCALL(glGenProgramsARB(1, &shader));
    checkGLcall("GL_EXTCALL(glGenProgramsARB(1, &shader))");
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader));
    checkGLcall("glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader)");
    if(!shader) {
        HeapFree(GetProcessHeap(), 0, buffer.buffer);
        return 0;
    }

    /* The YUY2 and UYVY formats contain two pixels packed into a 32 bit macropixel,
     * giving effectively 16 bit per pixel. The color consists of a luminance(Y) and
     * two chroma(U and V) values. Each macropixel has two luminance values, one for
     * each single pixel it contains, and one U and one V value shared between both
     * pixels.
     *
     * The data is loaded into an A8L8 texture. With YUY2, the luminance component
     * contains the luminance and alpha the chroma. With UYVY it is vice versa. Thus
     * take the format into account when generating the read swizzles
     *
     * Reading the Y value is streightforward - just sample the texture. The hardware
     * takes care of filtering in the horizontal and vertical direction.
     *
     * Reading the U and V values is harder. We have to avoid filtering horizontally,
     * because that would mix the U and V values of one pixel or two adjacent pixels.
     * Thus floor the texture coordinate and add 0.5 to get an unfiltered read,
     * regardless of the filtering setting. Vertical filtering works automatically
     * though - the U and V values of two rows are mixed nicely.
     *
     * Appart of avoiding filtering issues, the code has to know which value it just
     * read, and where it can find the other one. To determine this, it checks if
     * it sampled an even or odd pixel, and shifts the 2nd read accordingly.
     *
     * Handling horizontal filtering of U and V values requires reading a 2nd pair
     * of pixels, extracting U and V and mixing them. This is not implemented yet.
     *
     * An alternative implementation idea is to load the texture as A8R8G8B8 texture,
     * with width / 2. This way one read gives all 3 values, finding U and V is easy
     * in an unfiltered situation. Finding the luminance on the other hand requires
     * finding out if it is an odd or even pixel. The real drawback of this approach
     * is filtering. This would have to be emulated completely in the shader, reading
     * up two 2 packed pixels in up to 2 rows and interpolating both horizontally and
     * vertically. Beyond that it would require adjustments to the texture handling
     * code to deal with the width scaling
     */
    shader_addline(&buffer, "!!ARBfp1.0\n");
    shader_addline(&buffer, "TEMP luminance;\n");
    shader_addline(&buffer, "TEMP temp;\n");
    shader_addline(&buffer, "TEMP chroma;\n");
    shader_addline(&buffer, "TEMP texcrd;\n");
    shader_addline(&buffer, "TEMP texcrd2;\n");
    shader_addline(&buffer, "PARAM coef = {1.0, 0.5, 2.0, 0.25};\n");
    shader_addline(&buffer, "PARAM yuv_coef = {1.403, 0.344, 0.714, 1.770};\n");
    shader_addline(&buffer, "PARAM size = program.local[0];\n");

    if(fmt == WINED3DFMT_UYVY || fmt ==WINED3DFMT_YUY2) {
        if(gen_planar_yuv_read(&buffer, fmt, textype, &luminance_component) == FALSE) {
            HeapFree(GetProcessHeap(), 0, buffer.buffer);
            return 0;
        }
    } else {
        if(gen_yv12_read(&buffer, fmt, textype, &luminance_component) == FALSE) {
            HeapFree(GetProcessHeap(), 0, buffer.buffer);
            return 0;
        }
    }

    /* Calculate the final result. Formula is taken from
     * http://www.fourcc.org/fccyvrgb.php. Note that the chroma
     * ranges from -0.5 to 0.5
     */
    shader_addline(&buffer, "SUB chroma.rg, chroma, coef.y;\n");

    shader_addline(&buffer, "MAD result.color.r, chroma.r, yuv_coef.x, luminance.%c;\n", luminance_component);
    shader_addline(&buffer, "MAD temp.r, -chroma.g, yuv_coef.y, luminance.%c;\n", luminance_component);
    shader_addline(&buffer, "MAD result.color.g, -chroma.r, yuv_coef.z, temp.r;\n");
    shader_addline(&buffer, "MAD result.color.b, chroma.g, yuv_coef.w, luminance.%c;\n", luminance_component);
    shader_addline(&buffer, "END\n");

    GL_EXTCALL(glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(buffer.buffer), buffer.buffer));

    if (glGetError() == GL_INVALID_OPERATION) {
        GLint pos;
        glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &pos);
        FIXME("Fragment program error at position %d: %s\n", pos,
              debugstr_a((const char *)glGetString(GL_PROGRAM_ERROR_STRING_ARB)));
    }
    HeapFree(GetProcessHeap(), 0, buffer.buffer);

    if(fmt == WINED3DFMT_YUY2) {
        if(textype == GL_TEXTURE_RECTANGLE_ARB) {
            priv->yuy2_rect_shader = shader;
        } else {
            priv->yuy2_2d_shader = shader;
        }
    } else if(fmt == WINED3DFMT_UYVY) {
        if(textype == GL_TEXTURE_RECTANGLE_ARB) {
            priv->uyvy_rect_shader = shader;
        } else {
            priv->uyvy_2d_shader = shader;
        }
    } else {
        if(textype == GL_TEXTURE_RECTANGLE_ARB) {
            priv->yv12_rect_shader = shader;
        } else {
            priv->yv12_2d_shader = shader;
        }
    }
    return shader;
}

static HRESULT arbfp_blit_set(IWineD3DDevice *iface, WINED3DFORMAT fmt, GLenum textype, UINT width, UINT height) {
    GLenum shader;
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) iface;
    float size[4] = {width, height, 1, 1};
    struct arbfp_blit_priv *priv = (struct arbfp_blit_priv *) device->blit_priv;
    const GlPixelFormatDesc *glDesc;

    getFormatDescEntry(fmt, &GLINFO_LOCATION, &glDesc);

    if(glDesc->conversion_group != WINED3DFMT_YUY2 && glDesc->conversion_group != WINED3DFMT_UYVY &&
       glDesc->conversion_group != WINED3DFMT_YV12) {
        TRACE("Format: %s\n", debug_d3dformat(glDesc->conversion_group));
        /* Don't bother setting up a shader for unconverted formats */
        glEnable(textype);
        checkGLcall("glEnable(textype)");
        return WINED3D_OK;
    }

    if(glDesc->conversion_group == WINED3DFMT_YUY2) {
        if(textype == GL_TEXTURE_RECTANGLE_ARB) {
            shader = priv->yuy2_rect_shader;
        } else {
            shader = priv->yuy2_2d_shader;
        }
    } else if(glDesc->conversion_group == WINED3DFMT_UYVY) {
        if(textype == GL_TEXTURE_RECTANGLE_ARB) {
            shader = priv->uyvy_rect_shader;
        } else {
            shader = priv->uyvy_2d_shader;
        }
    } else {
        if(textype == GL_TEXTURE_RECTANGLE_ARB) {
            shader = priv->yv12_rect_shader;
        } else {
            shader = priv->yv12_2d_shader;
        }
    }

    if(!shader) {
        shader = gen_yuv_shader(device, glDesc->conversion_group, textype);
    }

    glEnable(GL_FRAGMENT_PROGRAM_ARB);
    checkGLcall("glEnable(GL_FRAGMENT_PROGRAM_ARB)");
    GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader));
    checkGLcall("glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, shader)");
    GL_EXTCALL(glProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0, size));
    checkGLcall("glProgramLocalParameter4fvARB");

    return WINED3D_OK;
}

static void arbfp_blit_unset(IWineD3DDevice *iface) {
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) iface;
    glDisable(GL_FRAGMENT_PROGRAM_ARB);
    checkGLcall("glDisable(GL_FRAGMENT_PROGRAM_ARB)");
    glDisable(GL_TEXTURE_2D);
    checkGLcall("glDisable(GL_TEXTURE_2D)");
    if(GL_SUPPORT(ARB_TEXTURE_CUBE_MAP)) {
        glDisable(GL_TEXTURE_CUBE_MAP_ARB);
        checkGLcall("glDisable(GL_TEXTURE_CUBE_MAP_ARB)");
    }
    if(GL_SUPPORT(ARB_TEXTURE_RECTANGLE)) {
        glDisable(GL_TEXTURE_RECTANGLE_ARB);
        checkGLcall("glDisable(GL_TEXTURE_RECTANGLE_ARB)");
    }
}

static BOOL arbfp_blit_conv_supported(WINED3DFORMAT fmt) {
    TRACE("Checking blit format support for format %s:", debug_d3dformat(fmt));
    switch(fmt) {
        case WINED3DFMT_YUY2:
        case WINED3DFMT_UYVY:
        case WINED3DFMT_YV12:
            TRACE("[OK]\n");
            return TRUE;
        default:
            TRACE("[FAILED]\n");
            return FALSE;
    }
}

const struct blit_shader arbfp_blit = {
    arbfp_blit_alloc,
    arbfp_blit_free,
    arbfp_blit_set,
    arbfp_blit_unset,
    arbfp_blit_conv_supported
};

#undef GLINFO_LOCATION
