/* $Id: internal.h,v 1.3 2004/01/15 14:59:06 ekohl Exp $ 
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/userenv/internal.h
 * PURPOSE:         internal stuff
 * PROGRAMMER:      Eric Kohl
 */

#ifndef _INTERNAL_H
#define _INTERNAL_H

/* debug.h */
void
DebugPrint (char* fmt,...);

#define DPRINT1 DebugPrint("(%s:%d) ",__FILE__,__LINE__), DebugPrint
#define CHECKPOINT1 do { DebugPrint("%s:%d\n",__FILE__,__LINE__); } while(0);

/* directory.h */
BOOL
CopyDirectory (LPCWSTR lpDestinationPath,
	       LPCWSTR lpSourcePath);

#endif /* _INTERNAL_H */

/* EOF */
