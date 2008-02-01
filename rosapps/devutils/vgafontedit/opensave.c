/*
 * PROJECT:     ReactOS VGA Font Editor
 * LICENSE:     GNU General Public License Version 2.0 or any later version
 * FILE:        devutils/vgafontedit/opensave.c
 * PURPOSE:     Functions for opening and saving files
 * COPYRIGHT:   Copyright 2008 Colin Finck <mail@colinfinck.de>
 */

#include "precomp.h"

static OPENFILENAMEW ofn;

VOID
FileInitialize(IN HWND hwnd)
{
    ZeroMemory( &ofn, sizeof(ofn) );
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"bin";
}

static __inline VOID
PrepareFilter(IN PWSTR pszFilter)
{
    // RC strings can't be double-null terminated, so we use | instead to separate the entries.
    // Convert them back to null characters here.
    do
    {
        if(*pszFilter == '|')
            *pszFilter = 0;
    }
    while(*++pszFilter);
}

BOOL
DoOpenFile(OUT PWSTR pszFileName)
{
    BOOL bRet;
    PWSTR pszFilter;

    if( AllocAndLoadString(&pszFilter, IDS_OPENFILTER) )
    {
        PrepareFilter(pszFilter);
        ofn.lpstrFilter = pszFilter;
        ofn.lpstrFile = pszFileName;
        ofn.Flags = OFN_FILEMUSTEXIST;

        bRet = GetOpenFileNameW(&ofn);
        HeapFree(hProcessHeap, 0, pszFilter);

        return bRet;
    }

    return FALSE;
}

BOOL
DoSaveFile(IN OUT PWSTR pszFileName)
{
    BOOL bRet;
    PWSTR pszFilter;

    if( AllocAndLoadString(&pszFilter, IDS_SAVEFILTER) )
    {
        PrepareFilter(pszFilter);
        ofn.lpstrFilter = pszFilter;
        ofn.lpstrFile = pszFileName;

        bRet = GetSaveFileNameW(&ofn);
        HeapFree(hProcessHeap, 0, pszFilter);

        return bRet;
    }

    return FALSE;
}
