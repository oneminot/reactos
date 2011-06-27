/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Driver test list
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <ntddk.h>
#include <kmt_test.h>

KMT_TESTFUNC Test_Example;
KMT_TESTFUNC Test_ExPools;
KMT_TESTFUNC Test_ExTimer;
KMT_TESTFUNC Test_FsRtlExpression;
KMT_TESTFUNC Test_IoDeviceInterface;
KMT_TESTFUNC Test_IoIrp;
KMT_TESTFUNC Test_IoMdl;
KMT_TESTFUNC Test_KeProcessor;
KMT_TESTFUNC Test_ObCreate;

const KMT_TEST TestList[] =
{
    { "Example",            Test_Example },
    { "ExPools",            Test_ExPools },
    { "ExTimer",            Test_ExTimer },
    { "FsRtlExpression",    Test_FsRtlExpression },
    { "IoDeviceInterface",  Test_IoDeviceInterface },
    { "IoIrp",              Test_IoIrp },
    { "IoMdl",              Test_IoMdl },
    { "KeProcessor",        Test_KeProcessor },
    { "ObCreate",           Test_ObCreate },
    { NULL,                 NULL }
};
