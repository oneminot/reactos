/*
 * IDL Compiler
 *
 * Copyright 2005-2006 Eric Kohl
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
#include "wine/port.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>
#include <ctype.h>

#include "widl.h"
#include "utils.h"
#include "parser.h"
#include "header.h"

#include "typegen.h"

static FILE* server;
static int indent = 0;


static void print_server(const char *format, ...)
{
    va_list va;
    va_start(va, format);
    print(server, indent, format, va);
    va_end(va);
}

static void write_function_stubs(type_t *iface, unsigned int *proc_offset)
{
    const statement_t *stmt;
    const var_t *var;
    const var_t* explicit_handle_var;

    STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
    {
        const var_t *func = stmt->u.var;
        int has_full_pointer = is_full_pointer_function(func);

        /* check for a defined binding handle */
        explicit_handle_var = get_explicit_handle_var(func);

        print_server("struct __frame_%s_%s\n{\n", iface->name, get_name(func));
        indent++;
        print_server("__DECL_EXCEPTION_FRAME\n");
        print_server("MIDL_STUB_MESSAGE _StubMsg;\n");

        /* Declare arguments */
        declare_stub_args(server, indent, func);

        indent--;
        print_server("};\n\n");

        print_server("static void __finally_%s_%s(", iface->name, get_name(func));
        fprintf(server," struct __frame_%s_%s *__frame )\n{\n", iface->name, get_name(func));

        indent++;
        write_remoting_arguments(server, indent, func, "__frame->", PASS_OUT, PHASE_FREE);

        if (has_full_pointer)
            write_full_pointer_free(server, indent, func);

        indent--;
        print_server("}\n\n");

        print_server("void __RPC_STUB %s_%s( PRPC_MESSAGE _pRpcMessage )\n", iface->name, get_name(func));

        /* write the functions body */
        fprintf(server, "{\n");
        indent++;
        print_server("struct __frame_%s_%s __f, * const __frame = &__f;\n", iface->name, get_name(func));
        if (has_out_arg_or_return(func)) print_server("RPC_STATUS _Status;\n");
        fprintf(server, "\n");

        print_server("NdrServerInitializeNew(\n");
        indent++;
        print_server("_pRpcMessage,\n");
        print_server("&__frame->_StubMsg,\n");
        print_server("&%s_StubDesc);\n", iface->name);
        indent--;
        fprintf(server, "\n");
        print_server( "RpcExceptionInit( __server_filter, __finally_%s_%s );\n", iface->name, get_name(func));

        write_parameters_init(server, indent, func, "__frame->");

        if (explicit_handle_var)
        {
            print_server("__frame->%s = _pRpcMessage->Handle;\n", explicit_handle_var->name);
            fprintf(server, "\n");
        }

        print_server("RpcTryFinally\n");
        print_server("{\n");
        indent++;
        print_server("RpcTryExcept\n");
        print_server("{\n");
        indent++;

        if (has_full_pointer)
            write_full_pointer_init(server, indent, func, TRUE);

        if (type_get_function_args(func->type))
        {
            print_server("if ((_pRpcMessage->DataRepresentation & 0x0000FFFFUL) != NDR_LOCAL_DATA_REPRESENTATION)\n");
            indent++;
            print_server("NdrConvert(&__frame->_StubMsg, (PFORMAT_STRING)&__MIDL_ProcFormatString.Format[%u]);\n",
                         *proc_offset);
            indent--;
            fprintf(server, "\n");

            /* unmarshall arguments */
            write_remoting_arguments(server, indent, func, "__frame->", PASS_IN, PHASE_UNMARSHAL);
        }

        print_server("if (__frame->_StubMsg.Buffer > __frame->_StubMsg.BufferEnd)\n");
        print_server("{\n");
        indent++;
        print_server("RpcRaiseException(RPC_X_BAD_STUB_DATA);\n");
        indent--;
        print_server("}\n");
        indent--;
        print_server("}\n");
        print_server("RpcExcept(RPC_BAD_STUB_DATA_EXCEPTION_FILTER)\n");
        print_server("{\n");
        indent++;
        print_server("RpcRaiseException(RPC_X_BAD_STUB_DATA);\n");
        indent--;
        print_server("}\n");
        print_server("RpcEndExcept\n");
        fprintf(server, "\n");

        /* Assign 'out' arguments */
        assign_stub_out_args(server, indent, func, "__frame->");

        /* Call the real server function */
        if (!is_void(type_function_get_rettype(func->type)))
            print_server("__frame->_RetVal = ");
        else
            print_server("");
        fprintf(server, "%s%s", prefix_server, get_name(func));

        if (type_get_function_args(func->type))
        {
            int first_arg = 1;

            fprintf(server, "(\n");
            indent++;
            LIST_FOR_EACH_ENTRY( var, type_get_function_args(func->type), const var_t, entry )
            {
                if (first_arg)
                    first_arg = 0;
                else
                    fprintf(server, ",\n");
                if (is_context_handle(var->type))
                {
                    /* if the context_handle attribute appears in the chain of types
                     * without pointers being followed, then the context handle must
                     * be direct, otherwise it is a pointer */
                    int is_ch_ptr = is_aliaschain_attr(var->type, ATTR_CONTEXTHANDLE) ? FALSE : TRUE;
                    print_server("(");
                    write_type_decl_left(server, var->type);
                    fprintf(server, ")%sNDRSContextValue(__frame->%s)",
                            is_ch_ptr ? "" : "*", var->name);
                }
                else
                {
                    print_server("%s__frame->%s", var->type->declarray ? "*" : "", var->name);
                }
            }
            fprintf(server, ");\n");
            indent--;
        }
        else
        {
            fprintf(server, "();\n");
        }

        if (has_out_arg_or_return(func))
        {
            write_remoting_arguments(server, indent, func, "__frame->", PASS_OUT, PHASE_BUFFERSIZE);

            if (!is_void(type_function_get_rettype(func->type)))
                write_remoting_arguments(server, indent, func, "__frame->", PASS_RETURN, PHASE_BUFFERSIZE);

            print_server("_pRpcMessage->BufferLength = __frame->_StubMsg.BufferLength;\n");
            fprintf(server, "\n");
            print_server("_Status = I_RpcGetBuffer(_pRpcMessage);\n");
            print_server("if (_Status)\n");
            indent++;
            print_server("RpcRaiseException(_Status);\n");
            indent--;
            fprintf(server, "\n");
            print_server("__frame->_StubMsg.Buffer = _pRpcMessage->Buffer;\n");
            fprintf(server, "\n");
        }

        /* marshall arguments */
        write_remoting_arguments(server, indent, func, "__frame->", PASS_OUT, PHASE_MARSHAL);

        /* marshall the return value */
        if (!is_void(type_function_get_rettype(func->type)))
            write_remoting_arguments(server, indent, func, "__frame->", PASS_RETURN, PHASE_MARSHAL);

        indent--;
        print_server("}\n");
        print_server("RpcFinally\n");
        print_server("{\n");
        indent++;
        print_server("__finally_%s_%s( __frame );\n", iface->name, get_name(func));
        indent--;
        print_server("}\n");
        print_server("RpcEndFinally\n");

        /* calculate buffer length */
        fprintf(server, "\n");
        print_server("_pRpcMessage->BufferLength = __frame->_StubMsg.Buffer - (unsigned char *)_pRpcMessage->Buffer;\n");
        indent--;
        fprintf(server, "}\n");
        fprintf(server, "\n");

        /* update proc_offset */
        *proc_offset += get_size_procformatstring_func( func );
    }
}


static void write_dispatchtable(type_t *iface)
{
    unsigned long ver = get_attrv(iface->attrs, ATTR_VERSION);
    unsigned long method_count = 0;
    const statement_t *stmt;

    print_server("static RPC_DISPATCH_FUNCTION %s_table[] =\n", iface->name);
    print_server("{\n");
    indent++;

    STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
    {
        var_t *func = stmt->u.var;
        print_server("%s_%s,\n", iface->name, get_name(func));
        method_count++;
    }
    print_server("0\n");
    indent--;
    print_server("};\n");
    print_server("static RPC_DISPATCH_TABLE %s_v%d_%d_DispatchTable =\n", iface->name, MAJORVERSION(ver), MINORVERSION(ver));
    print_server("{\n");
    indent++;
    print_server("%u,\n", method_count);
    print_server("%s_table\n", iface->name);
    indent--;
    print_server("};\n");
    fprintf(server, "\n");
}


static void write_stubdescdecl(type_t *iface)
{
    print_server("static const MIDL_STUB_DESC %s_StubDesc;\n", iface->name);
    fprintf(server, "\n");
}


static void write_stubdescriptor(type_t *iface, int expr_eval_routines)
{
    print_server("static const MIDL_STUB_DESC %s_StubDesc =\n", iface->name);
    print_server("{\n");
    indent++;
    print_server("(void *)& %s___RpcServerInterface,\n", iface->name);
    print_server("MIDL_user_allocate,\n");
    print_server("MIDL_user_free,\n");
    print_server("{\n");
    indent++;
    print_server("0,\n");
    indent--;
    print_server("},\n");
    print_server("0,\n");
    print_server("0,\n");
    if (expr_eval_routines)
        print_server("ExprEvalRoutines,\n");
    else
        print_server("0,\n");
    print_server("0,\n");
    print_server("__MIDL_TypeFormatString.Format,\n");
    print_server("1, /* -error bounds_check flag */\n");
    print_server("0x10001, /* Ndr library version */\n");
    print_server("0,\n");
    print_server("0x50100a4, /* MIDL Version 5.1.164 */\n");
    print_server("0,\n");
    print_server("%s,\n", list_empty(&user_type_list) ? "0" : "UserMarshalRoutines");
    print_server("0,  /* notify & notify_flag routine table */\n");
    print_server("1,  /* Flags */\n");
    print_server("0,  /* Reserved3 */\n");
    print_server("0,  /* Reserved4 */\n");
    print_server("0   /* Reserved5 */\n");
    indent--;
    print_server("};\n");
    fprintf(server, "\n");
}


static void write_serverinterfacedecl(type_t *iface)
{
    unsigned long ver = get_attrv(iface->attrs, ATTR_VERSION);
    UUID *uuid = get_attrp(iface->attrs, ATTR_UUID);
    const str_list_t *endpoints = get_attrp(iface->attrs, ATTR_ENDPOINT);

    if (endpoints) write_endpoints( server, iface->name, endpoints );

    print_server("static RPC_DISPATCH_TABLE %s_v%d_%d_DispatchTable;\n", iface->name, MAJORVERSION(ver), MINORVERSION(ver));
    fprintf(server, "\n");
    print_server("static const RPC_SERVER_INTERFACE %s___RpcServerInterface =\n", iface->name );
    print_server("{\n");
    indent++;
    print_server("sizeof(RPC_SERVER_INTERFACE),\n");
    print_server("{{0x%08lx,0x%04x,0x%04x,{0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x}},{%d,%d}},\n",
                 uuid->Data1, uuid->Data2, uuid->Data3, uuid->Data4[0], uuid->Data4[1],
                 uuid->Data4[2], uuid->Data4[3], uuid->Data4[4], uuid->Data4[5], uuid->Data4[6],
                 uuid->Data4[7], MAJORVERSION(ver), MINORVERSION(ver));
    print_server("{{0x8a885d04,0x1ceb,0x11c9,{0x9f,0xe8,0x08,0x00,0x2b,0x10,0x48,0x60}},{2,0}},\n"); /* FIXME */
    print_server("&%s_v%d_%d_DispatchTable,\n", iface->name, MAJORVERSION(ver), MINORVERSION(ver));
    if (endpoints)
    {
        print_server("%u,\n", list_count(endpoints));
        print_server("(PRPC_PROTSEQ_ENDPOINT)%s__RpcProtseqEndpoint,\n", iface->name);
    }
    else
    {
        print_server("0,\n");
        print_server("0,\n");
    }
    print_server("0,\n");
    print_server("0,\n");
    print_server("0,\n");
    indent--;
    print_server("};\n");
    if (old_names)
        print_server("RPC_IF_HANDLE %s_ServerIfHandle DECLSPEC_HIDDEN = (RPC_IF_HANDLE)& %s___RpcServerInterface;\n",
                     iface->name, iface->name);
    else
        print_server("RPC_IF_HANDLE %s%s_v%d_%d_s_ifspec DECLSPEC_HIDDEN = (RPC_IF_HANDLE)& %s___RpcServerInterface;\n",
                     prefix_server, iface->name, MAJORVERSION(ver), MINORVERSION(ver), iface->name);
    fprintf(server, "\n");
}


static void init_server(void)
{
    if (server)
        return;
    if (!(server = fopen(server_name, "w")))
        error("Could not open %s for output\n", server_name);

    print_server("/*** Autogenerated by WIDL %s from %s - Do not edit ***/\n", PACKAGE_VERSION, input_name);
    print_server("#include <string.h>\n");
    fprintf(server, "\n");
    print_server("#include \"%s\"\n", header_name);
    print_server("\n");
    print_server( "#ifndef DECLSPEC_HIDDEN\n");
    print_server( "#define DECLSPEC_HIDDEN\n");
    print_server( "#endif\n");
    print_server( "\n");
    write_exceptions( server );
    print_server("\n");
    print_server("struct __server_frame\n");
    print_server("{\n");
    print_server("    __DECL_EXCEPTION_FRAME\n");
    print_server("    MIDL_STUB_MESSAGE _StubMsg;\n");
    print_server("};\n");
    print_server("\n");
    print_server("#ifndef USE_COMPILER_EXCEPTIONS\n");
    print_server("static int __server_filter( struct __server_frame *__frame )\n");
    print_server( "{\n");
    print_server( "    return RPC_BAD_STUB_DATA_EXCEPTION_FILTER;\n");
    print_server( "}\n");
    print_server("#endif /* USE_COMPILER_EXCEPTIONS */\n");
    print_server( "\n");
}


static void write_server_stmts(const statement_list_t *stmts, int expr_eval_routines, unsigned int *proc_offset)
{
    const statement_t *stmt;
    if (stmts) LIST_FOR_EACH_ENTRY( stmt, stmts, const statement_t, entry )
    {
        if (stmt->type == STMT_LIBRARY)
            write_server_stmts(stmt->u.lib->stmts, expr_eval_routines, proc_offset);
        else if (stmt->type == STMT_TYPE && stmt->u.type->type == RPC_FC_IP)
        {
            type_t *iface = stmt->u.type;
            if (!need_stub(iface))
                continue;

            fprintf(server, "/*****************************************************************************\n");
            fprintf(server, " * %s interface\n", iface->name);
            fprintf(server, " */\n");
            fprintf(server, "\n");

            if (statements_has_func(type_iface_get_stmts(iface)))
            {
                write_serverinterfacedecl(iface);
                write_stubdescdecl(iface);

                write_function_stubs(iface, proc_offset);

                print_server("#if !defined(__RPC_WIN%u__)\n", pointer_size == 8 ? 64 : 32);
                print_server("#error  Invalid build platform for this stub.\n");
                print_server("#endif\n");

                fprintf(server, "\n");
                write_stubdescriptor(iface, expr_eval_routines);
                write_dispatchtable(iface);
            }
        }
    }
}

static void write_server_routines(const statement_list_t *stmts)
{
    unsigned int proc_offset = 0;
    int expr_eval_routines;

    write_formatstringsdecl(server, indent, stmts, need_stub);
    expr_eval_routines = write_expr_eval_routines(server, server_token);
    if (expr_eval_routines)
        write_expr_eval_routine_list(server, server_token);
    write_user_quad_list(server);

    write_server_stmts(stmts, expr_eval_routines, &proc_offset);

    fprintf(server, "\n");

    write_procformatstring(server, stmts, need_stub);
    write_typeformatstring(server, stmts, need_stub);
}

void write_server(const statement_list_t *stmts)
{
    if (!do_server)
        return;
    if (do_everything && !need_stub_files(stmts))
        return;

    init_server();
    if (!server)
        return;

    if (do_win32 && do_win64)
    {
        fprintf(server, "\n#ifndef _WIN64\n\n");
        pointer_size = 4;
        write_server_routines( stmts );
        fprintf(server, "\n#else /* _WIN64 */\n\n");
        pointer_size = 8;
        write_server_routines( stmts );
        fprintf(server, "\n#endif /* _WIN64 */\n");
    }
    else if (do_win32)
    {
        pointer_size = 4;
        write_server_routines( stmts );
    }
    else if (do_win64)
    {
        pointer_size = 8;
        write_server_routines( stmts );
    }

    fclose(server);
}
