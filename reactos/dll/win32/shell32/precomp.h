#ifndef _PRECOMP_H__
#define _PRECOMP_H__

#include "config.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <malloc.h>

#define COBJMACROS
#define WIN32_NO_STATUS
#define NTOS_MODE_USER

#include <windows.h>

#include <shlguid.h>
#include <shlguid_undoc.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shlobj_undoc.h>
#include <shldisp.h>
#include <commdlg.h>
#include <commctrl.h>
#include <cpl.h>
#include <objbase.h>
#include <ole2.h>
#include <ocidl.h>
#include <docobj.h>
#include <prsht.h>
//#include <initguid.h>
//#include <devguid.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <msi.h>
#include <appmgmt.h>
#include <ntquery.h>
#include <recyclebin.h>
#include <shtypes.h>
#include <ndk/rtlfuncs.h>
#include <fmifs/fmifs.h>
#include <largeint.h>
#include <sddl.h>

#include <tchar.h>
#include <strsafe.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>

#include "base/shell/explorer-new/todo.h"
#include "dlgs.h"
#include "pidl.h"
#include "debughlp.h"
#include "undocshell.h"
#include "shell32_main.h"
#include "shresdef.h"
#include "cpanel.h"
#include "enumidlist.h"
#include "shfldr.h"
#include "shellfolder.h"
#include "xdg.h"
#include "shellapi.h"

#include "shfldr_fs.h"
#include "shfldr_mycomp.h"
#include "shfldr_desktop.h"
#include "shellitem.h"
#include "shelllink.h"
#include "dragdrophelper.h"
#include "shfldr_cpanel.h"
#include "autocomplete.h"
#include "shfldr_mydocuments.h"
#include "shfldr_netplaces.h"
#include "shfldr_fonts.h"
#include "shfldr_printers.h"
#include "shfldr_admintools.h"
#include "shfldr_recyclebin.h"
#include "she_ocmenu.h"
#include "shv_item_new.h"
#include "startmenu.h"

#include "wine/debug.h"
#include "wine/unicode.h"

extern const GUID CLSID_AdminFolderShortcut;
extern const GUID CLSID_FontsFolderShortcut;
extern const GUID CLSID_StartMenu;
extern const GUID CLSID_MenuBandSite;
extern const GUID CLSID_OpenWith;
extern const GUID CLSID_UnixFolder;
extern const GUID CLSID_UnixDosFolder;
extern const GUID SHELL32_AdvtShortcutProduct;
extern const GUID SHELL32_AdvtShortcutComponent;


#endif
