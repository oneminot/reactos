/*
 *  ReactOS Task Manager
 *
 *  trayicon.h
 *
 *  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
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
	
#ifndef __TRAYICON_H
#define __TRAYICON_H

#define WM_ONTRAYICON   WM_USER + 5

HICON	TrayIcon_GetProcessorUsageIcon(void);
BOOL	TrayIcon_ShellAddTrayIcon(void);
BOOL	TrayIcon_ShellRemoveTrayIcon(void);
BOOL	TrayIcon_ShellUpdateTrayIcon(void);

#endif /* __TRAYICON_H */
