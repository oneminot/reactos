/*
 * nt_vdd.h
 *
 * Windows NT Device Driver Kit
 *
 * This file is part of the ReactOS DDK package.
 *
 * Contributors:
 *   Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#pragma once

#define _NT_VDD

#ifdef __cplusplus
extern "C" {
#endif

/*
 * I/O Port services
 */

typedef VOID (*PFNVDD_INB)   (WORD iport, PBYTE data);
typedef VOID (*PFNVDD_INW)   (WORD iport, PWORD data);
typedef VOID (*PFNVDD_INSB)  (WORD iport, PBYTE data, WORD count);
typedef VOID (*PFNVDD_INSW)  (WORD iport, PWORD data, WORD count);
typedef VOID (*PFNVDD_OUTB)  (WORD iport, BYTE  data);
typedef VOID (*PFNVDD_OUTW)  (WORD iport, WORD  data);
typedef VOID (*PFNVDD_OUTSB) (WORD iport, PBYTE data, WORD count);
typedef VOID (*PFNVDD_OUTSW) (WORD iport, PWORD data, WORD count);

typedef struct _VDD_IO_HANDLERS
{
    PFNVDD_INB   inb_handler;
    PFNVDD_INW   inw_handler;
    PFNVDD_INSB  insb_handler;
    PFNVDD_INSW  insw_handler;
    PFNVDD_OUTB  outb_handler;
    PFNVDD_OUTW  outw_handler;
    PFNVDD_OUTSB outsb_handler;
    PFNVDD_OUTSW outsw_handler;
} VDD_IO_HANDLERS, *PVDD_IO_HANDLERS;

typedef struct _VDD_IO_PORTRANGE
{
    WORD First;
    WORD Last;
} VDD_IO_PORTRANGE, *PVDD_IO_PORTRANGE;

BOOL
WINAPI
VDDInstallIOHook
(
    HANDLE            hVdd,
    WORD              cPortRange,
    PVDD_IO_PORTRANGE pPortRange,
    PVDD_IO_HANDLERS  IOhandler
);

VOID
WINAPI
VDDDeInstallIOHook
(
    HANDLE            hVdd,
    WORD              cPortRange,
    PVDD_IO_PORTRANGE pPortRange
);

#ifdef __cplusplus
}
#endif

/* EOF */
