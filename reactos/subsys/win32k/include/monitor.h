#ifndef _WIN32K_MONITOR_H
#define _WIN32K_MONITOR_H

#include <windows.h>

struct GDIDEVICE;

/* monitor object */
typedef struct _MONITOR_OBJECT
{
	HANDLE         Handle;     /* system object handle */
	FAST_MUTEX     Lock;       /* R/W lock */

	BOOL           IsPrimary;  /* wether this is the primary monitor */
	UNICODE_STRING DeviceName; /* name of the monitor */
	GDIDEVICE     *GdiDevice;  /* pointer to the GDI device to
	                              which this monitor is attached */
	struct _MONITOR_OBJECT *Prev, *Next; /* doubly linked list */
} MONITOR_OBJECT, *PMONITOR_OBJECT;

/* functions */
NTSTATUS InitMonitorImpl();
NTSTATUS CleanupMonitorImpl();

NTSTATUS IntAttachMonitor(GDIDEVICE *pGdiDevice, ULONG DisplayNumber);
NTSTATUS IntDetachMonitor(GDIDEVICE *pGdiDevice);

#endif /* _WIN32K_MONITOR_H */

/* EOF */
