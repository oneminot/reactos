#ifndef __WIN32K_PALETTE_H
#define __WIN32K_PALETTE_H

#define PALETTE_FIXED    0x0001 /* read-only colormap - have to use XAllocColor (if not virtual) */
#define PALETTE_VIRTUAL  0x0002 /* no mapping needed - pixel == pixel color */

#define PALETTE_PRIVATE  0x1000 /* private colormap, identity mapping */
#define PALETTE_WHITESET 0x2000

typedef struct {
    int shift;
    int scale;
    int max;
} ColorShifts;

HPALETTE FASTCALL PALETTE_Init (VOID);
VOID     FASTCALL PALETTE_ValidateFlags (PALETTEENTRY* lpPalE, INT size);
INT      STDCALL  PALETTE_SetMapping(PPALOBJ palPtr, UINT uStart, UINT uNum, BOOL mapOnly);
INT      FASTCALL PALETTE_ToPhysical (PDC dc, COLORREF color);

PPALETTEENTRY FASTCALL ReturnSystemPalette (VOID);

#endif /* __WIN32K_PALETTE_H */
