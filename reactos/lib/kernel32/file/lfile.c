/* $Id: lfile.c,v 1.10 2004/08/28 22:08:54 navaraf Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/lfile.c
 * PURPOSE:         Find functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <k32.h>


/*
 * @implemented
 */
long
STDCALL
_hread(
	HFILE	hFile,
	LPVOID	lpBuffer,
	long	lBytes
	)
{
	DWORD	NumberOfBytesRead;
	
	if (ReadFile(
		(HANDLE) hFile,
		(LPVOID) lpBuffer,
		(DWORD) lBytes,
		& NumberOfBytesRead,
		NULL
		) == FALSE)
	{
		return -1;
	}
	return NumberOfBytesRead;
}


/*
 * @implemented
 */
long
STDCALL
_hwrite (
	HFILE	hFile,
	LPCSTR	lpBuffer,
	long	lBytes
	)
{
	DWORD	NumberOfBytesWritten;
	
	if (lBytes == 0)
	{
		if ( SetEndOfFile((HANDLE) hFile ) == FALSE )
		{
			return -1;
		}
		return 0;
	}
	if ( WriteFile(
		(HANDLE) hFile,
		(LPVOID) lpBuffer,
		(DWORD) lBytes,
		& NumberOfBytesWritten,
		NULL
		) == FALSE )
	{
		return -1;
	}
	return NumberOfBytesWritten;
}


/*
 * @implemented
 */
HFILE
STDCALL
_lopen (
	LPCSTR	lpPathName,
	int	iReadWrite
	)
{
	DWORD dwAccessMask = 0;
	DWORD dwShareMode = 0;
	
	if ( (iReadWrite & OF_READWRITE ) == OF_READWRITE )
		dwAccessMask = GENERIC_READ | GENERIC_WRITE;
	else if ( (iReadWrite & OF_READ ) == OF_READ )
		dwAccessMask = GENERIC_READ;
	else if ( (iReadWrite & OF_WRITE ) == OF_WRITE )
		dwAccessMask = GENERIC_WRITE;

	if ((iReadWrite & OF_SHARE_COMPAT) == OF_SHARE_COMPAT )
		dwShareMode = FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE;
	else if ((iReadWrite & OF_SHARE_DENY_NONE) == OF_SHARE_DENY_NONE)
		dwShareMode = FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE;
	else if ((iReadWrite & OF_SHARE_DENY_READ) == OF_SHARE_DENY_READ)
		dwShareMode = FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	else if ((iReadWrite & OF_SHARE_DENY_WRITE) == OF_SHARE_DENY_WRITE )
		dwShareMode = FILE_SHARE_READ | FILE_SHARE_DELETE;
	else if ((iReadWrite & OF_SHARE_EXCLUSIVE) == OF_SHARE_EXCLUSIVE)
		dwShareMode = 0;

	return (HFILE) CreateFileA(
			lpPathName,
			dwAccessMask,
			dwShareMode,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
}


/*
 * @implemented
 */
HFILE
STDCALL
_lcreat (
	LPCSTR	lpPathName,
	int	iAttribute
	)
{
	return (HFILE) CreateFileA(
			lpPathName,
			GENERIC_ALL,
			(FILE_SHARE_READ | FILE_SHARE_WRITE),
			NULL,
			CREATE_ALWAYS,
			iAttribute & FILE_ATTRIBUTE_VALID_FLAGS,
			NULL);
}


/*
 * @implemented
 */
int
STDCALL
_lclose (
	HFILE	hFile
	)
{
	if (CloseHandle ((HANDLE)hFile))
	{
		return 0;
	}
	return -1;
}


/*
 * @implemented
 */
LONG
STDCALL
_llseek(
	HFILE	hFile,
	LONG	lOffset,
	int	iOrigin
	)
{
	return SetFilePointer (
			(HANDLE) hFile,
			lOffset,
			NULL,
			(DWORD) iOrigin);
}

/* EOF */
