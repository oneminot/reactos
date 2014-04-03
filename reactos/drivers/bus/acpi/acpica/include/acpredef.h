/******************************************************************************
 *
 * Name: acpredef - Information table for ACPI predefined methods and objects
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2014, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights. You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code. No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision. In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change. Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee. Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution. In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE. ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT, ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES. INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS. INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES. THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government. In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

#ifndef __ACPREDEF_H__
#define __ACPREDEF_H__


/******************************************************************************
 *
 * Return Package types
 *
 * 1) PTYPE1 packages do not contain subpackages.
 *
 * ACPI_PTYPE1_FIXED: Fixed-length length, 1 or 2 object types:
 *      object type
 *      count
 *      object type
 *      count
 *
 * ACPI_PTYPE1_VAR: Variable-length length. Zero-length package is allowed:
 *      object type (Int/Buf/Ref)
 *
 * ACPI_PTYPE1_OPTION: Package has some required and some optional elements
 *      (Used for _PRW)
 *
 *
 * 2) PTYPE2 packages contain a Variable-length number of subpackages. Each
 *    of the different types describe the contents of each of the subpackages.
 *
 * ACPI_PTYPE2: Each subpackage contains 1 or 2 object types. Zero-length
 *      parent package is allowed:
 *      object type
 *      count
 *      object type
 *      count
 *      (Used for _ALR,_MLS,_PSS,_TRT,_TSS)
 *
 * ACPI_PTYPE2_COUNT: Each subpackage has a count as first element.
 *      Zero-length parent package is allowed:
 *      object type
 *      (Used for _CSD,_PSD,_TSD)
 *
 * ACPI_PTYPE2_PKG_COUNT: Count of subpackages at start, 1 or 2 object types:
 *      object type
 *      count
 *      object type
 *      count
 *      (Used for _CST)
 *
 * ACPI_PTYPE2_FIXED: Each subpackage is of Fixed-length. Zero-length
 *      parent package is allowed.
 *      (Used for _PRT)
 *
 * ACPI_PTYPE2_MIN: Each subpackage has a Variable-length but minimum length.
 *      Zero-length parent package is allowed:
 *      (Used for _HPX)
 *
 * ACPI_PTYPE2_REV_FIXED: Revision at start, each subpackage is Fixed-length
 *      (Used for _ART, _FPS)
 *
 * ACPI_PTYPE2_FIX_VAR: Each subpackage consists of some fixed-length elements
 *      followed by an optional element. Zero-length parent package is allowed.
 *      object type
 *      count
 *      object type
 *      count = 0 (optional)
 *      (Used for _DLM)
 *
 *****************************************************************************/

enum AcpiReturnPackageTypes
{
    ACPI_PTYPE1_FIXED       = 1,
    ACPI_PTYPE1_VAR         = 2,
    ACPI_PTYPE1_OPTION      = 3,
    ACPI_PTYPE2             = 4,
    ACPI_PTYPE2_COUNT       = 5,
    ACPI_PTYPE2_PKG_COUNT   = 6,
    ACPI_PTYPE2_FIXED       = 7,
    ACPI_PTYPE2_MIN         = 8,
    ACPI_PTYPE2_REV_FIXED   = 9,
    ACPI_PTYPE2_FIX_VAR     = 10
};


/* Support macros for users of the predefined info table */

#define METHOD_PREDEF_ARGS_MAX          4
#define METHOD_ARG_BIT_WIDTH            3
#define METHOD_ARG_MASK                 0x0007
#define ARG_COUNT_IS_MINIMUM            0x8000
#define METHOD_MAX_ARG_TYPE             ACPI_TYPE_PACKAGE

#define METHOD_GET_ARG_COUNT(ArgList)   ((ArgList) & METHOD_ARG_MASK)
#define METHOD_GET_NEXT_TYPE(ArgList)   (((ArgList) >>= METHOD_ARG_BIT_WIDTH) & METHOD_ARG_MASK)

/* Macros used to build the predefined info table */

#define METHOD_0ARGS                    0
#define METHOD_1ARGS(a1)                (1 | (a1 << 3))
#define METHOD_2ARGS(a1,a2)             (2 | (a1 << 3) | (a2 << 6))
#define METHOD_3ARGS(a1,a2,a3)          (3 | (a1 << 3) | (a2 << 6) | (a3 << 9))
#define METHOD_4ARGS(a1,a2,a3,a4)       (4 | (a1 << 3) | (a2 << 6) | (a3 << 9) | (a4 << 12))

#define METHOD_RETURNS(type)            (type)
#define METHOD_NO_RETURN_VALUE          0

#define PACKAGE_INFO(a,b,c,d,e,f)       {{{(a),(b),(c),(d)}, ((((UINT16)(f)) << 8) | (e)), 0}}


/* Support macros for the resource descriptor info table */

#define WIDTH_1                         0x0001
#define WIDTH_2                         0x0002
#define WIDTH_3                         0x0004
#define WIDTH_8                         0x0008
#define WIDTH_16                        0x0010
#define WIDTH_32                        0x0020
#define WIDTH_64                        0x0040
#define VARIABLE_DATA                   0x0080
#define NUM_RESOURCE_WIDTHS             8

#define WIDTH_ADDRESS                   WIDTH_16 | WIDTH_32 | WIDTH_64


#ifdef ACPI_CREATE_PREDEFINED_TABLE
/******************************************************************************
 *
 * Predefined method/object information table.
 *
 * These are the names that can actually be evaluated via AcpiEvaluateObject.
 * Not present in this table are the following:
 *
 *      1) Predefined/Reserved names that are never evaluated via
 *         AcpiEvaluateObject:
 *              _Lxx and _Exx GPE methods
 *              _Qxx EC methods
 *              _T_x compiler temporary variables
 *              _Wxx wake events
 *
 *      2) Predefined names that never actually exist within the AML code:
 *              Predefined resource descriptor field names
 *
 *      3) Predefined names that are implemented within ACPICA:
 *              _OSI
 *
 * The main entries in the table each contain the following items:
 *
 * Name                 - The ACPI reserved name
 * ArgumentList         - Contains (in 16 bits), the number of required
 *                        arguments to the method (3 bits), and a 3-bit type
 *                        field for each argument (up to 4 arguments). The
 *                        METHOD_?ARGS macros generate the correct packed data.
 * ExpectedBtypes       - Allowed type(s) for the return value.
 *                        0 means that no return value is expected.
 *
 * For methods that return packages, the next entry in the table contains
 * information about the expected structure of the package. This information
 * is saved here (rather than in a separate table) in order to minimize the
 * overall size of the stored data.
 *
 * Note: The additional braces are intended to promote portability.
 *
 * Note2: Table is used by the kernel-resident subsystem, the iASL compiler,
 * and the AcpiHelp utility.
 *
 * TBD: _PRT - currently ignore reversed entries. Attempt to fix in nsrepair.
 * Possibly fixing package elements like _BIF, etc.
 *
 *****************************************************************************/

const ACPI_PREDEFINED_INFO          AcpiGbl_PredefinedMethods[] =
{
    {{"_AC0",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_AC1",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_AC2",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_AC3",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_AC4",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_AC5",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_AC6",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_AC7",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_AC8",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_AC9",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_ADR",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_AEI",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_BUFFER)}},

    {{"_AL0",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Refs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_REFERENCE, 0,0,0,0),

    {{"_AL1",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Refs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_REFERENCE, 0,0,0,0),

    {{"_AL2",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Refs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_REFERENCE, 0,0,0,0),

    {{"_AL3",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Refs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_REFERENCE, 0,0,0,0),

    {{"_AL4",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Refs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_REFERENCE, 0,0,0,0),

    {{"_AL5",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Refs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_REFERENCE, 0,0,0,0),

    {{"_AL6",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Refs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_REFERENCE, 0,0,0,0),

    {{"_AL7",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Refs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_REFERENCE, 0,0,0,0),

    {{"_AL8",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Refs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_REFERENCE, 0,0,0,0),

    {{"_AL9",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Refs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_REFERENCE, 0,0,0,0),

    {{"_ALC",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_ALI",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_ALP",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_ALR",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Pkgs) each 2 (Ints) */
                    PACKAGE_INFO (ACPI_PTYPE2, ACPI_RTYPE_INTEGER, 2,0,0,0),

    {{"_ALT",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_ART",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (1 Int(rev), n Pkg (2 Ref/11 Int) */
                    PACKAGE_INFO (ACPI_PTYPE2_REV_FIXED, ACPI_RTYPE_REFERENCE, 2, ACPI_RTYPE_INTEGER, 11,0),

    {{"_BBN",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_BCL",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Ints) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_INTEGER, 0,0,0,0),

    {{"_BCM",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_BCT",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_BDN",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_BFS",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_BIF",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Fixed-length (9 Int),(4 Str) */
                    PACKAGE_INFO (ACPI_PTYPE1_FIXED, ACPI_RTYPE_INTEGER, 9, ACPI_RTYPE_STRING, 4,0),

    {{"_BIX",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Fixed-length (16 Int),(4 Str) */
                    PACKAGE_INFO (ACPI_PTYPE1_FIXED, ACPI_RTYPE_INTEGER, 16, ACPI_RTYPE_STRING, 4,0),

    {{"_BLT",   METHOD_3ARGS (ACPI_TYPE_INTEGER, ACPI_TYPE_INTEGER, ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_BMA",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_BMC",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_BMD",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Fixed-length (5 Int) */
                    PACKAGE_INFO (ACPI_PTYPE1_FIXED, ACPI_RTYPE_INTEGER, 5,0,0,0),

    {{"_BMS",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_BQC",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_BST",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Fixed-length (4 Int) */
                    PACKAGE_INFO (ACPI_PTYPE1_FIXED, ACPI_RTYPE_INTEGER, 4,0,0,0),

    {{"_BTM",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_BTP",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_CBA",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}}, /* See PCI firmware spec 3.0 */

    {{"_CDM",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_CID",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER | ACPI_RTYPE_STRING | ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Ints/Strs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_INTEGER | ACPI_RTYPE_STRING, 0,0,0,0),

    {{"_CLS",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Fixed-length (3 Int) */
                    PACKAGE_INFO (ACPI_PTYPE1_FIXED, ACPI_RTYPE_INTEGER, 3,0,0,0),

    {{"_CPC",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Ints/Bufs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_INTEGER | ACPI_RTYPE_BUFFER, 0,0,0,0),

    {{"_CRS",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_BUFFER)}},

    {{"_CRT",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_CSD",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (1 Int(n), n-1 Int) */
                    PACKAGE_INFO (ACPI_PTYPE2_COUNT, ACPI_RTYPE_INTEGER, 0,0,0,0),

    {{"_CST",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (1 Int(n), n Pkg (1 Buf/3 Int) */
                    PACKAGE_INFO (ACPI_PTYPE2_PKG_COUNT,ACPI_RTYPE_BUFFER, 1, ACPI_RTYPE_INTEGER, 3,0),

    {{"_CWS",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_DCK",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_DCS",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_DDC",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_RETURNS (ACPI_RTYPE_INTEGER | ACPI_RTYPE_BUFFER)}},

    {{"_DDN",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_STRING)}},

    {{"_DEP",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Refs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_REFERENCE, 0,0,0,0),

    {{"_DGS",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_DIS",   METHOD_0ARGS,
                METHOD_NO_RETURN_VALUE}},

    {{"_DLM",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Pkgs) each (1 Ref, 0/1 Optional Buf/Ref) */
                    PACKAGE_INFO (ACPI_PTYPE2_FIX_VAR, ACPI_RTYPE_REFERENCE, 1, ACPI_RTYPE_REFERENCE | ACPI_RTYPE_BUFFER, 0,0),

    {{"_DMA",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_BUFFER)}},

    {{"_DOD",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Ints) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_INTEGER, 0,0,0,0),

    {{"_DOS",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_DSM",   METHOD_4ARGS (ACPI_TYPE_BUFFER, ACPI_TYPE_INTEGER, ACPI_TYPE_INTEGER, ACPI_TYPE_PACKAGE),
                METHOD_RETURNS (ACPI_RTYPE_ALL)}}, /* Must return a value, but it can be of any type */

    {{"_DSS",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_DSW",   METHOD_3ARGS (ACPI_TYPE_INTEGER, ACPI_TYPE_INTEGER, ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_DTI",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_EC_",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_EDL",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Refs)*/
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_REFERENCE, 0,0,0,0),

    {{"_EJ0",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_EJ1",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_EJ2",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_EJ3",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_EJ4",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_EJD",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_STRING)}},

    {{"_ERR",   METHOD_3ARGS (ACPI_TYPE_INTEGER, ACPI_TYPE_STRING, ACPI_TYPE_INTEGER),
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}}, /* Internal use only, used by ACPICA test suites */

    {{"_EVT",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_FDE",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_BUFFER)}},

    {{"_FDI",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Fixed-length (16 Int) */
                    PACKAGE_INFO (ACPI_PTYPE1_FIXED, ACPI_RTYPE_INTEGER, 16,0,0,0),

    {{"_FDM",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_FIF",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Fixed-length (4 Int) */
                    PACKAGE_INFO (ACPI_PTYPE1_FIXED, ACPI_RTYPE_INTEGER, 4,0,0,0),

    {{"_FIX",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Ints) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_INTEGER, 0,0,0,0),

    {{"_FPS",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (1 Int(rev), n Pkg (5 Int) */
                    PACKAGE_INFO (ACPI_PTYPE2_REV_FIXED,ACPI_RTYPE_INTEGER, 5, 0,0,0),

    {{"_FSL",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_FST",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Fixed-length (3 Int) */
                    PACKAGE_INFO (ACPI_PTYPE1_FIXED, ACPI_RTYPE_INTEGER, 3,0,0,0),

    {{"_GAI",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_GCP",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_GHL",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_GLK",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_GPD",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_GPE",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}}, /* _GPE method, not _GPE scope */

    {{"_GRT",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_BUFFER)}},

    {{"_GSB",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_GTF",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_BUFFER)}},

    {{"_GTM",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_BUFFER)}},

    {{"_GTS",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_GWS",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_HID",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER | ACPI_RTYPE_STRING)}},

    {{"_HOT",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_HPP",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Fixed-length (4 Int) */
                    PACKAGE_INFO (ACPI_PTYPE1_FIXED, ACPI_RTYPE_INTEGER, 4,0,0,0),

    /*
     * For _HPX, a single package is returned, containing a variable-length number
     * of subpackages. Each subpackage contains a PCI record setting.
     * There are several different type of record settings, of different
     * lengths, but all elements of all settings are Integers.
     */
    {{"_HPX",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Pkgs) each (var Ints) */
                    PACKAGE_INFO (ACPI_PTYPE2_MIN, ACPI_RTYPE_INTEGER, 5,0,0,0),

    {{"_HRV",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_IFT",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}}, /* See IPMI spec */

    {{"_INI",   METHOD_0ARGS,
                METHOD_NO_RETURN_VALUE}},

    {{"_IRC",   METHOD_0ARGS,
                METHOD_NO_RETURN_VALUE}},

    {{"_LCK",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_LID",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_MAT",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_BUFFER)}},

    {{"_MBM",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Fixed-length (8 Int) */
                    PACKAGE_INFO (ACPI_PTYPE1_FIXED, ACPI_RTYPE_INTEGER, 8,0,0,0),

    {{"_MLS",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Pkgs) each (1 Str/1 Buf) */
                    PACKAGE_INFO (ACPI_PTYPE2, ACPI_RTYPE_STRING, 1, ACPI_RTYPE_BUFFER, 1,0),

    {{"_MSG",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_MSM",   METHOD_4ARGS (ACPI_TYPE_INTEGER, ACPI_TYPE_INTEGER, ACPI_TYPE_INTEGER, ACPI_TYPE_INTEGER),
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_NTT",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_OFF",   METHOD_0ARGS,
                METHOD_NO_RETURN_VALUE}},

    {{"_ON_",   METHOD_0ARGS,
                METHOD_NO_RETURN_VALUE}},

    {{"_OS_",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_STRING)}},

    {{"_OSC",   METHOD_4ARGS (ACPI_TYPE_BUFFER, ACPI_TYPE_INTEGER, ACPI_TYPE_INTEGER, ACPI_TYPE_BUFFER),
                METHOD_RETURNS (ACPI_RTYPE_BUFFER)}},

    {{"_OST",   METHOD_3ARGS (ACPI_TYPE_INTEGER, ACPI_TYPE_INTEGER, ACPI_TYPE_BUFFER),
                METHOD_NO_RETURN_VALUE}},

    {{"_PAI",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_PCL",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Refs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_REFERENCE, 0,0,0,0),

    {{"_PCT",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Fixed-length (2 Buf) */
                    PACKAGE_INFO (ACPI_PTYPE1_FIXED, ACPI_RTYPE_BUFFER, 2,0,0,0),

    {{"_PDC",   METHOD_1ARGS (ACPI_TYPE_BUFFER),
                METHOD_NO_RETURN_VALUE}},

    {{"_PDL",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_PIC",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_PIF",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Fixed-length (3 Int),(3 Str) */
                    PACKAGE_INFO (ACPI_PTYPE1_FIXED, ACPI_RTYPE_INTEGER, 3, ACPI_RTYPE_STRING, 3,0),

    {{"_PLD",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Bufs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_BUFFER, 0,0,0,0),

    {{"_PMC",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Fixed-length (11 Int),(3 Str) */
                    PACKAGE_INFO (ACPI_PTYPE1_FIXED, ACPI_RTYPE_INTEGER, 11, ACPI_RTYPE_STRING, 3,0),

    {{"_PMD",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Refs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_REFERENCE, 0,0,0,0),

    {{"_PMM",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_PPC",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_PPE",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}}, /* See dig64 spec */

    {{"_PR0",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Refs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_REFERENCE, 0,0,0,0),

    {{"_PR1",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Refs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_REFERENCE, 0,0,0,0),

    {{"_PR2",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Refs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_REFERENCE, 0,0,0,0),

    {{"_PR3",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Refs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_REFERENCE, 0,0,0,0),

    {{"_PRE",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Refs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_REFERENCE, 0,0,0,0),

    {{"_PRL",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Refs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_REFERENCE, 0,0,0,0),

    {{"_PRP",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Pkgs) each: 1 Str, 1 Int/Str/Pkg */
                    PACKAGE_INFO (ACPI_PTYPE2, ACPI_RTYPE_STRING, 1,
                        ACPI_RTYPE_INTEGER | ACPI_RTYPE_STRING |
                        ACPI_RTYPE_PACKAGE | ACPI_RTYPE_REFERENCE, 1,0),

    {{"_PRS",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_BUFFER)}},

    /*
     * For _PRT, many BIOSs reverse the 3rd and 4th Package elements (Source
     * and SourceIndex). This bug is so prevalent that there is code in the
     * ACPICA Resource Manager to detect this and switch them back. For now,
     * do not allow and issue a warning. To allow this and eliminate the
     * warning, add the ACPI_RTYPE_REFERENCE type to the 4th element (index 3)
     * in the statement below.
     */
    {{"_PRT",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Pkgs) each (4): Int,Int,Int/Ref,Int */
                    PACKAGE_INFO (ACPI_PTYPE2_FIXED, 4, ACPI_RTYPE_INTEGER, ACPI_RTYPE_INTEGER,
                                  ACPI_RTYPE_INTEGER | ACPI_RTYPE_REFERENCE, ACPI_RTYPE_INTEGER),

    {{"_PRW",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Pkgs) each: Pkg/Int,Int,[Variable-length Refs] (Pkg is Ref/Int) */
                    PACKAGE_INFO (ACPI_PTYPE1_OPTION, 2, ACPI_RTYPE_INTEGER | ACPI_RTYPE_PACKAGE,
                                  ACPI_RTYPE_INTEGER, ACPI_RTYPE_REFERENCE, 0),

    {{"_PS0",   METHOD_0ARGS,
                METHOD_NO_RETURN_VALUE}},

    {{"_PS1",   METHOD_0ARGS,
                METHOD_NO_RETURN_VALUE}},

    {{"_PS2",   METHOD_0ARGS,
                METHOD_NO_RETURN_VALUE}},

    {{"_PS3",   METHOD_0ARGS,
                METHOD_NO_RETURN_VALUE}},

    {{"_PSC",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_PSD",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Pkgs) each (5 Int) with count */
                    PACKAGE_INFO (ACPI_PTYPE2_COUNT, ACPI_RTYPE_INTEGER, 0,0,0,0),

    {{"_PSE",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_PSL",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Refs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_REFERENCE, 0,0,0,0),

    {{"_PSR",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_PSS",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Pkgs) each (6 Int) */
                    PACKAGE_INFO (ACPI_PTYPE2, ACPI_RTYPE_INTEGER, 6,0,0,0),

    {{"_PSV",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_PSW",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_PTC",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Fixed-length (2 Buf) */
                    PACKAGE_INFO (ACPI_PTYPE1_FIXED, ACPI_RTYPE_BUFFER, 2,0,0,0),

    {{"_PTP",   METHOD_2ARGS (ACPI_TYPE_INTEGER, ACPI_TYPE_INTEGER),
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_PTS",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_PUR",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Fixed-length (2 Int) */
                    PACKAGE_INFO (ACPI_PTYPE1_FIXED, ACPI_RTYPE_INTEGER, 2,0,0,0),

    {{"_PXM",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_REG",   METHOD_2ARGS (ACPI_TYPE_INTEGER, ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_REV",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_RMV",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_ROM",   METHOD_2ARGS (ACPI_TYPE_INTEGER, ACPI_TYPE_INTEGER),
                METHOD_RETURNS (ACPI_RTYPE_BUFFER)}},

    {{"_RTV",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    /*
     * For _S0_ through _S5_, the ACPI spec defines a return Package
     * containing 1 Integer, but most DSDTs have it wrong - 2,3, or 4 integers.
     * Allow this by making the objects "Variable-length length", but all elements
     * must be Integers.
     */
    {{"_S0_",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Fixed-length (1 Int) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_INTEGER, 1,0,0,0),

    {{"_S1_",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Fixed-length (1 Int) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_INTEGER, 1,0,0,0),

    {{"_S2_",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Fixed-length (1 Int) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_INTEGER, 1,0,0,0),

    {{"_S3_",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Fixed-length (1 Int) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_INTEGER, 1,0,0,0),

    {{"_S4_",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Fixed-length (1 Int) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_INTEGER, 1,0,0,0),

    {{"_S5_",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Fixed-length (1 Int) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_INTEGER, 1,0,0,0),

    {{"_S1D",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_S2D",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_S3D",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_S4D",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_S0W",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_S1W",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_S2W",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_S3W",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_S4W",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_SBS",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_SCP",   METHOD_1ARGS (ACPI_TYPE_INTEGER) | ARG_COUNT_IS_MINIMUM,
                METHOD_NO_RETURN_VALUE}},   /* Acpi 1.0 allowed 1 integer arg. Acpi 3.0 expanded to 3 args. Allow both. */

    {{"_SDD",   METHOD_1ARGS (ACPI_TYPE_BUFFER),
                METHOD_NO_RETURN_VALUE}},

    {{"_SEG",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_SHL",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_SLI",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_BUFFER)}},

    {{"_SPD",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_SRS",   METHOD_1ARGS (ACPI_TYPE_BUFFER),
                METHOD_NO_RETURN_VALUE}},

    {{"_SRT",   METHOD_1ARGS (ACPI_TYPE_BUFFER),
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_SRV",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}}, /* See IPMI spec */

    {{"_SST",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_STA",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_STM",   METHOD_3ARGS (ACPI_TYPE_BUFFER, ACPI_TYPE_BUFFER, ACPI_TYPE_BUFFER),
                METHOD_NO_RETURN_VALUE}},

    {{"_STP",   METHOD_2ARGS (ACPI_TYPE_INTEGER, ACPI_TYPE_INTEGER),
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_STR",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_BUFFER)}},

    {{"_STV",   METHOD_2ARGS (ACPI_TYPE_INTEGER, ACPI_TYPE_INTEGER),
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_SUB",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_STRING)}},

    {{"_SUN",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_SWS",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_TC1",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_TC2",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_TDL",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_TIP",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_TIV",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_TMP",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_TPC",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_TPT",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_TRT",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Pkgs) each 2 Ref/6 Int */
                    PACKAGE_INFO (ACPI_PTYPE2, ACPI_RTYPE_REFERENCE, 2, ACPI_RTYPE_INTEGER, 6, 0),

    {{"_TSD",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Pkgs) each 5 Int with count */
                    PACKAGE_INFO (ACPI_PTYPE2_COUNT,ACPI_RTYPE_INTEGER, 5,0,0,0),

    {{"_TSP",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_TSS",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Pkgs) each 5 Int */
                    PACKAGE_INFO (ACPI_PTYPE2, ACPI_RTYPE_INTEGER, 5,0,0,0),

    {{"_TST",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_TTS",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_NO_RETURN_VALUE}},

    {{"_TZD",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Variable-length (Refs) */
                    PACKAGE_INFO (ACPI_PTYPE1_VAR, ACPI_RTYPE_REFERENCE, 0,0,0,0),

    {{"_TZM",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_REFERENCE)}},

    {{"_TZP",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_UID",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER | ACPI_RTYPE_STRING)}},

    {{"_UPC",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_PACKAGE)}}, /* Fixed-length (4 Int) */
                    PACKAGE_INFO (ACPI_PTYPE1_FIXED, ACPI_RTYPE_INTEGER, 4,0,0,0),

    {{"_UPD",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_UPP",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    {{"_VPO",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_INTEGER)}},

    /* Acpi 1.0 defined _WAK with no return value. Later, it was changed to return a package */

    {{"_WAK",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_RETURNS (ACPI_RTYPE_NONE | ACPI_RTYPE_INTEGER | ACPI_RTYPE_PACKAGE)}},
                    PACKAGE_INFO (ACPI_PTYPE1_FIXED, ACPI_RTYPE_INTEGER, 2,0,0,0), /* Fixed-length (2 Int), but is optional */

    /* _WDG/_WED are MS extensions defined by "Windows Instrumentation" */

    {{"_WDG",   METHOD_0ARGS,
                METHOD_RETURNS (ACPI_RTYPE_BUFFER)}},

    {{"_WED",   METHOD_1ARGS (ACPI_TYPE_INTEGER),
                METHOD_RETURNS (ACPI_RTYPE_INTEGER | ACPI_RTYPE_STRING | ACPI_RTYPE_BUFFER)}},

    PACKAGE_INFO (0,0,0,0,0,0) /* Table terminator */
};
#else
extern const ACPI_PREDEFINED_INFO       AcpiGbl_PredefinedMethods[];
#endif


#if (defined ACPI_CREATE_RESOURCE_TABLE && defined ACPI_APPLICATION)
/******************************************************************************
 *
 * Predefined names for use in Resource Descriptors. These names do not
 * appear in the global Predefined Name table (since these names never
 * appear in actual AML byte code, only in the original ASL)
 *
 * Note: Used by iASL compiler and AcpiHelp utility only.
 *
 *****************************************************************************/

const ACPI_PREDEFINED_INFO      AcpiGbl_ResourceNames[] =
{
    {{"_ADR",   WIDTH_16 | WIDTH_64,            0}},
    {{"_ALN",   WIDTH_8 | WIDTH_16 | WIDTH_32,  0}},
    {{"_ASI",   WIDTH_8,                        0}},
    {{"_ASZ",   WIDTH_8,                        0}},
    {{"_ATT",   WIDTH_64,                       0}},
    {{"_BAS",   WIDTH_16 | WIDTH_32,            0}},
    {{"_BM_",   WIDTH_1,                        0}},
    {{"_DBT",   WIDTH_16,                       0}},  /* Acpi 5.0 */
    {{"_DEC",   WIDTH_1,                        0}},
    {{"_DMA",   WIDTH_8,                        0}},
    {{"_DPL",   WIDTH_1,                        0}},  /* Acpi 5.0 */
    {{"_DRS",   WIDTH_16,                       0}},  /* Acpi 5.0 */
    {{"_END",   WIDTH_1,                        0}},  /* Acpi 5.0 */
    {{"_FLC",   WIDTH_2,                        0}},  /* Acpi 5.0 */
    {{"_GRA",   WIDTH_ADDRESS,                  0}},
    {{"_HE_",   WIDTH_1,                        0}},
    {{"_INT",   WIDTH_16 | WIDTH_32,            0}},
    {{"_IOR",   WIDTH_2,                        0}},  /* Acpi 5.0 */
    {{"_LEN",   WIDTH_8 | WIDTH_ADDRESS,        0}},
    {{"_LIN",   WIDTH_8,                        0}},  /* Acpi 5.0 */
    {{"_LL_",   WIDTH_1,                        0}},
    {{"_MAF",   WIDTH_1,                        0}},
    {{"_MAX",   WIDTH_ADDRESS,                  0}},
    {{"_MEM",   WIDTH_2,                        0}},
    {{"_MIF",   WIDTH_1,                        0}},
    {{"_MIN",   WIDTH_ADDRESS,                  0}},
    {{"_MOD",   WIDTH_1,                        0}},  /* Acpi 5.0 */
    {{"_MTP",   WIDTH_2,                        0}},
    {{"_PAR",   WIDTH_8,                        0}},  /* Acpi 5.0 */
    {{"_PHA",   WIDTH_1,                        0}},  /* Acpi 5.0 */
    {{"_PIN",   WIDTH_16,                       0}},  /* Acpi 5.0 */
    {{"_PPI",   WIDTH_8,                        0}},  /* Acpi 5.0 */
    {{"_POL",   WIDTH_1 | WIDTH_2,              0}},  /* Acpi 5.0 */
    {{"_RBO",   WIDTH_8,                        0}},
    {{"_RBW",   WIDTH_8,                        0}},
    {{"_RNG",   WIDTH_1,                        0}},
    {{"_RT_",   WIDTH_8,                        0}},  /* Acpi 3.0 */
    {{"_RW_",   WIDTH_1,                        0}},
    {{"_RXL",   WIDTH_16,                       0}},  /* Acpi 5.0 */
    {{"_SHR",   WIDTH_2,                        0}},
    {{"_SIZ",   WIDTH_2,                        0}},
    {{"_SLV",   WIDTH_1,                        0}},  /* Acpi 5.0 */
    {{"_SPE",   WIDTH_32,                       0}},  /* Acpi 5.0 */
    {{"_STB",   WIDTH_2,                        0}},  /* Acpi 5.0 */
    {{"_TRA",   WIDTH_ADDRESS,                  0}},
    {{"_TRS",   WIDTH_1,                        0}},
    {{"_TSF",   WIDTH_8,                        0}},  /* Acpi 3.0 */
    {{"_TTP",   WIDTH_1,                        0}},
    {{"_TXL",   WIDTH_16,                       0}},  /* Acpi 5.0 */
    {{"_TYP",   WIDTH_2 | WIDTH_16,             0}},
    {{"_VEN",   VARIABLE_DATA,                  0}},  /* Acpi 5.0 */
    PACKAGE_INFO (0,0,0,0,0,0) /* Table terminator */
};

static const ACPI_PREDEFINED_INFO      AcpiGbl_ScopeNames[] = {
    {{"_GPE",     0,      0}},
    {{"_PR_",     0,      0}},
    {{"_SB_",     0,      0}},
    {{"_SI_",     0,      0}},
    {{"_TZ_",     0,      0}},
    PACKAGE_INFO (0,0,0,0,0,0) /* Table terminator */
};
#else
extern const ACPI_PREDEFINED_INFO      AcpiGbl_ResourceNames[];
#endif

#endif
