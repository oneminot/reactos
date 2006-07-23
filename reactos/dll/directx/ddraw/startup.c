/* $Id: main.c 21434 2006-04-01 19:12:56Z greatlrd $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/ddraw/ddraw.c
 * PURPOSE:              DirectDraw Library 
 * PROGRAMMER:           Magnus Olsen (greatlrd)
 *
 */

#include <windows.h>
#include "rosdraw.h"
#include "d3dhal.h"


HRESULT WINAPI 
StartDirectDraw(LPDIRECTDRAW* iface)
{
	IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
    DWORD hal_ret;
    DWORD hel_ret;
    DEVMODE devmode;
    HBITMAP hbmp;
    const UINT bmiSize = sizeof(BITMAPINFOHEADER) + 0x10;
    UCHAR *pbmiData;
    BITMAPINFO *pbmi;    
    DWORD *pMasks;
    
    DX_WINDBG_trace();
	  
	RtlZeroMemory(&This->mDDrawGlobal, sizeof(DDRAWI_DIRECTDRAW_GBL));
	
	/* cObsolete is undoc in msdn it being use in CreateDCA */
	RtlCopyMemory(&This->mDDrawGlobal.cObsolete,&"DISPLAY",7);
	RtlCopyMemory(&This->mDDrawGlobal.cDriverName,&"DISPLAY",7);
	
    /* Same for HEL and HAL */
    This->mcModeInfos = 1;
    This->mpModeInfos = (DDHALMODEINFO*) DxHeapMemAlloc(This->mcModeInfos * sizeof(DDHALMODEINFO));  
   
    if (This->mpModeInfos == NULL)
    {
	   DX_STUB_str("DD_FALSE");
       return DD_FALSE;
    }

    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);

    This->mpModeInfos[0].dwWidth      = devmode.dmPelsWidth;
    This->mpModeInfos[0].dwHeight     = devmode.dmPelsHeight;
    This->mpModeInfos[0].dwBPP        = devmode.dmBitsPerPel;
    This->mpModeInfos[0].lPitch       = (devmode.dmPelsWidth*devmode.dmBitsPerPel)/8;
    This->mpModeInfos[0].wRefreshRate = (WORD)devmode.dmDisplayFrequency;
   
    This->hdc = CreateDCW(L"DISPLAY",L"DISPLAY",NULL,NULL);    

    if (This->hdc == NULL)
    {
	   DX_STUB_str("DDERR_OUTOFMEMORY");
       return DDERR_OUTOFMEMORY ;
    }

    hbmp = CreateCompatibleBitmap(This->hdc, 1, 1);  
    if (hbmp==NULL)
    {
       DxHeapMemFree(This->mpModeInfos);
       DeleteDC(This->hdc);
	   DX_STUB_str("DDERR_OUTOFMEMORY");
       return DDERR_OUTOFMEMORY;
    }
  
    pbmiData = (UCHAR *) DxHeapMemAlloc(bmiSize);
    pbmi = (BITMAPINFO*)pbmiData;

    if (pbmiData==NULL)
    {
       DxHeapMemFree(This->mpModeInfos);       
       DeleteDC(This->hdc);
       DeleteObject(hbmp);
	   DX_STUB_str("DDERR_OUTOFMEMORY");
       return DDERR_OUTOFMEMORY;
    }

    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biBitCount = (WORD)devmode.dmBitsPerPel;
    pbmi->bmiHeader.biCompression = BI_BITFIELDS;
    pbmi->bmiHeader.biWidth = 1;
    pbmi->bmiHeader.biHeight = 1;

    GetDIBits(This->hdc, hbmp, 0, 0, NULL, pbmi, 0);
    DeleteObject(hbmp);

    pMasks = (DWORD*)(pbmiData + sizeof(BITMAPINFOHEADER));
    This->mpModeInfos[0].dwRBitMask = pMasks[0];
    This->mpModeInfos[0].dwGBitMask = pMasks[1];
    This->mpModeInfos[0].dwBBitMask = pMasks[2];
    This->mpModeInfos[0].dwAlphaBitMask = pMasks[3];

	DxHeapMemFree(pbmiData);

    /* Startup HEL and HAL */
    RtlZeroMemory(&This->mDDrawGlobal, sizeof(DDRAWI_DIRECTDRAW_GBL));
    RtlZeroMemory(&This->mHALInfo, sizeof(DDHALINFO));
    RtlZeroMemory(&This->mCallbacks, sizeof(DDHAL_CALLBACKS));

    This->mDDrawLocal.lpDDCB = &This->mCallbacks;
    This->mDDrawLocal.lpGbl = &This->mDDrawGlobal;
    This->mDDrawLocal.dwProcessId = GetCurrentProcessId();

    This->mDDrawGlobal.lpDDCBtmp = &This->mCallbacks;
    This->mDDrawGlobal.lpExclusiveOwner = &This->mDDrawLocal;

    hal_ret = StartDirectDrawHal(iface);
	hel_ret = StartDirectDrawHel(iface);    
    if ((hal_ret!=DD_OK) &&  (hel_ret!=DD_OK))
    {
		DX_STUB_str("DDERR_NODIRECTDRAWSUPPORT");
        return DDERR_NODIRECTDRAWSUPPORT; 
    }

    /* 
       Try figout which api we shall use, first we try see if HAL exits 
       if it does not we select HEL instead 
    */ 
            
    if (This->mDDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_CANCREATESURFACE) 
    {
        This->mDdCanCreateSurface.CanCreateSurface = This->mCallbacks.HALDD.CanCreateSurface;  
    }
    else
    {
        This->mDdCanCreateSurface.CanCreateSurface = This->mCallbacks.HELDD.CanCreateSurface;
    }
        
    This->mDdCreateSurface.lpDD = &This->mDDrawGlobal;
        
    if (This->mDDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_CREATESURFACE) 
    {
        This->mDdCreateSurface.CreateSurface = This->mCallbacks.HALDD.CreateSurface;  
    }
    else
    {
        This->mDdCreateSurface.CreateSurface = This->mCallbacks.HELDD.CreateSurface;
    }
    
    /* Setup calback struct so we do not need refill same info again */
    This->mDdCreateSurface.lpDD = &This->mDDrawGlobal;    
    This->mDdCanCreateSurface.lpDD = &This->mDDrawGlobal;  
                  
    return DD_OK;
}


HRESULT WINAPI 
StartDirectDrawHal(LPDIRECTDRAW* iface)
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

	/* HAL Startup process */
    BOOL newmode = FALSE;	
	    	

    /* 
      Startup DX HAL step one of three 
    */
    if (!DdCreateDirectDrawObject(&This->mDDrawGlobal, This->hdc))
    {
       DxHeapMemFree(This->mpModeInfos);	   
       DeleteDC(This->hdc);       
       return DD_FALSE;
    }
	
    // Do not relase HDC it have been map in kernel mode 
    // DeleteDC(hdc);
      
    if (!DdReenableDirectDrawObject(&This->mDDrawGlobal, &newmode))
    {
      DxHeapMemFree(This->mpModeInfos);
      DeleteDC(This->hdc);      
      return DD_FALSE;
    }
           
	
    /*
       Startup DX HAL step two of three 
    */

    if (!DdQueryDirectDrawObject(&This->mDDrawGlobal,
                                 &This->mHALInfo,
                                 &This->mCallbacks.HALDD,
                                 &This->mCallbacks.HALDDSurface,
                                 &This->mCallbacks.HALDDPalette, 
                                 &This->mD3dCallbacks,
                                 &This->mD3dDriverData,
                                 &This->mD3dBufferCallbacks,
                                 NULL,
                                 NULL,
                                 NULL))
    {
      DxHeapMemFree(This->mpModeInfos);
      DeleteDC(This->hdc);      
      // FIXME Close DX fristcall and second call
      return DD_FALSE;
    }

    This->mcvmList = This->mHALInfo.vmiData.dwNumHeaps;
    This->mpvmList = (VIDMEM*) DxHeapMemAlloc(sizeof(VIDMEM) * This->mcvmList);
    if (This->mpvmList == NULL)
    {      
      DxHeapMemFree(This->mpModeInfos);
      DeleteDC(This->hdc);      
      // FIXME Close DX fristcall and second call
      return DD_FALSE;
    }

    This->mcFourCC = This->mHALInfo.ddCaps.dwNumFourCCCodes;
    This->mpFourCC = (DWORD *) DxHeapMemAlloc(sizeof(DWORD) * This->mcFourCC);
    if (This->mpFourCC == NULL)
    {
      DxHeapMemFree(This->mpvmList);
      DxHeapMemFree(This->mpModeInfos);
      DeleteDC(This->hdc);      
      // FIXME Close DX fristcall and second call
      return DD_FALSE;
    }

    This->mcTextures = This->mD3dDriverData.dwNumTextureFormats;
    This->mpTextures = (DDSURFACEDESC*) DxHeapMemAlloc(sizeof(DDSURFACEDESC) * This->mcTextures);
    if (This->mpTextures == NULL)
    {      
      DxHeapMemFree( This->mpFourCC);
      DxHeapMemFree( This->mpvmList);
      DxHeapMemFree( This->mpModeInfos);
      DeleteDC(This->hdc);      
      // FIXME Close DX fristcall and second call
      return DD_FALSE;
    }

    This->mHALInfo.vmiData.pvmList = This->mpvmList;
    This->mHALInfo.lpdwFourCC = This->mpFourCC;
    This->mD3dDriverData.lpTextureFormats = This->mpTextures;

    if (!DdQueryDirectDrawObject(
                                    &This->mDDrawGlobal,
                                    &This->mHALInfo,
                                    &This->mCallbacks.HALDD,
                                    &This->mCallbacks.HALDDSurface,
                                    &This->mCallbacks.HALDDPalette, 
                                    &This->mD3dCallbacks,
                                    &This->mD3dDriverData,
                                    &This->mCallbacks.HALDDExeBuf,
                                    This->mpTextures,
                                    This->mpFourCC,
                                    This->mpvmList))
  
    {
      DxHeapMemFree(This->mpTextures);
      DxHeapMemFree(This->mpFourCC);
      DxHeapMemFree(This->mpvmList);
      DxHeapMemFree(This->mpModeInfos);
      DeleteDC(This->hdc);      
	  // FIXME Close DX fristcall and second call
      return DD_FALSE;
    }

   /*
      Copy over from HalInfo to DirectDrawGlobal
   */

  // this is wrong, cDriverName need be in ASC code not UNICODE 
  //memcpy(mDDrawGlobal.cDriverName, mDisplayAdapter, sizeof(wchar)*MAX_DRIVER_NAME);

  memcpy(&This->mDDrawGlobal.vmiData, &This->mHALInfo.vmiData,sizeof(VIDMEMINFO));
  memcpy(&This->mDDrawGlobal.ddCaps,  &This->mHALInfo.ddCaps,sizeof(DDCORECAPS));

  This->mHALInfo.dwNumModes = This->mcModeInfos;
  This->mHALInfo.lpModeInfo = This->mpModeInfos;
  This->mHALInfo.dwMonitorFrequency = This->mpModeInfos[0].wRefreshRate;

  This->mDDrawGlobal.dwMonitorFrequency = This->mHALInfo.dwMonitorFrequency;
  This->mDDrawGlobal.dwModeIndex        = This->mHALInfo.dwModeIndex;
  This->mDDrawGlobal.dwNumModes         = This->mHALInfo.dwNumModes;
  This->mDDrawGlobal.lpModeInfo         = This->mHALInfo.lpModeInfo;
  This->mDDrawGlobal.hInstance          = This->mHALInfo.hInstance;    
  
  This->mDDrawGlobal.lp16DD = &This->mDDrawGlobal;
  
  //DeleteDC(This->hdc);

   DDHAL_GETDRIVERINFODATA DriverInfo;
   memset(&DriverInfo,0, sizeof(DDHAL_GETDRIVERINFODATA));
   DriverInfo.dwSize = sizeof(DDHAL_GETDRIVERINFODATA);
   DriverInfo.dwContext = This->mDDrawGlobal.hDD; 

  /* Get the MiscellaneousCallbacks  */    
  DriverInfo.guidInfo = GUID_MiscellaneousCallbacks;
  DriverInfo.lpvData = &This->mDDrawGlobal.lpDDCBtmp->HALDDMiscellaneous;
  DriverInfo.dwExpectedSize = sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS);
  This->mHALInfo.GetDriverInfo(&DriverInfo);

  return DD_OK;
}

HRESULT WINAPI 
StartDirectDrawHel(LPDIRECTDRAW* iface)
{
	IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

	This->HELMemoryAvilable = HEL_GRAPHIC_MEMORY_MAX;

    This->mCallbacks.HELDD.dwFlags = DDHAL_CB32_DESTROYDRIVER;
    This->mCallbacks.HELDD.DestroyDriver = HelDdDestroyDriver;
    
    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_CREATESURFACE; 
    This->mCallbacks.HELDD.CreateSurface = HelDdCreateSurface;
    
    // DDHAL_CB32_
    //This->mCallbacks.HELDD.SetColorKey = HelDdSetColorKey;
   
    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_SETMODE;
    This->mCallbacks.HELDD.SetMode = HelDdSetMode;
    
    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_WAITFORVERTICALBLANK;     
    This->mCallbacks.HELDD.WaitForVerticalBlank = HelDdWaitForVerticalBlank;
        
    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_CANCREATESURFACE;
    This->mCallbacks.HELDD.CanCreateSurface = HelDdCanCreateSurface;
    
    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_CREATEPALETTE;
    This->mCallbacks.HELDD.CreatePalette = HelDdCreatePalette;
    
    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_GETSCANLINE;
    This->mCallbacks.HELDD.GetScanLine = HelDdGetScanLine;
    
    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_SETEXCLUSIVEMODE;
    This->mCallbacks.HELDD.SetExclusiveMode = HelDdSetExclusiveMode;

    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_FLIPTOGDISURFACE;
    This->mCallbacks.HELDD.FlipToGDISurface = HelDdFlipToGDISurface;
   
	return DD_OK;
}

HRESULT 
WINAPI 
Create_DirectDraw (LPGUID pGUID, 
				   LPDIRECTDRAW* pIface, 
				   REFIID id, 
				   BOOL ex)
{   
    IDirectDrawImpl* This = (IDirectDrawImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectDrawImpl));

	DX_WINDBG_trace();

	if (This == NULL) 
		return E_OUTOFMEMORY;

	ZeroMemory(This,sizeof(IDirectDrawImpl));

	This->lpVtbl = &DirectDraw7_Vtable;
	This->lpVtbl_v1 = &DDRAW_IDirectDraw_VTable;
	This->lpVtbl_v2 = &DDRAW_IDirectDraw2_VTable;
	This->lpVtbl_v4 = &DDRAW_IDirectDraw4_VTable;
	
	*pIface = (LPDIRECTDRAW)This;

	if(This->lpVtbl->QueryInterface ((LPDIRECTDRAW7)This, id, (void**)&pIface) != S_OK)
	{
		return DDERR_INVALIDPARAMS;
	}

	if (StartDirectDraw((LPDIRECTDRAW*)This) == DD_OK);
    {
		return This->lpVtbl->Initialize ((LPDIRECTDRAW7)This, pGUID);
	}

	return DDERR_INVALIDPARAMS;
}

