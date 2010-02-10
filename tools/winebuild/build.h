/*
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Martin von Loewis
 * Copyright 1995, 1996, 1997 Alexandre Julliard
 * Copyright 1997 Eric Youngdale
 * Copyright 1999 Ulrich Weigand
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

#ifndef __WINE_BUILD_H
#define __WINE_BUILD_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef max
#define max(a,b)   (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)   (((a) < (b)) ? (a) : (b))
#endif

#define EXEEXT ".exe"

typedef enum
{
    TYPE_VARIABLE,     /* variable */
    TYPE_PASCAL,       /* pascal function (Win16) */
    TYPE_ABS,          /* absolute value (Win16) */
    TYPE_STUB,         /* unimplemented stub */
    TYPE_STDCALL,      /* stdcall function (Win32) */
    TYPE_CDECL,        /* cdecl function (Win32) */
    TYPE_VARARGS,      /* varargs function (Win32) */
    TYPE_FASTCALL,     /* fastcall function (Win32) */
    TYPE_EXTERN,       /* external symbol (Win32) */
    TYPE_NBTYPES
} ORD_TYPE;

typedef enum
{
    SPEC_WIN16,
    SPEC_WIN32
} SPEC_TYPE;

typedef struct
{
    int n_values;
    int *values;
} ORD_VARIABLE;

typedef struct
{
    char arg_types[21];
} ORD_FUNCTION;

typedef struct
{
    unsigned short value;
} ORD_ABS;

typedef struct
{
    ORD_TYPE    type;
    int         ordinal;
    int         lineno;
    int         flags;
    char       *name;         /* public name of this function */
    char       *link_name;    /* name of the C symbol to link to */
    char       *export_name;  /* name exported under for noname exports */
    union
    {
        ORD_VARIABLE   var;
        ORD_FUNCTION   func;
        ORD_ABS        abs;
    } u;
} ORDDEF;

typedef struct
{
    char            *src_name;           /* file name of the source spec file */
    char            *file_name;          /* file name of the dll */
    char            *dll_name;           /* internal name of the dll */
    char            *init_func;          /* initialization routine */
    char            *main_module;        /* main Win32 module for Win16 specs */
    SPEC_TYPE        type;               /* type of dll (Win16/Win32) */
    int              base;               /* ordinal base */
    int              limit;              /* ordinal limit */
    int              stack_size;         /* exe stack size */
    int              heap_size;          /* exe heap size */
    int              nb_entry_points;    /* number of used entry points */
    int              alloc_entry_points; /* number of allocated entry points */
    int              nb_names;           /* number of entry points with names */
    unsigned int     nb_resources;       /* number of resources */
    int              characteristics;    /* characteristics for the PE header */
    int              dll_characteristics;/* DLL characteristics for the PE header */
    int              subsystem;          /* subsystem id */
    int              subsystem_major;    /* subsystem version major number */
    int              subsystem_minor;    /* subsystem version minor number */
    ORDDEF          *entry_points;       /* dll entry points */
    ORDDEF         **names;              /* array of entry point names (points into entry_points) */
    ORDDEF         **ordinals;           /* array of dll ordinals (points into entry_points) */
    struct resource *resources;          /* array of dll resources (format differs between Win16/Win32) */
} DLLSPEC;

enum target_cpu
{
    CPU_x86, CPU_x86_64, CPU_SPARC, CPU_ALPHA, CPU_POWERPC, CPU_ARM, CPU_LAST = CPU_ARM
};

enum target_platform
{
    PLATFORM_UNSPECIFIED,
    PLATFORM_APPLE,
    PLATFORM_FREEBSD,
    PLATFORM_SOLARIS,
    PLATFORM_WINDOWS
};

extern char *target_alias;
extern enum target_cpu target_cpu;
extern enum target_platform target_platform;

/* entry point flags */
#define FLAG_NORELAY   0x01  /* don't use relay debugging for this function */
#define FLAG_NONAME    0x02  /* don't export function by name */
#define FLAG_RET16     0x04  /* function returns a 16-bit value */
#define FLAG_RET64     0x08  /* function returns a 64-bit value */
#define FLAG_REGISTER  0x10  /* use register calling convention */
#define FLAG_PRIVATE   0x20  /* function is private (cannot be imported) */
#define FLAG_ORDINAL   0x40  /* function should be imported by ordinal */

#define FLAG_FORWARD   0x100  /* function is a forwarded name */
#define FLAG_EXT_LINK  0x200  /* function links to an external symbol */

#define FLAG_CPU(cpu)  (0x01000 << (cpu))
#define FLAG_CPU_MASK  (FLAG_CPU(CPU_LAST + 1) - FLAG_CPU(0))
#define FLAG_CPU_WIN64 (FLAG_CPU(CPU_x86_64))
#define FLAG_CPU_WIN32 (FLAG_CPU_MASK & ~FLAG_CPU_WIN64)

#define MAX_ORDINALS  65535

/* some Windows constants */

#define IMAGE_FILE_RELOCS_STRIPPED	   0x0001 /* No relocation info */
#define IMAGE_FILE_EXECUTABLE_IMAGE	   0x0002
#define IMAGE_FILE_LINE_NUMS_STRIPPED      0x0004
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED     0x0008
#define IMAGE_FILE_AGGRESIVE_WS_TRIM	   0x0010
#define IMAGE_FILE_LARGE_ADDRESS_AWARE	   0x0020
#define IMAGE_FILE_16BIT_MACHINE	   0x0040
#define IMAGE_FILE_BYTES_REVERSED_LO	   0x0080
#define IMAGE_FILE_32BIT_MACHINE	   0x0100
#define IMAGE_FILE_DEBUG_STRIPPED	   0x0200
#define IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP 0x0400
#define IMAGE_FILE_NET_RUN_FROM_SWAP	   0x0800
#define IMAGE_FILE_SYSTEM		   0x1000
#define IMAGE_FILE_DLL			   0x2000
#define IMAGE_FILE_UP_SYSTEM_ONLY	   0x4000
#define IMAGE_FILE_BYTES_REVERSED_HI	   0x8000

#define IMAGE_DLLCHARACTERISTICS_NX_COMPAT 0x0100

#define	IMAGE_SUBSYSTEM_NATIVE      1
#define	IMAGE_SUBSYSTEM_WINDOWS_GUI 2
#define	IMAGE_SUBSYSTEM_WINDOWS_CUI 3

/* global functions */

#ifndef __GNUC__
#define __attribute__(X)
#endif

#ifndef DECLSPEC_NORETURN
# if defined(_MSC_VER) && (_MSC_VER >= 1200) && !defined(MIDL_PASS)
#  define DECLSPEC_NORETURN __declspec(noreturn)
# else
#  define DECLSPEC_NORETURN __attribute__((noreturn))
# endif
#endif

extern void *xmalloc (size_t size);
extern void *xrealloc (void *ptr, size_t size);
extern char *xstrdup( const char *str );
extern char *strupper(char *s);
extern int strendswith(const char* str, const char* end);
extern DECLSPEC_NORETURN void fatal_error( const char *msg, ... )
   __attribute__ ((__format__ (__printf__, 1, 2)));
extern DECLSPEC_NORETURN void fatal_perror( const char *msg, ... )
   __attribute__ ((__format__ (__printf__, 1, 2)));
extern void error( const char *msg, ... )
   __attribute__ ((__format__ (__printf__, 1, 2)));
extern void warning( const char *msg, ... )
   __attribute__ ((__format__ (__printf__, 1, 2)));
extern int output( const char *format, ... )
   __attribute__ ((__format__ (__printf__, 1, 2)));
extern const char *get_as_command(void);
extern const char *get_ld_command(void);
extern const char *get_nm_command(void);
extern const char *get_windres_command(void);
extern char *get_temp_file_name( const char *prefix, const char *suffix );
extern void output_standard_file_header(void);
extern FILE *open_input_file( const char *srcdir, const char *name );
extern void close_input_file( FILE *file );
extern void dump_bytes( const void *buffer, unsigned int size );
extern int remove_stdcall_decoration( char *name );
extern void assemble_file( const char *src_file, const char *obj_file );
extern DLLSPEC *alloc_dll_spec(void);
extern void free_dll_spec( DLLSPEC *spec );
extern const char *make_c_identifier( const char *str );
extern const char *get_stub_name( const ORDDEF *odp, const DLLSPEC *spec );
extern enum target_cpu get_cpu_from_name( const char *name );
extern unsigned int get_alignment(unsigned int align);
extern unsigned int get_page_size(void);
extern unsigned int get_ptr_size(void);
extern const char *asm_name( const char *func );
extern const char *func_declaration( const char *func );
extern const char *asm_globl( const char *func );
extern const char *get_asm_ptr_keyword(void);
extern const char *get_asm_string_keyword(void);
extern const char *get_asm_short_keyword(void);
extern const char *get_asm_rodata_section(void);
extern const char *get_asm_string_section(void);
extern void output_function_size( const char *name );
extern void output_gnu_stack_note(void);

extern void add_import_dll( const char *name, const char *filename );
extern void add_delayed_import( const char *name );
extern void add_ignore_symbol( const char *name );
extern void add_extra_ld_symbol( const char *name );
extern void read_undef_symbols( DLLSPEC *spec, char **argv );
extern int resolve_imports( DLLSPEC *spec );
extern int has_imports(void);
extern int has_relays( DLLSPEC *spec );
extern void output_get_pc_thunk(void);
extern void output_module( DLLSPEC *spec );
extern void output_stubs( DLLSPEC *spec );
extern void output_imports( DLLSPEC *spec );
extern void output_exports( DLLSPEC *spec );
extern int load_res32_file( const char *name, DLLSPEC *spec );
extern void output_resources( DLLSPEC *spec );
extern void output_bin_resources( DLLSPEC *spec, unsigned int start_rva );
extern void output_fake_module( DLLSPEC *spec );
extern void load_res16_file( const char *name, DLLSPEC *spec );
extern void output_res16_data( DLLSPEC *spec );
extern void output_bin_res16_data( DLLSPEC *spec );
extern void output_res16_directory( DLLSPEC *spec );
extern void output_bin_res16_directory( DLLSPEC *spec, unsigned int data_offset );
extern void output_spec16_file( DLLSPEC *spec );
extern void output_fake_module16( DLLSPEC *spec16 );
extern void output_res_o_file( DLLSPEC *spec );

extern void BuildRelays16(void);
extern void BuildRelays32(void);
extern void BuildSpec16File( DLLSPEC *spec );
extern void BuildSpec32File( DLLSPEC *spec );
extern void BuildDef32File( DLLSPEC *spec );
extern void BuildPedllFile( DLLSPEC *spec );

extern void add_16bit_exports( DLLSPEC *spec32, DLLSPEC *spec16 );
extern int parse_spec_file( FILE *file, DLLSPEC *spec );
extern int parse_def_file( FILE *file, DLLSPEC *spec );

/* buffer management */

extern int byte_swapped;
extern const char *input_buffer_filename;
extern const unsigned char *input_buffer;
extern size_t input_buffer_pos;
extern size_t input_buffer_size;
extern unsigned char *output_buffer;
extern size_t output_buffer_pos;
extern size_t output_buffer_size;

extern void init_input_buffer( const char *file );
extern void init_output_buffer(void);
extern void flush_output_buffer(void);
extern unsigned char get_byte(void);
extern unsigned short get_word(void);
extern unsigned int get_dword(void);
extern void put_data( const void *data, size_t size );
extern void put_byte( unsigned char val );
extern void put_word( unsigned short val );
extern void put_dword( unsigned int val );
extern void put_qword( unsigned int val );
extern void put_pword( unsigned int val );
extern void align_output( unsigned int align );

/* global variables */

extern int current_line;
extern int UsePIC;
extern int nb_lib_paths;
extern int nb_errors;
extern int display_warnings;
extern int kill_at;
extern int verbose;
extern int save_temps;
extern int link_ext_symbols;
extern int force_pointer_size;

extern char *input_file_name;
extern char *spec_file_name;
extern FILE *output_file;
extern const char *output_file_name;
extern char **lib_path;

extern char *as_command;
extern char *ld_command;
extern char *nm_command;

#endif  /* __WINE_BUILD_H */
