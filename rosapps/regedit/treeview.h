/*
 *  ReactOS regedit
 *
 *  treeview.h
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __TREEVIEW_H__
#define __TREEVIEW_H__

#ifdef __cplusplus
extern "C" {
#endif

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


HWND CreateTreeView(HWND hwndParent, LPTSTR pHostName, int id);
BOOL OnTreeExpanding(HWND hWnd, NMTREEVIEW* pnmtv);
HKEY FindRegRoot(HWND hwndTV, HTREEITEM hItem, LPTSTR keyPath, int* pPathLen, int max);


#ifdef __cplusplus
};
#endif

#endif // __TREEVIEW_H__
