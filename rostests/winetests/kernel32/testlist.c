/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include "wine/test.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include <windows.h>

extern void func_actctx(void);
extern void func_alloc(void);
extern void func_atom(void);
extern void func_change(void);
extern void func_codepage(void);
extern void func_comm(void);
extern void func_console(void);
extern void func_directory(void);
extern void func_drive(void);
extern void func_environ(void);
extern void func_fiber(void);
extern void func_file(void);
extern void func_format_msg(void);
extern void func_heap(void);
extern void func_interlck(void);
extern void func_locale(void);
extern void func_loader(void);
extern void func_module(void);
extern void func_mailslot(void);
extern void func_path(void);
extern void func_pipe(void);
extern void func_process(void);
extern void func_profile(void);
extern void func_resource(void);
extern void func_sync(void);
extern void func_thread(void);
extern void func_time(void);
extern void func_timer(void);
extern void func_toolhelp(void);
extern void func_virtual(void);
extern void func_version(void);
extern void func_volume(void);
extern void func_dosdev(void);

const struct test winetest_testlist[] =
{
    { "actctx", func_actctx },
    { "alloc", func_alloc },
    { "atom", func_atom },
    { "change", func_change },
    { "codepage", func_codepage },
    { "comm", func_comm },
    { "console", func_console },
    //{ "debbuger", func_debbuger },
    { "directory", func_directory },
    { "drive", func_drive },
    { "environ", func_environ },
	{ "fiber", func_fiber },
    { "file", func_file },
    { "format_msg", func_format_msg },
    { "heap", func_heap },
    { "interlck", func_interlck },
    { "loader", func_loader },
    { "locale", func_locale },
    { "module", func_module },
    { "mailslot", func_mailslot },
    { "path", func_path },
    { "pipe", func_pipe },
    { "process", func_process },
    { "profile", func_profile },
    { "resource", func_resource },
    { "sync", func_sync },
    { "thread", func_thread },
    { "time", func_time },
    { "timer", func_timer },
    { "toolhelp", func_toolhelp },
    { "virtual", func_virtual },
    { "version", func_version },
    { "volume", func_volume },
    { "dosdev", func_dosdev },
    { 0, 0 }
};


