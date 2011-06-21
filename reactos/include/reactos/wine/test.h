/*
 * Definitions for Wine C unit tests.
 *
 * Copyright (C) 2002 Alexandre Julliard
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

#ifndef __WINE_WINE_TEST_H
#define __WINE_WINE_TEST_H

#include <stdarg.h>
#include <stdlib.h>
#include <windef.h>
#include <winbase.h>

#ifdef __WINE_CONFIG_H
#error config.h should not be used in Wine tests
#endif
#ifdef __WINE_WINE_LIBRARY_H
#error wine/library.h should not be used in Wine tests
#endif
#ifdef __WINE_WINE_UNICODE_H
#error wine/unicode.h should not be used in Wine tests
#endif
#ifdef __WINE_WINE_DEBUG_H
#error wine/debug.h should not be used in Wine tests
#endif

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES  (~0u)
#endif
#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER (~0u)
#endif

/* debug level */
extern int winetest_debug;

/* running in interactive mode? */
extern int winetest_interactive;

/* current platform */
extern const char *winetest_platform;

extern void winetest_set_location( const char* file, int line );
extern void winetest_start_todo( const char* platform );
extern int winetest_loop_todo(void);
extern void winetest_end_todo( const char* platform );
extern int winetest_get_mainargs( char*** pargv );
extern void winetest_wait_child_process( HANDLE process );

extern const char *wine_dbgstr_wn( const WCHAR *str, int n );
static inline const char *wine_dbgstr_w( const WCHAR *s ) { return wine_dbgstr_wn( s, -1 ); }

/* strcmpW is avaiable for tests compiled under Wine, but not in standalone
 * builds under Windows, so we reimplement it under a different name. */
static inline int winetest_strcmpW( const WCHAR *str1, const WCHAR *str2 )
{
    while (*str1 && (*str1 == *str2)) { str1++; str2++; }
    return *str1 - *str2;
}

#ifdef STANDALONE
#define START_TEST(name) \
  static void func_##name(void); \
  const struct test winetest_testlist[] = { { #name, func_##name }, { 0, 0 } }; \
  static void func_##name(void)
#else
#define START_TEST(name) void func_##name(void)
#endif

#if defined(__x86_64__) && defined(__GNUC__) && defined(__WINE_USE_MSVCRT)
#define __winetest_cdecl __cdecl
#define __winetest_va_list __builtin_ms_va_list
#else
#define __winetest_cdecl
#define __winetest_va_list va_list
#endif

extern int broken( int condition );
extern int winetest_vok( int condition, const char *msg, __winetest_va_list ap );
extern void winetest_vskip( const char *msg, __winetest_va_list ap );

#ifdef __GNUC__

extern void __winetest_cdecl winetest_ok( int condition, const char *msg, ... ) __attribute__((format (printf,2,3) ));
extern void __winetest_cdecl winetest_skip( const char *msg, ... ) __attribute__((format (printf,1,2)));
extern void __winetest_cdecl winetest_win_skip( const char *msg, ... ) __attribute__((format (printf,1,2)));
extern void __winetest_cdecl winetest_trace( const char *msg, ... ) __attribute__((format (printf,1,2)));

#else /* __GNUC__ */

extern void __winetest_cdecl winetest_ok( int condition, const char *msg, ... );
extern void __winetest_cdecl winetest_skip( const char *msg, ... );
extern void __winetest_cdecl winetest_win_skip( const char *msg, ... );
extern void __winetest_cdecl winetest_trace( const char *msg, ... );

#endif /* __GNUC__ */

#define ok_(file, line)       (winetest_set_location(file, line), 0) ? (void)0 : winetest_ok
#define skip_(file, line)     (winetest_set_location(file, line), 0) ? (void)0 : winetest_skip
#define win_skip_(file, line) (winetest_set_location(file, line), 0) ? (void)0 : winetest_win_skip
#define trace_(file, line)    (winetest_set_location(file, line), 0) ? (void)0 : winetest_trace

#define ok       ok_(__FILE__, __LINE__)
#define skip     skip_(__FILE__, __LINE__)
#define win_skip win_skip_(__FILE__, __LINE__)
#define trace    trace_(__FILE__, __LINE__)

#define todo(platform) for (winetest_start_todo(platform); \
                            winetest_loop_todo(); \
                            winetest_end_todo(platform))
#define todo_wine      todo("wine")


#ifdef NONAMELESSUNION
# define U(x)  (x).u
# define U1(x) (x).u1
# define U2(x) (x).u2
# define U3(x) (x).u3
# define U4(x) (x).u4
# define U5(x) (x).u5
# define U6(x) (x).u6
# define U7(x) (x).u7
# define U8(x) (x).u8
#else
# define U(x)  (x)
# define U1(x) (x)
# define U2(x) (x)
# define U3(x) (x)
# define U4(x) (x)
# define U5(x) (x)
# define U6(x) (x)
# define U7(x) (x)
# define U8(x) (x)
#endif

#ifdef NONAMELESSSTRUCT
# define S(x)  (x).s
# define S1(x) (x).s1
# define S2(x) (x).s2
# define S3(x) (x).s3
# define S4(x) (x).s4
# define S5(x) (x).s5
#else
# define S(x)  (x)
# define S1(x) (x)
# define S2(x) (x)
# define S3(x) (x)
# define S4(x) (x)
# define S5(x) (x)
#endif


/************************************************************************/
/* Below is the implementation of the various functions, to be included
 * directly into the generated testlist.c file.
 * It is done that way so that the dlls can build the test routines with
 * different includes or flags if needed.
 */

#ifdef STANDALONE

#include <stdio.h>

#if defined(__x86_64__) && defined(__GNUC__) && defined(__WINE_USE_MSVCRT)
# define __winetest_va_start(list,arg) __builtin_ms_va_start(list,arg)
# define __winetest_va_end(list) __builtin_ms_va_end(list)
#else
# define __winetest_va_start(list,arg) va_start(list,arg)
# define __winetest_va_end(list) va_end(list)
#endif

struct test
{
    const char *name;
    void (*func)(void);
};

extern const struct test winetest_testlist[];

/* debug level */
int winetest_debug = 1;

/* interactive mode? */
int winetest_interactive = 0;

/* current platform */
const char *winetest_platform = "windows";

/* report successful tests (BOOL) */
static int report_success = 0;

/* passing arguments around */
static int winetest_argc;
static char** winetest_argv;

static const struct test *current_test; /* test currently being run */

static LONG successes;       /* number of successful tests */
static LONG failures;        /* number of failures */
static LONG skipped;         /* number of skipped test chunks */
static LONG todo_successes;  /* number of successful tests inside todo block */
static LONG todo_failures;   /* number of failures inside todo block */

/* The following data must be kept track of on a per-thread basis */
typedef struct
{
    const char* current_file;        /* file of current check */
    int current_line;                /* line of current check */
    int todo_level;                  /* current todo nesting level */
    int todo_do_loop;
    char *str_pos;                   /* position in debug buffer */
    char strings[2000];              /* buffer for debug strings */
} tls_data;
static DWORD tls_index;

static tls_data* get_tls_data(void)
{
    tls_data* data;
    DWORD last_error;

    last_error=GetLastError();
    data=TlsGetValue(tls_index);
    if (!data)
    {
        data=HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(tls_data));
        data->str_pos = data->strings;
        TlsSetValue(tls_index,data);
    }
    SetLastError(last_error);
    return data;
}

/* allocate some tmp space for a string */
static char *get_temp_buffer( size_t n )
{
    tls_data *data = get_tls_data();
    char *res = data->str_pos;

    if (res + n >= &data->strings[sizeof(data->strings)]) res = data->strings;
    data->str_pos = res + n;
    return res;
}

/* release extra space that we requested in gimme1() */
static void release_temp_buffer( char *ptr, size_t size )
{
    tls_data *data = get_tls_data();
    data->str_pos = ptr + size;
}

static void exit_process( int code )
{
    fflush( stdout );
    ExitProcess( code );
}


void winetest_set_location( const char* file, int line )
{
    tls_data* data=get_tls_data();
    data->current_file=strrchr(file,'/');
    if (data->current_file==NULL)
        data->current_file=strrchr(file,'\\');
    if (data->current_file==NULL)
        data->current_file=file;
    else
        data->current_file++;
    data->current_line=line;
}

int broken( int condition )
{
    return (strcmp(winetest_platform, "windows") == 0) && condition;
}

/*
 * Checks condition.
 * Parameters:
 *   - condition - condition to check;
 *   - msg test description;
 *   - file - test application source code file name of the check
 *   - line - test application source code file line number of the check
 * Return:
 *   0 if condition does not have the expected value, 1 otherwise
 */
int winetest_vok( int condition, const char *msg, __winetest_va_list args )
{
    tls_data* data=get_tls_data();

    if (data->todo_level)
    {
        if (condition)
        {
            fprintf( stdout, "%s:%d: Test succeeded inside todo block: ",
                     data->current_file, data->current_line );
            vfprintf(stdout, msg, args);
            InterlockedIncrement(&todo_failures);
            return 0;
        }
        else
        {
            if (winetest_debug > 0)
            {
                fprintf( stdout, "%s:%d: Test marked todo: ",
                         data->current_file, data->current_line );
                vfprintf(stdout, msg, args);
            }
            InterlockedIncrement(&todo_successes);
            return 1;
        }
    }
    else
    {
        if (!condition)
        {
            fprintf( stdout, "%s:%d: Test failed: ",
                     data->current_file, data->current_line );
            vfprintf(stdout, msg, args);
            InterlockedIncrement(&failures);
            return 0;
        }
        else
        {
            if (report_success)
                fprintf( stdout, "%s:%d: Test succeeded\n",
                         data->current_file, data->current_line);
            InterlockedIncrement(&successes);
            return 1;
        }
    }
}

void __winetest_cdecl winetest_ok( int condition, const char *msg, ... )
{
    __winetest_va_list valist;

    __winetest_va_start(valist, msg);
    winetest_vok(condition, msg, valist);
    __winetest_va_end(valist);
}

void __winetest_cdecl winetest_trace( const char *msg, ... )
{
    __winetest_va_list valist;
    tls_data* data=get_tls_data();

    if (winetest_debug > 0)
    {
        fprintf( stdout, "%s:%d: ", data->current_file, data->current_line );
        __winetest_va_start(valist, msg);
        vfprintf(stdout, msg, valist);
        __winetest_va_end(valist);
    }
}

void winetest_vskip( const char *msg, __winetest_va_list args )
{
    tls_data* data=get_tls_data();

    fprintf( stdout, "%s:%d: Tests skipped: ", data->current_file, data->current_line );
    vfprintf(stdout, msg, args);
    skipped++;
}

void __winetest_cdecl winetest_skip( const char *msg, ... )
{
    __winetest_va_list valist;
    __winetest_va_start(valist, msg);
    winetest_vskip(msg, valist);
    __winetest_va_end(valist);
}

void __winetest_cdecl winetest_win_skip( const char *msg, ... )
{
    __winetest_va_list valist;
    __winetest_va_start(valist, msg);
    if (strcmp(winetest_platform, "windows") == 0)
        winetest_vskip(msg, valist);
    else
        winetest_vok(0, msg, valist);
    __winetest_va_end(valist);
}

void winetest_start_todo( const char* platform )
{
    tls_data* data=get_tls_data();
    if (strcmp(winetest_platform,platform)==0)
        data->todo_level++;
    data->todo_do_loop=1;
}

int winetest_loop_todo(void)
{
    tls_data* data=get_tls_data();
    int do_loop=data->todo_do_loop;
    data->todo_do_loop=0;
    return do_loop;
}

void winetest_end_todo( const char* platform )
{
    if (strcmp(winetest_platform,platform)==0)
    {
        tls_data* data=get_tls_data();
        data->todo_level--;
    }
}

int winetest_get_mainargs( char*** pargv )
{
    *pargv = winetest_argv;
    return winetest_argc;
}

void winetest_wait_child_process( HANDLE process )
{
    DWORD exit_code = 1;

    if (WaitForSingleObject( process, 30000 ))
        fprintf( stdout, "%s: child process wait failed\n", current_test->name );
    else
        GetExitCodeProcess( process, &exit_code );

    if (exit_code)
    {
        if (exit_code > 255)
        {
            fprintf( stdout, "%s: exception 0x%08x in child process\n", current_test->name, exit_code );
            InterlockedIncrement( &failures );
        }
        else
        {
            fprintf( stdout, "%s: %u failures in child process\n",
                     current_test->name, exit_code );
            while (exit_code-- > 0)
                InterlockedIncrement(&failures);
        }
    }
}

const char *wine_dbgstr_wn( const WCHAR *str, int n )
{
    char *dst, *res;
    size_t size;

    if (!((ULONG_PTR)str >> 16))
    {
        if (!str) return "(null)";
        res = get_temp_buffer( 6 );
        sprintf( res, "#%04x", LOWORD(str) );
        return res;
    }
    if (n == -1)
    {
        const WCHAR *end = str;
        while (*end) end++;
        n = end - str;
    }
    if (n < 0) n = 0;
    size = 12 + min( 300, n * 5 );
    dst = res = get_temp_buffer( size );
    *dst++ = 'L';
    *dst++ = '"';
    while (n-- > 0 && dst <= res + size - 10)
    {
        WCHAR c = *str++;
        switch (c)
        {
        case '\n': *dst++ = '\\'; *dst++ = 'n'; break;
        case '\r': *dst++ = '\\'; *dst++ = 'r'; break;
        case '\t': *dst++ = '\\'; *dst++ = 't'; break;
        case '"':  *dst++ = '\\'; *dst++ = '"'; break;
        case '\\': *dst++ = '\\'; *dst++ = '\\'; break;
        default:
            if (c >= ' ' && c <= 126)
                *dst++ = (char)c;
            else
            {
                *dst++ = '\\';
                sprintf(dst,"%04x",c);
                dst+=4;
            }
        }
    }
    *dst++ = '"';
    if (n > 0)
    {
        *dst++ = '.';
        *dst++ = '.';
        *dst++ = '.';
    }
    *dst++ = 0;
    release_temp_buffer( res, dst - res );
    return res;
}

/* Find a test by name */
static const struct test *find_test( const char *name )
{
    const struct test *test;
    const char *p;
    size_t len;

    if ((p = strrchr( name, '/' ))) name = p + 1;
    if ((p = strrchr( name, '\\' ))) name = p + 1;
    len = strlen(name);
    if (len > 2 && !strcmp( name + len - 2, ".c" )) len -= 2;

    for (test = winetest_testlist; test->name; test++)
    {
        if (!strncmp( test->name, name, len ) && !test->name[len]) break;
    }
    return test->name ? test : NULL;
}


/* Display list of valid tests */
static void list_tests(void)
{
    const struct test *test;

    fprintf( stdout, "Valid test names:\n" );
    for (test = winetest_testlist; test->name; test++) fprintf( stdout, "    %s\n", test->name );
}


/* Run a named test, and return exit status */
static int run_test( const char *name )
{
    const struct test *test;
    int status;

    if (!(test = find_test( name )))
    {
        fprintf( stdout, "Fatal: test '%s' does not exist.\n", name );
        exit_process(1);
    }
    successes = failures = todo_successes = todo_failures = 0;
    tls_index=TlsAlloc();
    current_test = test;
    test->func();

    if (winetest_debug)
    {
        fprintf( stdout, "%s: %d tests executed (%d marked as todo, %d %s), %d skipped.\n",
                 test->name, successes + failures + todo_successes + todo_failures,
                 todo_successes, failures + todo_failures,
                 (failures + todo_failures != 1) ? "failures" : "failure",
                 skipped );
    }
    status = (failures + todo_failures < 255) ? failures + todo_failures : 255;
    return status;
}


/* Display usage and exit */
static void usage( const char *argv0 )
{
    fprintf( stdout, "Usage: %s test_name\n\n", argv0 );
    list_tests();
    exit_process(1);
}


/* main function */
int main( int argc, char **argv )
{
    char p[128];

    setvbuf (stdout, NULL, _IONBF, 0);

    winetest_argc = argc;
    winetest_argv = argv;

    if (GetEnvironmentVariableA( "WINETEST_PLATFORM", p, sizeof(p) )) winetest_platform = _strdup(p);
    if (GetEnvironmentVariableA( "WINETEST_DEBUG", p, sizeof(p) )) winetest_debug = atoi(p);
    if (GetEnvironmentVariableA( "WINETEST_INTERACTIVE", p, sizeof(p) )) winetest_interactive = atoi(p);
    if (GetEnvironmentVariableA( "WINETEST_REPORT_SUCCESS", p, sizeof(p) )) report_success = atoi(p);

    if (!winetest_interactive) SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX );

    if (!argv[1])
    {
        if (winetest_testlist[0].name && !winetest_testlist[1].name)  /* only one test */
            return run_test( winetest_testlist[0].name );
        usage( argv[0] );
    }
    if (!strcmp( argv[1], "--list" ))
    {
        list_tests();
        return 0;
    }
    return run_test(argv[1]);
}

#endif  /* STANDALONE */

// hack for ntdll winetest (this is defined in excpt.h)
#undef exception_info

#endif  /* __WINE_WINE_TEST_H */
