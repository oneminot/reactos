/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/hal/ddraw.c
 * PURPOSE:              DirectDraw HAL Implementation 
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */

#include "rosdraw.h"


HRESULT Hal_DirectDraw_Initialize (LPDIRECTDRAW7 iface)
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
 
	/* get the object */
	if(!DdCreateDirectDrawObject (&This->DirectDrawGlobal, This->hdc))
		return DDERR_INVALIDPARAMS;

 	/* alloc some space */
	This->DirectDrawGlobal.lpDDCBtmp = (LPDDHAL_CALLBACKS)HeapAlloc(GetProcessHeap(), 0, sizeof(DDHAL_CALLBACKS));
	memset(This->DirectDrawGlobal.lpDDCBtmp, 0, sizeof(DDHAL_CALLBACKS));
	This->HalInfo.lpDDCallbacks = &This->DirectDrawGlobal.lpDDCBtmp->cbDDCallbacks;
	This->HalInfo.lpDDSurfaceCallbacks = &This->DirectDrawGlobal.lpDDCBtmp->cbDDSurfaceCallbacks;
	This->HalInfo.lpDDExeBufCallbacks = &This->DirectDrawGlobal.lpDDCBtmp->cbDDExeBufCallbacks;
	This->HalInfo.lpDDPaletteCallbacks = &This->DirectDrawGlobal.lpDDCBtmp->cbDDPaletteCallbacks;
	This->DirectDrawGlobal.lpD3DHALCallbacks = (ULONG_PTR)HeapAlloc(GetProcessHeap(), 0, sizeof(D3DHAL_CALLBACKS));
	This->DirectDrawGlobal.lpD3DGlobalDriverData = (ULONG_PTR)HeapAlloc(GetProcessHeap(), 0, sizeof(D3DHAL_GLOBALDRIVERDATA));

	/* fill the sizeofs */
	This->HalInfo.dwSize = sizeof(DDHALINFO);
	This->HalInfo.lpDDCallbacks->dwSize = sizeof(DDHAL_DDCALLBACKS);
	This->HalInfo.lpDDSurfaceCallbacks->dwSize = sizeof(DDHAL_DDSURFACECALLBACKS);
	This->HalInfo.lpDDPaletteCallbacks->dwSize = sizeof(DDHAL_DDPALETTECALLBACKS);
	This->HalInfo.lpDDExeBufCallbacks->dwSize = sizeof(DDHAL_DDEXEBUFCALLBACKS);
	((LPD3DHAL_CALLBACKS)This->DirectDrawGlobal.lpD3DHALCallbacks)->dwSize = sizeof(D3DHAL_CALLBACKS);
	((LPD3DHAL_GLOBALDRIVERDATA)This->DirectDrawGlobal.lpD3DGlobalDriverData)->dwSize = sizeof(D3DHAL_GLOBALDRIVERDATA);	

	/* query all kinds of infos from the driver */
	if(!DdQueryDirectDrawObject (
		&This->DirectDrawGlobal, 
		&This->HalInfo, 
		This->HalInfo.lpDDCallbacks,
		This->HalInfo.lpDDSurfaceCallbacks,
		This->HalInfo.lpDDPaletteCallbacks,
		(LPD3DHAL_CALLBACKS)This->DirectDrawGlobal.lpD3DHALCallbacks,
		(LPD3DHAL_GLOBALDRIVERDATA)This->DirectDrawGlobal.lpD3DGlobalDriverData,
		This->HalInfo.lpDDExeBufCallbacks, 
		NULL, 
		NULL, 
		NULL ))
	{
		OutputDebugString(L"First DdQueryDirectDrawObject failed");
		return 1;
	}

	/* ms wants us to call that function twice */	
	This->HalInfo.vmiData.pvmList = HeapAlloc(GetProcessHeap(), 0, sizeof(VIDMEM) * This->HalInfo.vmiData.dwNumHeaps);
	This->DirectDrawGlobal.lpdwFourCC = HeapAlloc(GetProcessHeap(), 0, sizeof(DWORD) * This->HalInfo.ddCaps.dwNumFourCCCodes);
	This->DirectDrawGlobal.lpZPixelFormats = HeapAlloc(GetProcessHeap(), 0, sizeof(DDPIXELFORMAT) * This->DirectDrawGlobal.dwNumZPixelFormats);
	((LPD3DHAL_GLOBALDRIVERDATA)This->DirectDrawGlobal.lpD3DGlobalDriverData)->lpTextureFormats = HeapAlloc(GetProcessHeap(), 0, sizeof(DDSURFACEDESC) * ((LPD3DHAL_GLOBALDRIVERDATA)This->DirectDrawGlobal.lpD3DGlobalDriverData)->dwNumTextureFormats);

	if(!DdQueryDirectDrawObject (
		&This->DirectDrawGlobal, 
		&This->HalInfo, 
		This->HalInfo.lpDDCallbacks,
		This->HalInfo.lpDDSurfaceCallbacks,
		This->HalInfo.lpDDPaletteCallbacks,
		(LPD3DHAL_CALLBACKS)This->DirectDrawGlobal.lpD3DHALCallbacks,
		(LPD3DHAL_GLOBALDRIVERDATA)This->DirectDrawGlobal.lpD3DGlobalDriverData,
		This->HalInfo.lpDDExeBufCallbacks, 
		((LPD3DHAL_GLOBALDRIVERDATA)This->DirectDrawGlobal.lpD3DGlobalDriverData)->lpTextureFormats, 
		This->DirectDrawGlobal.lpdwFourCC, 
		This->HalInfo.vmiData.pvmList 
		))
	{
		OutputDebugString(L"Second DdQueryDirectDrawObject failed");
		return 1;
	}
		
	/* Copy HalInfo to DirectDrawGlobal */

	/* have not check where it should go into yet  
       LPDDHAL_DDCALLBACKS		lpDDCallbacks;
       LPDDHAL_DDSURFACECALLBACKS	lpDDSurfaceCallbacks;
       LPDDHAL_DDPALETTECALLBACKS	lpDDPaletteCallbacks;
     */
    
	RtlCopyMemory(&This->DirectDrawGlobal.vmiData,&This->HalInfo.vmiData,sizeof(VIDMEMINFO));
	RtlCopyMemory(&This->DirectDrawGlobal.ddCaps,&This->HalInfo.ddCaps,sizeof(DDCORECAPS));
	This->DirectDrawGlobal.dwMonitorFrequency = This->HalInfo.dwMonitorFrequency;
        
    /* have not check where it should go into yet
	   LPDDHAL_GETDRIVERINFO	GetDriverInfo;
    */

    This->DirectDrawGlobal.dwModeIndex = This->HalInfo.dwModeIndex;

	/* have not check where it should go into yet
       LPDWORD			lpdwFourCC;
     */

    This->DirectDrawGlobal.dwNumModes =  This->HalInfo.dwNumModes;
    This->DirectDrawGlobal.lpModeInfo =  This->HalInfo.lpModeInfo;

	/* have not check where it should go into yet
       DWORD			    dwFlags;
	*/

	/* Unsure which of these two for lpPDevice 
      This->DirectDrawGlobal.dwPDevice = This->HalInfo.lpPDevice;
	  This->lpDriverHandle = This->HalInfo.lpPDevice;
	*/

    This->DirectDrawGlobal.hInstance = This->HalInfo.hInstance;    
    
	/* have not check where it should go into yet
       ULONG_PTR			lpD3DGlobalDriverData;
       ULONG_PTR			lpD3DHALCallbacks;
       LPDDHAL_DDEXEBUFCALLBACKS	lpDDExeBufCallbacks;
    */ 
	

	
	This->DirectDrawGlobal.lpDDCBtmp->HALDD = This->DirectDrawGlobal.lpDDCBtmp->cbDDCallbacks;
	This->DirectDrawGlobal.lpDDCBtmp->HALDDSurface = This->DirectDrawGlobal.lpDDCBtmp->cbDDSurfaceCallbacks;
	This->DirectDrawGlobal.lpDDCBtmp->HALDDExeBuf = This->DirectDrawGlobal.lpDDCBtmp->cbDDExeBufCallbacks;
	This->DirectDrawGlobal.lpDDCBtmp->HALDDPalette = This->DirectDrawGlobal.lpDDCBtmp->cbDDPaletteCallbacks;

	return DD_OK;
}

HRESULT Hal_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7 iface)
{
   	return DD_OK;
}

VOID Hal_DirectDraw_Release (LPDIRECTDRAW7 iface) 
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

	DdDeleteDirectDrawObject (&This->DirectDrawGlobal);

	if(This->HalInfo.vmiData.pvmList)
		HeapFree(GetProcessHeap(), 0, This->HalInfo.vmiData.pvmList);
	if(This->DirectDrawGlobal.lpdwFourCC)
		HeapFree(GetProcessHeap(), 0, This->DirectDrawGlobal.lpdwFourCC);
	if(This->DirectDrawGlobal.lpZPixelFormats)
		HeapFree(GetProcessHeap(), 0, This->DirectDrawGlobal.lpZPixelFormats);
	if(((LPD3DHAL_GLOBALDRIVERDATA)This->DirectDrawGlobal.lpD3DGlobalDriverData)->lpTextureFormats)
		HeapFree(GetProcessHeap(), 0, ((LPD3DHAL_GLOBALDRIVERDATA)This->DirectDrawGlobal.lpD3DGlobalDriverData)->lpTextureFormats);			
	
	if(This->DirectDrawGlobal.lpDDCBtmp)
		HeapFree(GetProcessHeap(), 0, This->DirectDrawGlobal.lpDDCBtmp);
	if(This->DirectDrawGlobal.lpD3DHALCallbacks)
		HeapFree(GetProcessHeap(), 0, (PVOID)This->DirectDrawGlobal.lpD3DHALCallbacks);
	if(This->DirectDrawGlobal.lpD3DGlobalDriverData)
		HeapFree(GetProcessHeap(), 0, (PVOID)This->DirectDrawGlobal.lpD3DGlobalDriverData);
}
