/*
 * Copyright 1999 Marcus Meissner
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

#ifndef __WINE_MSVIDEO_PRIVATE_H
#define __WINE_MSVIDEO_PRIVATE_H

/* Installable Compressor Manager */
#define ICVERSION 0x0104

#define ICM_CHOOSE_COMPRESSOR 1
#define IDC_COMP_LIST 880
#define IDS_FULLFRAMES 901

/* HIC struct (same layout as Win95 one) */
typedef struct tagWINE_HIC {
    DWORD		magic;		/* 00: 'Smag' */
    HANDLE		curthread;	/* 04: */
    DWORD		type;		/* 08: */
    DWORD		handler;	/* 0C: */
    HDRVR		hdrv;		/* 10: */
    DWORD		private;	/* 14:(handled by SendDriverMessage)*/
    DRIVERPROC  	driverproc;	/* 18:(handled by SendDriverMessage)*/
    DWORD		x1;		/* 1c: name? */
    WORD		x2;		/* 20: */
    DWORD		x3;		/* 22: */
					/* 26: */
    DWORD               driverproc16;   /* Wine specific flags */
    HIC                 hic;
    DWORD               driverId;
    struct tagWINE_HIC* next;
} WINE_HIC;

HIC             MSVIDEO_OpenFunction(DWORD, DWORD, UINT, DRIVERPROC, DWORD);
LRESULT         MSVIDEO_SendMessage(WINE_HIC*, UINT, DWORD_PTR, DWORD_PTR);
WINE_HIC*       MSVIDEO_GetHicPtr(HIC);

extern LRESULT  (CALLBACK *pFnCallTo16)(HDRVR, HIC, UINT, LPARAM, LPARAM);

/* handle16 --> handle conversions */
#define HDRAWDIB_32(h16)	((HDRAWDIB)(ULONG_PTR)(h16))
#define HIC_32(h16)		((HIC)(ULONG_PTR)(h16))

/* handle --> handle16 conversions */
#define HDRVR_16(h32)		(LOWORD(h32))
#define HDRAWDIB_16(h32)	(LOWORD(h32))
#define HIC_16(h32)		(LOWORD(h32))

#define IDC_CONFIGURE 882
#define IDC_ABOUT 883

#define IDC_QUALITY_SCROLL 884
#define IDC_QUALITY_TXT 886

#define IDC_KEYFRAME_CHECKBOX 887
#define IDC_KEYFRAME  888
#define IDC_KEYFRAME_FRAMES 889
#define IDC_DATARATE_CHECKBOX 894
#define IDC_DATARATE  895
#define IDC_DATARATE_KB 896

#endif  /* __WINE_MSVIDEO_PRIVATE_H */
