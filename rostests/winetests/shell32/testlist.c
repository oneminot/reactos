/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_generated(void);
extern void func_shelllink(void);
extern void func_shellpath(void);
extern void func_shfldr_netplaces(void);
extern void func_shlexec(void);
extern void func_shlfileop(void);
extern void func_shlfolder(void);
extern void func_string(void);
extern void func_systray(void);

const struct test winetest_testlist[] =
{
    { "generated", func_generated },
    { "shelllink", func_shelllink },
    { "shellpath", func_shellpath },
    { "shfldr_netplaces", func_shfldr_netplaces },
    { "shlexec", func_shlexec },
    { "shlfileop", func_shlfileop },
    { "shlfolder", func_shlfolder },
    { "string", func_string },
    { "systray", func_systray },
    { 0, 0 }
};
