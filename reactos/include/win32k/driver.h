
#ifndef __WIN32K_DRIVER_H
#define __WIN32K_DRIVER_H

#include <ddk/winddi.h>

typedef BOOL STDCALL (*PGD_ENABLEDRIVER)(ULONG, ULONG, PDRVENABLEDATA);
typedef DHPDEV STDCALL (*PGD_ENABLEPDEV)(DEVMODEW  *,
                                 LPWSTR,
                                 ULONG,
                                 HSURF  *,
                                 ULONG,
                                 ULONG  *,
                                 ULONG,
                                 DEVINFO  *,
                                 LPWSTR,
                                 LPWSTR,
                                 HANDLE);
typedef VOID STDCALL (*PGD_COMPLETEPDEV)(DHPDEV, HDEV);
typedef VOID STDCALL (*PGD_DISABLEPDEV)(DHPDEV); 
typedef HSURF STDCALL (*PGD_ENABLESURFACE)(DHPDEV);
typedef VOID STDCALL (*PGD_DISABLESURFACE)(DHPDEV);
typedef BOOL STDCALL (*PGD_ASSERTMODE)(DHPDEV, BOOL);
typedef BOOL STDCALL (*PGD_RESETPDEV)(DHPDEV, DHPDEV);
typedef HBITMAP STDCALL (*PGD_CREATEDEVICEBITMAP)(DHPDEV, SIZEL, ULONG); 
typedef VOID STDCALL (*PGD_DELETEDEVICEBITMAP)(DHSURF); 
typedef BOOL STDCALL (*PGD_REALIZEBRUSH)(PBRUSHOBJ, PSURFOBJ, PSURFOBJ, PSURFOBJ,
                                 PXLATEOBJ, ULONG); 
typedef ULONG STDCALL (*PGD_DITHERCOLOR)(DHPDEV, ULONG, ULONG, PULONG); 
typedef BOOL STDCALL (*PGD_STROKEPATH)(PSURFOBJ, PPATHOBJ, PCLIPOBJ, PXFORMOBJ,
                               PBRUSHOBJ, PPOINTL, PLINEATTRS, MIX); 
typedef BOOL STDCALL (*PGD_FILLPATH)(PSURFOBJ, PPATHOBJ, PCLIPOBJ, PBRUSHOBJ,
                             PPOINTL, MIX, ULONG); 
typedef BOOL STDCALL (*PGD_STROKEANDFILLPATH)(PSURFOBJ, PPATHOBJ, PCLIPOBJ,
                                      PXFORMOBJ, PBRUSHOBJ, PLINEATTRS,
                                      PBRUSHOBJ, PPOINTL, MIX, ULONG); 
typedef BOOL STDCALL (*PGD_PAINT)(PSURFOBJ, PCLIPOBJ, PBRUSHOBJ, PPOINTL, MIX); 
typedef BOOL STDCALL (*PGD_BITBLT)(PSURFOBJ, PSURFOBJ, PSURFOBJ, PCLIPOBJ,
                           PXLATEOBJ, PRECTL, PPOINTL, PPOINTL, PBRUSHOBJ,
                           PPOINTL, ROP4); 
typedef BOOL STDCALL (*PGD_TRANSPARENTBLT)(PSURFOBJ, PSURFOBJ, PCLIPOBJ, PXLATEOBJ, PRECTL, PRECTL, ULONG, ULONG);
typedef BOOL STDCALL (*PGD_COPYBITS)(PSURFOBJ, PSURFOBJ, PCLIPOBJ, PXLATEOBJ,
                             PRECTL, PPOINTL); 
typedef BOOL STDCALL (*PGD_STRETCHBLT)(PSURFOBJ, PSURFOBJ, PSURFOBJ, PCLIPOBJ,
                               PXLATEOBJ, PCOLORADJUSTMENT, PPOINTL,
                               PRECTL, PRECTL, PPOINTL, ULONG); 
typedef BOOL STDCALL (*PGD_SETPALETTE)(DHPDEV, PPALOBJ, ULONG, ULONG, ULONG); 
typedef BOOL STDCALL (*PGD_TEXTOUT)(PSURFOBJ, PSTROBJ, PFONTOBJ, PCLIPOBJ, PRECTL,
                            PRECTL, PBRUSHOBJ, PBRUSHOBJ, PPOINTL, MIX); 
typedef ULONG STDCALL (*PGD_ESCAPE)(PSURFOBJ, ULONG, ULONG, PVOID *, ULONG, PVOID *); 
typedef ULONG STDCALL (*PGD_DRAWESCAPE)(PSURFOBJ, ULONG, PCLIPOBJ, PRECTL, ULONG, 
                                PVOID *); 
typedef PIFIMETRICS STDCALL (*PGD_QUERYFONT)(DHPDEV, ULONG, ULONG, PULONG); 
typedef PVOID STDCALL (*PGD_QUERYFONTTREE)(DHPDEV, ULONG, ULONG, ULONG, PULONG); 
typedef LONG STDCALL (*PGD_QUERYFONTDATA)(DHPDEV, PFONTOBJ, ULONG, HGLYPH, PGLYPHDATA,
                                  PVOID, ULONG); 
typedef ULONG STDCALL (*PGD_SETPOINTERSHAPE)(PSURFOBJ, PSURFOBJ, PSURFOBJ, PXLATEOBJ,
                                     LONG, LONG, LONG, LONG, PRECTL, ULONG); 
typedef VOID STDCALL (*PGD_MOVEPOINTER)(PSURFOBJ, LONG, LONG, PRECTL); 
typedef BOOL STDCALL (*PGD_LINETO)(PSURFOBJ, PCLIPOBJ, PBRUSHOBJ, LONG, LONG, LONG,
                           LONG, PRECTL, MIX);
typedef BOOL STDCALL (*PGD_SENDPAGE)(PSURFOBJ);
typedef BOOL STDCALL (*PGD_STARTPAGE)(PSURFOBJ);
typedef BOOL STDCALL (*PGD_ENDDOC)(PSURFOBJ, ULONG);
typedef BOOL STDCALL (*PGD_STARTDOC)(PSURFOBJ, PWSTR, DWORD);
typedef ULONG STDCALL (*PGD_GETGLYPHMODE)(DHPDEV, PFONTOBJ);
typedef VOID STDCALL (*PGD_SYNCHRONIZE)(DHPDEV, PRECTL);
typedef ULONG STDCALL (*PGD_SAVESCREENBITS)(PSURFOBJ, ULONG, ULONG, PRECTL);
typedef ULONG STDCALL (*PGD_GETMODES)(HANDLE, ULONG, PDEVMODEW);
typedef VOID STDCALL (*PGD_FREE)(PVOID, ULONG);
typedef VOID STDCALL (*PGD_DESTROYFONT)(PFONTOBJ);
typedef LONG STDCALL (*PGD_QUERYFONTCAPS)(ULONG, PULONG);
typedef ULONG STDCALL (*PGD_LOADFONTFILE)(ULONG, PVOID, ULONG, ULONG);
typedef BOOL STDCALL (*PGD_UNLOADFONTFILE)(ULONG);
typedef ULONG STDCALL (*PGD_FONTMANAGEMENT)(PSURFOBJ, PFONTOBJ, ULONG, ULONG, PVOID,
                                    ULONG, PVOID);
typedef LONG STDCALL (*PGD_QUERYTRUETYPETABLE)(ULONG, ULONG, ULONG, PTRDIFF, ULONG,
                                       PBYTE);
typedef LONG STDCALL (*PGD_QUERYTRUETYPEOUTLINE)(DHPDEV, PFONTOBJ, HGLYPH, BOOL,
                                         PGLYPHDATA, ULONG, PTTPOLYGONHEADER);
typedef PVOID STDCALL (*PGD_GETTRUETYPEFILE)(ULONG, PULONG);
typedef LONG STDCALL (*PGD_QUERYFONTFILE)(ULONG, ULONG, ULONG, PULONG);
typedef BOOL STDCALL (*PGD_QUERYADVANCEWIDTHS)(DHPDEV, PFONTOBJ, ULONG, HGLYPH *,
                                       PVOID *, ULONG);
typedef BOOL STDCALL (*PGD_SETPIXELFORMAT)(PSURFOBJ, LONG, ULONG);
typedef LONG STDCALL (*PGD_DESCRIBEPIXELFORMAT)(DHPDEV, LONG, ULONG,
                                        PPIXELFORMATDESCRIPTOR);
typedef BOOL STDCALL (*PGD_SWAPBUFFERS)(PSURFOBJ, PWNDOBJ);
typedef BOOL STDCALL (*PGD_STARTBANDING)(PSURFOBJ, PPOINTL);
typedef BOOL STDCALL (*PGD_NEXTBAND)(PSURFOBJ, PPOINTL);

typedef BOOL STDCALL (*PGD_GETDIRECTDRAWINFO)(DHPDEV, PDD_HALINFO, PDWORD, PVIDEOMEMORY, PDWORD, PDWORD);
typedef BOOL STDCALL (*PGD_ENABLEDIRECTDRAW)(DHPDEV, PDD_CALLBACKS, PDD_SURFACECALLBACKS, PDD_PALETTECALLBACKS);
typedef VOID STDCALL (*PGD_DISABLEDIRECTDRAW)(DHPDEV);

typedef LONG STDCALL (*PGD_QUERYSPOOLTYPE)(DHPDEV, LPWSTR);


typedef struct _DRIVER_FUNCTIONS
{
  PGD_ENABLEDRIVER  EnableDriver;
  PGD_ENABLEPDEV  EnablePDev;
  PGD_COMPLETEPDEV  CompletePDev;
  PGD_DISABLEPDEV  DisablePDev;
  PGD_ENABLESURFACE  EnableSurface;
  PGD_DISABLESURFACE  DisableSurface;
  PGD_ASSERTMODE  AssertMode;
  PGD_RESETPDEV  ResetPDev;
  PGD_CREATEDEVICEBITMAP  CreateDeviceBitmap;
  PGD_DELETEDEVICEBITMAP  DeleteDeviceBitmap;
  PGD_REALIZEBRUSH  RealizeBrush;
  PGD_DITHERCOLOR  DitherColor;
  PGD_STROKEPATH  StrokePath;
  PGD_FILLPATH  FillPath;
  PGD_STROKEANDFILLPATH  StrokeAndFillPath;
  PGD_PAINT  Paint;
  PGD_BITBLT  BitBlt;
  PGD_TRANSPARENTBLT TransparentBlt;
  PGD_COPYBITS  CopyBits;
  PGD_STRETCHBLT  StretchBlt;
  PGD_SETPALETTE  SetPalette;
  PGD_TEXTOUT  TextOut;
  PGD_ESCAPE  Escape;
  PGD_DRAWESCAPE  DrawEscape;
  PGD_QUERYFONT  QueryFont;
  PGD_QUERYFONTTREE  QueryFontTree;
  PGD_QUERYFONTDATA  QueryFontData;
  PGD_SETPOINTERSHAPE  SetPointerShape;
  PGD_MOVEPOINTER  MovePointer;
  PGD_LINETO  LineTo;
  PGD_SENDPAGE  SendPage;
  PGD_STARTPAGE  StartPage;
  PGD_ENDDOC  EndDoc;
  PGD_STARTDOC  StartDoc;
  PGD_GETGLYPHMODE  GetGlyphMode;
  PGD_SYNCHRONIZE  Synchronize;
  PGD_SAVESCREENBITS  SaveScreenBits;
  PGD_GETMODES  GetModes;
  PGD_FREE  Free;
  PGD_DESTROYFONT  DestroyFont;
  PGD_QUERYFONTCAPS  QueryFontCaps;
  PGD_LOADFONTFILE  LoadFontFile;
  PGD_UNLOADFONTFILE  UnloadFontFile;
  PGD_FONTMANAGEMENT  FontManagement;
  PGD_QUERYTRUETYPETABLE  QueryTrueTypeTable;
  PGD_QUERYTRUETYPEOUTLINE  QueryTrueTypeOutline;
  PGD_GETTRUETYPEFILE  GetTrueTypeFile;
  PGD_QUERYFONTFILE  QueryFontFile;
  PGD_QUERYADVANCEWIDTHS  QueryAdvanceWidths;
  PGD_SETPIXELFORMAT  SetPixelFormat;
  PGD_DESCRIBEPIXELFORMAT  DescribePixelFormat;
  PGD_SWAPBUFFERS  SwapBuffers;
  PGD_STARTBANDING  StartBanding;
  PGD_NEXTBAND  NextBand;
  PGD_GETDIRECTDRAWINFO  GetDirectDrawInfo;
  PGD_ENABLEDIRECTDRAW  EnableDirectDraw;
  PGD_DISABLEDIRECTDRAW  DisableDirectDraw;
  PGD_QUERYSPOOLTYPE  QuerySpoolType;
} DRIVER_FUNCTIONS, *PDRIVER_FUNCTIONS;

BOOL  DRIVER_RegisterDriver(LPCWSTR  Name, PGD_ENABLEDRIVER  EnableDriver);
PGD_ENABLEDRIVER  DRIVER_FindDDIDriver(LPCWSTR  Name);
HANDLE  DRIVER_FindMPDriver(LPCWSTR  Name);
BOOL  DRIVER_BuildDDIFunctions(PDRVENABLEDATA  DED, 
                               PDRIVER_FUNCTIONS  DF);
BOOL  DRIVER_UnregisterDriver(LPCWSTR  Name);
INT  DRIVER_ReferenceDriver (LPCWSTR  Name);
INT  DRIVER_UnreferenceDriver (LPCWSTR  Name);

#endif

