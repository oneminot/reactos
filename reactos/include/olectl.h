#ifndef _OLECTL_H
#define _OLECTL_H
#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <ocidl.h>
#include <olectlid.h>

#define STD_CTL_SCODE(n) MAKE_SCODE(SEVERITY_ERROR,FACILITY_CONTROL,n)
#define CTL_E_ILLEGALFUNCTIONCALL STD_CTL_SCODE(5)
#define CTL_E_OVERFLOW STD_CTL_SCODE(6)
#define CTL_E_OUTOFMEMORY STD_CTL_SCODE(7)
#define CTL_E_DIVISIONBYZERO STD_CTL_SCODE(11)
#define CTL_E_OUTOFSTRINGSPACE STD_CTL_SCODE(14)
#define CTL_E_OUTOFSTACKSPACE STD_CTL_SCODE(28)
#define CTL_E_BADFILENAMEORNUMBER STD_CTL_SCODE(52)
#define CTL_E_FILENOTFOUND STD_CTL_SCODE(53)
#define CTL_E_BADFILEMODE STD_CTL_SCODE(54)
#define CTL_E_FILEALREADYOPEN STD_CTL_SCODE(55)
#define CTL_E_DEVICEIOERROR STD_CTL_SCODE(57)
#define CTL_E_FILEALREADYEXISTS STD_CTL_SCODE(58)
#define CTL_E_BADRECORDLENGTH STD_CTL_SCODE(59)
#define CTL_E_DISKFULL STD_CTL_SCODE(61)
#define CTL_E_BADRECORDNUMBER STD_CTL_SCODE(63)
#define CTL_E_BADFILENAME STD_CTL_SCODE(64)
#define CTL_E_TOOMANYFILES STD_CTL_SCODE(67)
#define CTL_E_DEVICEUNAVAILABLE STD_CTL_SCODE(68)
#define CTL_E_PERMISSIONDENIED STD_CTL_SCODE(70)
#define CTL_E_DISKNOTREADY STD_CTL_SCODE(71)
#define CTL_E_PATHFILEACCESSERROR STD_CTL_SCODE(75)
#define CTL_E_PATHNOTFOUND STD_CTL_SCODE(76)
#define CTL_E_INVALIDPATTERNSTRING STD_CTL_SCODE(93)
#define CTL_E_INVALIDUSEOFNULL STD_CTL_SCODE(94)
#define CTL_E_INVALIDFILEFORMAT STD_CTL_SCODE(321)
#define CTL_E_INVALIDPROPERTYVALUE STD_CTL_SCODE(380)
#define CTL_E_INVALIDPROPERTYARRAYINDEX STD_CTL_SCODE(381)
#define CTL_E_SETNOTSUPPORTEDATRUNTIME STD_CTL_SCODE(382)
#define CTL_E_SETNOTSUPPORTED STD_CTL_SCODE(383)
#define CTL_E_NEEDPROPERTYARRAYINDEX STD_CTL_SCODE(385)
#define CTL_E_SETNOTPERMITTED STD_CTL_SCODE(387)
#define CTL_E_GETNOTSUPPORTEDATRUNTIME STD_CTL_SCODE(393)
#define CTL_E_GETNOTSUPPORTED STD_CTL_SCODE(394)
#define CTL_E_PROPERTYNOTFOUND STD_CTL_SCODE(422)
#define CTL_E_INVALIDCLIPBOARDFORMAT STD_CTL_SCODE(460)
#define CTL_E_INVALIDPICTURE STD_CTL_SCODE(481)
#define CTL_E_PRINTERERROR STD_CTL_SCODE(482)
#define CTL_E_CANTSAVEFILETOTEMP STD_CTL_SCODE(735)
#define CTL_E_SEARCHTEXTNOTFOUND STD_CTL_SCODE(744)
#define CTL_E_REPLACEMENTSTOOLONG STD_CTL_SCODE(746)
#define CUSTOM_CTL_SCODE(n) MAKE_SCODE(SEVERITY_ERROR,FACILITY_CONTROL,n)
#define CTL_E_CUSTOM_FIRST CUSTOM_CTL_SCODE(600)
#define CLASS_E_NOTLICENSED (CLASSFACTORY_E_FIRST+2)
#define CONNECT_E_FIRST MAKE_SCODE(SEVERITY_ERROR,FACILITY_ITF,0x200)
#define CONNECT_E_LAST MAKE_SCODE(SEVERITY_ERROR,FACILITY_ITF,0x20F)
#define CONNECT_S_FIRST MAKE_SCODE(SEVERITY_SUCCESS,FACILITY_ITF,0x200)
#define CONNECT_S_LAST MAKE_SCODE(SEVERITY_SUCCESS,FACILITY_ITF,0x20F)
#define CONNECT_E_NOCONNECTION (CONNECT_E_FIRST+0)
#define CONNECT_E_ADVISELIMIT (CONNECT_E_FIRST+1)
#define CONNECT_E_CANNOTCONNECT (CONNECT_E_FIRST+2)
#define CONNECT_E_OVERRIDDEN (CONNECT_E_FIRST+3)
#define SELFREG_E_FIRST MAKE_SCODE(SEVERITY_ERROR,FACILITY_ITF,0x200)
#define SELFREG_E_LAST MAKE_SCODE(SEVERITY_ERROR,FACILITY_ITF,0x20F)
#define SELFREG_S_FIRST MAKE_SCODE(SEVERITY_SUCCESS,FACILITY_ITF,0x200)
#define SELFREG_S_LAST MAKE_SCODE(SEVERITY_SUCCESS,FACILITY_ITF,0x20F)
#define SELFREG_E_TYPELIB (SELFREG_E_FIRST+0)
#define SELFREG_E_CLASS (SELFREG_E_FIRST+1)
#define PERPROP_E_FIRST MAKE_SCODE(SEVERITY_ERROR,FACILITY_ITF,0x200)
#define PERPROP_E_LAST MAKE_SCODE(SEVERITY_ERROR,FACILITY_ITF,0x20F)
#define PERPROP_S_FIRST MAKE_SCODE(SEVERITY_SUCCESS,FACILITY_ITF,0x200)
#define PERPROP_S_LAST MAKE_SCODE(SEVERITY_SUCCESS,FACILITY_ITF,0x20F)
#define PERPROP_E_NOPAGEAVAILABLE PERPROP_E_FIRST
#define OLEMISC_RECOMPOSEONRESIZE 0x1
#define OLEMISC_ONLYICONIC 0x2
#define OLEMISC_INSERTNOTREPLACE 0x4
#define OLEMISC_STATIC 0x8
#define OLEMISC_CANTLINKINSIDE 0x10
#define OLEMISC_CANLINKBYOLE1 0x20
#define OLEMISC_ISLINKOBJECT 0x40
#define OLEMISC_INSIDEOUT 0x80
#define OLEMISC_ACTIVATEWHENVISIBLE 0x100
#define OLEMISC_RENDERINGISDEVICEINDEPENDENT 0x200
#define OLEMISC_INVISIBLEATRUNTIME 0x400
#define OLEMISC_ALWAYSRUN 0x800
#define OLEMISC_ACTSLIKEBUTTON 0x1000
#define OLEMISC_ACTSLIKELABEL 0x2000
#define OLEMISC_NOUIACTIVATE 0x4000
#define OLEMISC_ALIGNABLE 0x8000
#define OLEMISC_SIMPLEFRAME 0x10000
#define OLEMISC_SETCLIENTSITEFIRST 0x20000
#define OLEMISC_IMEMODE 0x40000
#define OLEMISC_IGNOREACTIVATEWHENVISIBLE 0x80000
#define OLEMISC_WANTSTOMENUMERGE 0x100000
#define OLEMISC_SUPPORTSMULTILEVELUNDO 0x200000
#define OLEIVERB_PROPERTIES (-7)
#define VT_STREAMED_PROPSET 73
#define VT_STORED_PROPSET 74
#define VT_BLOB_PROPSET 75
#define VT_VERBOSE_ENUM	76
#define VT_COLOR VT_I4
#define VT_XPOS_PIXELS VT_I4
#define VT_YPOS_PIXELS VT_I4
#define VT_XSIZE_PIXELS VT_I4
#define VT_YSIZE_PIXELS VT_I4
#define VT_XPOS_HIMETRIC VT_I4
#define VT_YPOS_HIMETRIC VT_I4
#define VT_XSIZE_HIMETRIC VT_I4
#define VT_YSIZE_HIMETRIC VT_I4
#define VT_TRISTATE VT_I2
#define VT_OPTEXCLUSIVE VT_BOOL
#define VT_FONT VT_DISPATCH
#define VT_PICTURE VT_DISPATCH
#define VT_HANDLE VT_I4
#define OCM__BASE (WM_USER+0x1c00)
#define OCM_COMMAND (OCM__BASE+WM_COMMAND)
#define OCM_CTLCOLORBTN (OCM__BASE+WM_CTLCOLORBTN)
#define OCM_CTLCOLOREDIT (OCM__BASE+WM_CTLCOLOREDIT)
#define OCM_CTLCOLORDLG (OCM__BASE+WM_CTLCOLORDLG)
#define OCM_CTLCOLORLISTBOX (OCM__BASE+WM_CTLCOLORLISTBOX)
#define OCM_CTLCOLORMSGBOX (OCM__BASE+WM_CTLCOLORMSGBOX)
#define OCM_CTLCOLORSCROLLBAR (OCM__BASE+WM_CTLCOLORSCROLLBAR)
#define OCM_CTLCOLORSTATIC (OCM__BASE+WM_CTLCOLORSTATIC)
#define OCM_DRAWITEM (OCM__BASE+WM_DRAWITEM)
#define OCM_MEASUREITEM (OCM__BASE+WM_MEASUREITEM)
#define OCM_DELETEITEM (OCM__BASE+WM_DELETEITEM)
#define OCM_VKEYTOITEM (OCM__BASE+WM_VKEYTOITEM)
#define OCM_CHARTOITEM (OCM__BASE+WM_CHARTOITEM)
#define OCM_COMPAREITEM (OCM__BASE+WM_COMPAREITEM)
#define OCM_HSCROLL (OCM__BASE+WM_HSCROLL)
#define OCM_VSCROLL (OCM__BASE+WM_VSCROLL)
#define OCM_PARENTNOTIFY (OCM__BASE+WM_PARENTNOTIFY)
#define OCM_NOTIFY (OCM__BASE+WM_NOTIFY)
#define CTRLINFO_EATS_RETURN 1
#define CTRLINFO_EATS_ESCAPE 2
#define XFORMCOORDS_POSITION 1
#define XFORMCOORDS_SIZE 2
#define XFORMCOORDS_HIMETRICTOCONTAINER 4
#define XFORMCOORDS_CONTAINERTOHIMETRIC 8
#define GUIDKIND_DEFAULT_SOURCE_DISP_IID 1
#define PROPPAGESTATUS_DIRTY 1
#define PROPPAGESTATUS_VALIDATE 2
#define FONTSIZE(n) {n##0000,0}
#define PICTURE_SCALABLE 1
#define PICTURE_TRANSPARENT 2
#define PICTYPE_UNINITIALIZED (-1)
#define PICTYPE_NONE	0
#define PICTYPE_BITMAP	1
#define PICTYPE_METAFILE	2
#define PICTYPE_ICON	3
#define PICTYPE_ENHMETAFILE	4
#define DISPID_AUTOSIZE (-500)
#define DISPID_BACKCOLOR (-501)
#define DISPID_BACKSTYLE (-502)
#define DISPID_BORDERCOLOR (-503)
#define DISPID_BORDERSTYLE (-504)
#define DISPID_BORDERWIDTH (-505)
#define DISPID_DRAWMODE (-507)
#define DISPID_DRAWSTYLE (-508)
#define DISPID_DRAWWIDTH (-509)
#define DISPID_FILLCOLOR (-510)
#define DISPID_FILLSTYLE (-511)
#define DISPID_FONT (-512)
#define DISPID_FORECOLOR (-513)
#define DISPID_ENABLED (-514)
#define DISPID_HWND (-515)
#define DISPID_TABSTOP (-516)
#define DISPID_TEXT (-517)
#define DISPID_CAPTION (-518)
#define DISPID_BORDERVISIBLE (-519)
#define DISPID_APPEARANCE	(-520)
#define DISPID_MOUSEPOINTER	(-521)
#define DISPID_MOUSEICON	(-522)
#define DISPID_PICTURE	(-523)
#define DISPID_VALID	(-524)
#define DISPID_REFRESH (-550)
#define DISPID_DOCLICK (-551)
#define DISPID_ABOUTBOX (-552)
#define DISPID_CLICK (-600)
#define DISPID_DBLCLICK (-601)
#define DISPID_KEYDOWN (-602)
#define DISPID_KEYPRESS (-603)
#define DISPID_KEYUP (-604)
#define DISPID_MOUSEDOWN (-605)
#define DISPID_MOUSEMOVE (-606)
#define DISPID_MOUSEUP (-607)
#define DISPID_ERROREVENT (-608)
#define DISPID_AMBIENT_BACKCOLOR (-701)
#define DISPID_AMBIENT_DISPLAYNAME (-702)
#define DISPID_AMBIENT_FONT (-703)
#define DISPID_AMBIENT_FORECOLOR (-704)
#define DISPID_AMBIENT_LOCALEID (-705)
#define DISPID_AMBIENT_MESSAGEREFLECT (-706)
#define DISPID_AMBIENT_SCALEUNITS (-707)
#define DISPID_AMBIENT_TEXTALIGN (-708)
#define DISPID_AMBIENT_USERMODE (-709)
#define DISPID_AMBIENT_UIDEAD (-710)
#define DISPID_AMBIENT_SHOWGRABHANDLES (-711)
#define DISPID_AMBIENT_SHOWHATCHING (-712)
#define DISPID_AMBIENT_DISPLAYASDEFAULT (-713)
#define DISPID_AMBIENT_SUPPORTSMNEMONICS (-714)
#define DISPID_AMBIENT_AUTOCLIP (-715)
#define DISPID_AMBIENT_APPEARANCE	(-716)
#define DISPID_AMBIENT_CODEPAGE	(-725)
#define DISPID_AMBIENT_PALETTE	(-726)
#define DISPID_AMBIENT_CHARSET	(-727)
#define DISPID_AMBIENT_RIGHTTOLEFT	(-732)
#define DISPID_AMBIENT_TOPTOBOTTOM	(-733)

#define DISPID_FONT_NAME 0
#define DISPID_FONT_SIZE 2
#define DISPID_FONT_BOLD 3
#define DISPID_FONT_ITALIC 4
#define DISPID_FONT_UNDER 5
#define DISPID_FONT_STRIKE 6
#define DISPID_FONT_WEIGHT 7
#define DISPID_FONT_CHARSET 8
#define DISPID_FONT_CHANGED 9

/* IPicture */
#define DISPID_PICT_HANDLE 0
#define DISPID_PICT_HPAL 2
#define DISPID_PICT_TYPE 3
#define DISPID_PICT_WIDTH 4
#define DISPID_PICT_HEIGHT 5
#define DISPID_PICT_RENDER 6

typedef interface IOleControl *LPOLECONTROL;
typedef interface IOleControlSite *LPOLECONTROLSITE;
typedef interface ISimpleFrameSite *LPSIMPLEFRAMESITE;
typedef interface IPersistPropertyBag *LPPERSISTPROPERTYBAG;
typedef interface IPersistStreamInit *LPPERSISTSTREAMINIT;
typedef interface IPersistMemory *LPPERSISTMEMORY;
typedef interface IPropertyNotifySink* LPPROPERTYNOTIFYSINK;
typedef interface IProvideClassInfo *LPPROVIDECLASSINFO;
typedef interface IProvideClassInfo2 *LPPROVIDECLASSINFO2;
typedef interface IConnectionPointContainer *LPCONNECTIONPOINTCONTAINER;
typedef interface IClassFactory2 *LPCLASSFACTORY2;
typedef interface ISpecifyPropertyPages *LPSPECIFYPROPERTYPAGES;
typedef interface IPerPropertyBrowsing *LPPERPROPERTYBROWSING;
typedef interface IPropertyPage *LPPROPERTYPAGE;
typedef interface IPropertyPage2 *LPPROPERTYPAGE2;

typedef interface IPicture *LPPICTURE;
typedef interface IPictureDisp *LPPICTUREDISP;
typedef long OLE_XPOS_PIXELS;
typedef long OLE_YPOS_PIXELS;
typedef long OLE_XSIZE_PIXELS;
typedef long OLE_YSIZE_PIXELS;
typedef float OLE_XPOS_CONTAINER;
typedef float OLE_YPOS_CONTAINER;
typedef float OLE_XSIZE_CONTAINER;

typedef VARIANT_BOOL OLE_OPTEXCLUSIVE;
typedef VARIANT_BOOL OLE_CANCELBOOL;
typedef VARIANT_BOOL OLE_ENABLEDEFAULTBOOL;

#pragma pack(push,8)
typedef enum { triUnchecked,triChecked1,triGray } OLE_TRISTATE;
typedef struct tagOCPFIPARAMS {
	ULONG cbStructSize;
	HWND hWndOwner;
	int x;
	int y;
	LPCOLESTR lpszCaption;
	ULONG cObjects;
	LPUNKNOWN *lplpUnk;
	ULONG cPages;
	CLSID *lpPages;
	LCID lcid;
	DISPID dispidInitialProperty;
} OCPFIPARAMS,*LPOCPFIPARAMS;
typedef struct tagFONTDESC {
	UINT cbSizeofstruct;
	LPOLESTR lpstrName;
	CY cySize;
	SHORT sWeight;
	SHORT sCharset;
	BOOL fItalic;
	BOOL fUnderline;
	BOOL fStrikethrough;
} FONTDESC,*LPFONTDESC;
typedef struct tagPICTDESC {
	UINT cbSizeofstruct;
	UINT picType;
	_ANONYMOUS_UNION union {
		struct {
			HBITMAP hbitmap;
			HPALETTE hpal;
		} bmp;
		struct {
			HMETAFILE hmeta;
			int xExt;
			int yExt;
		} wmf;
		struct {
			HICON hicon;
		} icon;
		struct {
			HENHMETAFILE hemf;
		} emf;
	} DUMMYUNIONNAME;
} PICTDESC,*LPPICTDESC;
#pragma pack(pop)

STDAPI DllRegisterServer(void);
STDAPI DllUnregisterServer(void);
STDAPI OleCreateFontIndirect(LPFONTDESC,REFIID,PVOID*);
STDAPI OleCreatePictureIndirect(LPPICTDESC,REFIID,BOOL,PVOID*);
STDAPI OleCreatePropertyFrame(HWND,UINT,UINT,LPCOLESTR,ULONG,LPUNKNOWN*,ULONG,LPCLSID,LCID,DWORD,PVOID);
STDAPI OleCreatePropertyFrameIndirect(LPOCPFIPARAMS);
STDAPI_(HCURSOR) OleIconToCursor(HINSTANCE,HICON);
STDAPI OleLoadPicture(LPSTREAM,LONG,BOOL,REFIID,PVOID*);
STDAPI OleLoadPictureEx(LPSTREAM,LONG,BOOL,REFIID,DWORD,DWORD,DWORD,LPVOID*);
STDAPI OleLoadPicturePath(LPOLESTR,LPUNKNOWN,DWORD,OLE_COLOR,REFIID,LPVOID*);
STDAPI OleLoadPictureFile(VARIANT,LPDISPATCH*);
STDAPI OleLoadPictureFileEx(VARIANT,DWORD,DWORD,DWORD,LPDISPATCH*);
STDAPI OleSavePictureFile(LPDISPATCH,BSTR);
STDAPI OleTranslateColor(OLE_COLOR,HPALETTE,COLORREF*);
#ifdef __cplusplus
}
#endif
#endif
