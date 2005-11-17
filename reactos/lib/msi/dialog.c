/*
 * Implementation of the Microsoft Installer (msi.dll)
 *
 * Copyright 2005 Mike McCormack for CodeWeavers
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

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "wingdi.h"
#include "msi.h"
#include "msipriv.h"
#include "msidefs.h"
#include "ocidl.h"
#include "olectl.h"
#include "richedit.h"
#include "commctrl.h"

#include "wine/debug.h"
#include "wine/unicode.h"

#include "action.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);


struct msi_control_tag;
typedef struct msi_control_tag msi_control;
typedef UINT (*msi_handler)( msi_dialog *, msi_control *, WPARAM );

struct msi_control_tag
{
    struct list entry;
    HWND hwnd;
    msi_handler handler;
    LPWSTR property;
    LPWSTR value;
    HBITMAP hBitmap;
    HICON hIcon;
    LPWSTR tabnext;
    HMODULE hDll;
    WCHAR name[1];
};

typedef struct msi_font_tag
{
    struct msi_font_tag *next;
    HFONT hfont;
    WCHAR name[1];
} msi_font;

struct msi_dialog_tag
{
    MSIPACKAGE *package;
    msi_dialog_event_handler event_handler;
    BOOL finished;
    INT scale;
    DWORD attributes;
    HWND hwnd;
    LPWSTR default_font;
    msi_font *font_list;
    struct list controls;
    HWND hWndFocus;
    LPWSTR control_default;
    LPWSTR control_cancel;
    WCHAR name[1];
};

typedef UINT (*msi_dialog_control_func)( msi_dialog *dialog, MSIRECORD *rec );
struct control_handler 
{
    LPCWSTR control_type;
    msi_dialog_control_func func;
};

typedef struct
{
    msi_dialog* dialog;
    msi_control *parent;
    DWORD       attributes;
} radio_button_group_descr;

const WCHAR szMsiDialogClass[] = {
    'M','s','i','D','i','a','l','o','g','C','l','o','s','e','C','l','a','s','s',0
};
const WCHAR szMsiHiddenWindow[] = {
    'M','s','i','H','i','d','d','e','n','W','i','n','d','o','w',0 };
static const WCHAR szStatic[] = { 'S','t','a','t','i','c',0 };
static const WCHAR szButton[] = { 'B','U','T','T','O','N', 0 };
static const WCHAR szButtonData[] = { 'M','S','I','D','A','T','A',0 };
static const WCHAR szText[] = { 'T','e','x','t',0 };
static const WCHAR szPushButton[] = { 'P','u','s','h','B','u','t','t','o','n',0 };
static const WCHAR szLine[] = { 'L','i','n','e',0 };
static const WCHAR szBitmap[] = { 'B','i','t','m','a','p',0 };
static const WCHAR szCheckBox[] = { 'C','h','e','c','k','B','o','x',0 };
static const WCHAR szScrollableText[] = {
    'S','c','r','o','l','l','a','b','l','e','T','e','x','t',0 };
static const WCHAR szComboBox[] = { 'C','o','m','b','o','B','o','x',0 };
static const WCHAR szEdit[] = { 'E','d','i','t',0 };
static const WCHAR szMaskedEdit[] = { 'M','a','s','k','e','d','E','d','i','t',0 };
static const WCHAR szPathEdit[] = { 'P','a','t','h','E','d','i','t',0 };
static const WCHAR szProgressBar[] = {
     'P','r','o','g','r','e','s','s','B','a','r',0 };
static const WCHAR szRadioButtonGroup[] = { 
    'R','a','d','i','o','B','u','t','t','o','n','G','r','o','u','p',0 };
static const WCHAR szIcon[] = { 'I','c','o','n',0 };
static const WCHAR szSelectionTree[] = {
    'S','e','l','e','c','t','i','o','n','T','r','e','e',0 };

static UINT msi_dialog_checkbox_handler( msi_dialog *, msi_control *, WPARAM );
static void msi_dialog_checkbox_sync_state( msi_dialog *, msi_control * );
static UINT msi_dialog_button_handler( msi_dialog *, msi_control *, WPARAM );
static UINT msi_dialog_edit_handler( msi_dialog *, msi_control *, WPARAM );
static UINT msi_dialog_radiogroup_handler( msi_dialog *, msi_control *, WPARAM param );
static UINT msi_dialog_evaluate_control_conditions( msi_dialog *dialog );
static LRESULT WINAPI MSIRadioGroup_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


/* dialog sequencing */

#define WM_MSI_DIALOG_CREATE  (WM_USER+0x100)
#define WM_MSI_DIALOG_DESTROY (WM_USER+0x101)

static DWORD uiThreadId;
static HWND hMsiHiddenWindow;

static INT msi_dialog_scale_unit( msi_dialog *dialog, INT val )
{
    return (dialog->scale * val + 5) / 10;
}

static msi_control *msi_dialog_find_control( msi_dialog *dialog, LPCWSTR name )
{
    msi_control *control;

    if( !name )
        return NULL;
    LIST_FOR_EACH_ENTRY( control, &dialog->controls, msi_control, entry )
        if( !strcmpW( control->name, name ) ) /* FIXME: case sensitive? */
            return control;
    return NULL;
}

static msi_control *msi_dialog_find_control_by_hwnd( msi_dialog *dialog, HWND hwnd )
{
    msi_control *control;

    LIST_FOR_EACH_ENTRY( control, &dialog->controls, msi_control, entry )
        if( hwnd == control->hwnd )
            return control;
    return NULL;
}

static LPWSTR msi_get_deformatted_field( MSIPACKAGE *package, MSIRECORD *rec, int field )
{
    LPCWSTR str = MSI_RecordGetString( rec, field );
    LPWSTR ret = NULL;

    if (str)
        deformat_string( package, str, &ret );
    return ret;
}

/*
 * msi_dialog_get_style
 *
 * Extract the {\style} string from the front of the text to display and
 *  update the pointer.
 */
static LPWSTR msi_dialog_get_style( LPCWSTR p, LPCWSTR *rest )
{
    LPWSTR ret = NULL;
    LPCWSTR q, i;
    DWORD len;

    *rest = p;
    if( !p )
        return ret;
    if( *p++ != '{' )
        return ret;
    q = strchrW( p, '}' );
    if( !q )
        return ret;
    if( *p == '\\' || *p == '&' )
        p++;

    /* little bit of sanity checking to stop us getting confused with RTF */
    for( i=p; i<q; i++ )
        if( *i == '}' || *i == '\\' )
            return ret;
    
    *rest = ++q;
    len = q - p;

    ret = msi_alloc( len*sizeof(WCHAR) );
    if( !ret )
        return ret;
    memcpy( ret, p, len*sizeof(WCHAR) );
    ret[len-1] = 0;
    return ret;
}

static UINT msi_dialog_add_font( MSIRECORD *rec, LPVOID param )
{
    msi_dialog *dialog = param;
    msi_font *font;
    LPCWSTR face, name;
    LOGFONTW lf;
    INT style;
    HDC hdc;

    /* create a font and add it to the list */
    name = MSI_RecordGetString( rec, 1 );
    font = msi_alloc( sizeof *font + strlenW( name )*sizeof (WCHAR) );
    strcpyW( font->name, name );
    font->next = dialog->font_list;
    dialog->font_list = font;

    memset( &lf, 0, sizeof lf );
    face = MSI_RecordGetString( rec, 2 );
    lf.lfHeight = MSI_RecordGetInteger( rec, 3 );
    style = MSI_RecordGetInteger( rec, 5 );
    if( style & msidbTextStyleStyleBitsBold )
        lf.lfWeight = FW_BOLD;
    if( style & msidbTextStyleStyleBitsItalic )
        lf.lfItalic = TRUE;
    if( style & msidbTextStyleStyleBitsUnderline )
        lf.lfUnderline = TRUE;
    if( style & msidbTextStyleStyleBitsStrike )
        lf.lfStrikeOut = TRUE;
    lstrcpynW( lf.lfFaceName, face, LF_FACESIZE );

    /* adjust the height */
    hdc = GetDC( dialog->hwnd );
    if (hdc)
    {
        lf.lfHeight = -MulDiv(lf.lfHeight, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        ReleaseDC( dialog->hwnd, hdc );
    }

    font->hfont = CreateFontIndirectW( &lf );

    TRACE("Adding font style %s\n", debugstr_w(font->name) );

    return ERROR_SUCCESS;
}

static msi_font *msi_dialog_find_font( msi_dialog *dialog, LPCWSTR name )
{
    msi_font *font;

    for( font = dialog->font_list; font; font = font->next )
        if( !strcmpW( font->name, name ) )  /* FIXME: case sensitive? */
            break;

    return font;
}

static UINT msi_dialog_set_font( msi_dialog *dialog, HWND hwnd, LPCWSTR name )
{
    msi_font *font;

    font = msi_dialog_find_font( dialog, name );
    if( font )
        SendMessageW( hwnd, WM_SETFONT, (WPARAM) font->hfont, TRUE );
    else
        ERR("No font entry for %s\n", debugstr_w(name));
    return ERROR_SUCCESS;
}

static UINT msi_dialog_build_font_list( msi_dialog *dialog )
{
    static const WCHAR query[] = {
      'S','E','L','E','C','T',' ','*',' ',
      'F','R','O','M',' ','`','T','e','x','t','S','t','y','l','e','`',' ',0
    };
    UINT r;
    MSIQUERY *view = NULL;

    TRACE("dialog %p\n", dialog );

    r = MSI_OpenQuery( dialog->package->db, &view, query );
    if( r != ERROR_SUCCESS )
        return r;

    r = MSI_IterateRecords( view, NULL, msi_dialog_add_font, dialog );
    msiobj_release( &view->hdr );

    return r;
}

static msi_control *msi_dialog_create_window( msi_dialog *dialog,
                MSIRECORD *rec, LPCWSTR szCls, LPCWSTR name, LPCWSTR text,
                DWORD style, HWND parent )
{
    DWORD x, y, width, height;
    LPWSTR font = NULL, title_font = NULL;
    LPCWSTR title = NULL;
    msi_control *control;

    style |= WS_CHILD;

    control = msi_alloc( sizeof *control + strlenW(name)*sizeof(WCHAR) );
    strcpyW( control->name, name );
    list_add_head( &dialog->controls, &control->entry );
    control->handler = NULL;
    control->property = NULL;
    control->value = NULL;
    control->hBitmap = NULL;
    control->hIcon = NULL;
    control->hDll = NULL;
    control->tabnext = strdupW( MSI_RecordGetString( rec, 11) );

    x = MSI_RecordGetInteger( rec, 4 );
    y = MSI_RecordGetInteger( rec, 5 );
    width = MSI_RecordGetInteger( rec, 6 );
    height = MSI_RecordGetInteger( rec, 7 );

    x = msi_dialog_scale_unit( dialog, x );
    y = msi_dialog_scale_unit( dialog, y );
    width = msi_dialog_scale_unit( dialog, width );
    height = msi_dialog_scale_unit( dialog, height );

    if( text )
    {
        deformat_string( dialog->package, text, &title_font );
        font = msi_dialog_get_style( title_font, &title );
    }

    control->hwnd = CreateWindowW( szCls, title, style,
                          x, y, width, height, parent, NULL, NULL, NULL );

    TRACE("Dialog %s control %s hwnd %p\n",
           debugstr_w(dialog->name), debugstr_w(text), control->hwnd );

    msi_dialog_set_font( dialog, control->hwnd,
                         font ? font : dialog->default_font );

    msi_free( title_font );
    msi_free( font );

    return control;
}

static MSIRECORD *msi_get_binary_record( MSIDATABASE *db, LPCWSTR name )
{
    static const WCHAR query[] = {
        's','e','l','e','c','t',' ','*',' ',
        'f','r','o','m',' ','B','i','n','a','r','y',' ',
        'w','h','e','r','e',' ',
            '`','N','a','m','e','`',' ','=',' ','\'','%','s','\'',0
    };

    return MSI_QueryGetRecord( db, query, name );
}

static LPWSTR msi_create_tmp_path(void)
{
    WCHAR tmp[MAX_PATH];
    LPWSTR path = NULL;
    static const WCHAR prefix[] = { 'm','s','i',0 };
    DWORD len, r;

    r = GetTempPathW( MAX_PATH, tmp );
    if( !r )
        return path;
    len = lstrlenW( tmp ) + 20;
    path = msi_alloc( len * sizeof (WCHAR) );
    if( path )
    {
        r = GetTempFileNameW( tmp, prefix, 0, path );
        if (!r)
        {
            msi_free( path );
            path = NULL;
        }
    }
    return path;
}


static HANDLE msi_load_image( MSIDATABASE *db, LPCWSTR name, UINT type,
                              UINT cx, UINT cy, UINT flags )
{
    MSIRECORD *rec = NULL;
    HANDLE himage = NULL;
    LPWSTR tmp;
    UINT r;

    TRACE("%p %s %u %u %08x\n", db, debugstr_w(name), cx, cy, flags);

    tmp = msi_create_tmp_path();
    if( !tmp )
        return himage;

    rec = msi_get_binary_record( db, name );
    if( rec )
    {
        r = MSI_RecordStreamToFile( rec, 2, tmp );
        if( r == ERROR_SUCCESS )
        {
            himage = LoadImageW( 0, tmp, type, cx, cy, flags );
            DeleteFileW( tmp );
        }
        msiobj_release( &rec->hdr );
    }

    msi_free( tmp );
    return himage;
}

static HICON msi_load_icon( MSIDATABASE *db, LPCWSTR text, UINT attributes )
{
    DWORD cx = 0, cy = 0, flags;

    flags = LR_LOADFROMFILE | LR_DEFAULTSIZE;
    if( attributes & msidbControlAttributesFixedSize )
    {
        flags &= ~LR_DEFAULTSIZE;
        if( attributes & msidbControlAttributesIconSize16 )
        {
            cx += 16;
            cy += 16;
        }
        if( attributes & msidbControlAttributesIconSize32 )
        {
            cx += 32;
            cy += 32;
        }
        /* msidbControlAttributesIconSize48 handled by above logic */
    }
    return msi_load_image( db, text, IMAGE_ICON, cx, cy, flags );
}


/* called from the Control Event subscription code */
void msi_dialog_handle_event( msi_dialog* dialog, LPCWSTR control, 
                              LPCWSTR attribute, MSIRECORD *rec )
{
    msi_control* ctrl;
    LPCWSTR text;

    ctrl = msi_dialog_find_control( dialog, control );
    if (!ctrl)
        return;
    if( lstrcmpW(attribute, szText) )
    {
        ERR("Attribute %s\n", debugstr_w(attribute));
        return;
    }
    text = MSI_RecordGetString( rec , 1 );
    SetWindowTextW( ctrl->hwnd, text );
    msi_dialog_check_messages( NULL );
}

static void msi_dialog_map_events(msi_dialog* dialog, LPCWSTR control)
{
    static WCHAR Query[] = {
        'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ',
         '`','E','v','e','n','t','M','a','p','p','i','n','g','`',' ',
        'W','H','E','R','E',' ',
         '`','D','i','a','l','o','g','_','`',' ','=',' ','\'','%','s','\'',' ',
        'A','N','D',' ',
         '`','C','o','n','t','r','o','l','_','`',' ','=',' ','\'','%','s','\'',0
    };
    MSIRECORD *row;
    LPCWSTR event, attribute;

    row = MSI_QueryGetRecord( dialog->package->db, Query, dialog->name, control );
    if (!row)
        return;

    event = MSI_RecordGetString( row, 3 );
    attribute = MSI_RecordGetString( row, 4 );
    ControlEvent_SubscribeToEvent( dialog->package, event, control, attribute );
    msiobj_release( &row->hdr );
}

/* everything except radio buttons */
static msi_control *msi_dialog_add_control( msi_dialog *dialog,
                MSIRECORD *rec, LPCWSTR szCls, DWORD style )
{
    DWORD attributes;
    LPCWSTR text, name;

    name = MSI_RecordGetString( rec, 2 );
    attributes = MSI_RecordGetInteger( rec, 8 );
    text = MSI_RecordGetString( rec, 10 );
    if( attributes & msidbControlAttributesVisible )
        style |= WS_VISIBLE;
    if( ~attributes & msidbControlAttributesEnabled )
        style |= WS_DISABLED;

    msi_dialog_map_events(dialog, name);

    return msi_dialog_create_window( dialog, rec, szCls, name, text,
                                     style, dialog->hwnd );
}

struct msi_text_info
{
    WNDPROC oldproc;
    DWORD attributes;
};

/*
 * we don't erase our own background,
 * so we have to make sure that the parent window redraws first
 */
static void msi_text_on_settext( HWND hWnd )
{
    HWND hParent;
    RECT rc;

    hParent = GetParent( hWnd );
    GetWindowRect( hWnd, &rc );
    MapWindowPoints( NULL, hParent, (LPPOINT) &rc, 2 );
    InvalidateRect( hParent, &rc, TRUE );
}

static LRESULT WINAPI
MSIText_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct msi_text_info *info;
    LRESULT r = 0;

    TRACE("%p %04x %08x %08lx\n", hWnd, msg, wParam, lParam);

    info = GetPropW(hWnd, szButtonData);

    if( msg == WM_CTLCOLORSTATIC &&
       ( info->attributes & msidbControlAttributesTransparent ) )
    {
        SetBkMode( (HDC)wParam, TRANSPARENT );
        return (LRESULT) GetStockObject(NULL_BRUSH);
    }

    r = CallWindowProcW(info->oldproc, hWnd, msg, wParam, lParam);

    switch( msg )
    {
    case WM_SETTEXT:
        msi_text_on_settext( hWnd );
        break;
    case WM_NCDESTROY:
        msi_free( info );
        RemovePropW( hWnd, szButtonData );
        break;
    }

    return r;
}

static UINT msi_dialog_text_control( msi_dialog *dialog, MSIRECORD *rec )
{
    msi_control *control;
    struct msi_text_info *info;

    TRACE("%p %p\n", dialog, rec);

    control = msi_dialog_add_control( dialog, rec, szStatic, SS_LEFT | WS_GROUP );
    if( !control )
        return ERROR_FUNCTION_FAILED;

    info = msi_alloc( sizeof *info );
    if( !info )
        return ERROR_SUCCESS;

    info->attributes = MSI_RecordGetInteger( rec, 8 );
    if( info->attributes & msidbControlAttributesTransparent )
        SetWindowLongPtrW( control->hwnd, GWL_EXSTYLE, WS_EX_TRANSPARENT );

    info->oldproc = (WNDPROC) SetWindowLongPtrW( control->hwnd, GWLP_WNDPROC,
                                          (LONG_PTR)MSIText_WndProc );
    SetPropW( control->hwnd, szButtonData, info );

    return ERROR_SUCCESS;
}

static UINT msi_dialog_button_control( msi_dialog *dialog, MSIRECORD *rec )
{
    msi_control *control;
    UINT attributes, style;
    LPWSTR text;

    TRACE("%p %p\n", dialog, rec);

    style = WS_TABSTOP;
    attributes = MSI_RecordGetInteger( rec, 8 );
    if( attributes & msidbControlAttributesIcon )
        style |= BS_ICON;

    control = msi_dialog_add_control( dialog, rec, szButton, style );
    if( !control )
        return ERROR_FUNCTION_FAILED;

    control->handler = msi_dialog_button_handler;

    /* set the icon */
    text = msi_get_deformatted_field( dialog->package, rec, 10 );
    control->hIcon = msi_load_icon( dialog->package->db, text, attributes );
    if( attributes & msidbControlAttributesIcon )
        SendMessageW( control->hwnd, BM_SETIMAGE, IMAGE_ICON, (LPARAM) control->hIcon );
    msi_free( text );

    return ERROR_SUCCESS;
}

static LPWSTR msi_get_checkbox_value( msi_dialog *dialog, LPCWSTR prop )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ',
        'F','R','O','M',' ','`','C','h','e','c','k','B','o','x',' ','`',
        'W','H','E','R','E',' ',
        '`','P','r','o','p','e','r','t','y','`',' ','=',' ',
        '\'','%','s','\'',0
    };
    MSIRECORD *rec = NULL;
    LPWSTR ret = NULL;

    /* find if there is a value associated with the checkbox */
    rec = MSI_QueryGetRecord( dialog->package->db, query, prop );
    if (!rec)
        return ret;

    ret = msi_get_deformatted_field( dialog->package, rec, 2 );
    if( ret && !ret[0] )
    {
        msi_free( ret );
        ret = NULL;
    }
    msiobj_release( &rec->hdr );
    if (ret)
        return ret;

    ret = msi_dup_property( dialog->package, prop );
    if( ret && !ret[0] )
    {
        msi_free( ret );
        ret = NULL;
    }

    return ret;
}

static UINT msi_dialog_checkbox_control( msi_dialog *dialog, MSIRECORD *rec )
{
    msi_control *control;
    LPCWSTR prop;

    TRACE("%p %p\n", dialog, rec);

    control = msi_dialog_add_control( dialog, rec, szButton,
                                BS_CHECKBOX | BS_MULTILINE | WS_TABSTOP );
    control->handler = msi_dialog_checkbox_handler;
    prop = MSI_RecordGetString( rec, 9 );
    if( prop )
    {
        control->property = strdupW( prop );
        control->value = msi_get_checkbox_value( dialog, prop );
        TRACE("control %s value %s\n", debugstr_w(control->property),
              debugstr_w(control->value));
    }
    msi_dialog_checkbox_sync_state( dialog, control );

    return ERROR_SUCCESS;
}

static UINT msi_dialog_line_control( msi_dialog *dialog, MSIRECORD *rec )
{
    TRACE("%p %p\n", dialog, rec);

    msi_dialog_add_control( dialog, rec, szStatic, SS_ETCHEDHORZ | SS_SUNKEN );
    return ERROR_SUCCESS;
}

struct msi_streamin_info
{
    LPSTR string;
    DWORD offset;
    DWORD length;
};

static DWORD CALLBACK
msi_richedit_stream_in( DWORD_PTR arg, LPBYTE buffer, LONG count, LONG *pcb )
{
    struct msi_streamin_info *info = (struct msi_streamin_info*) arg;

    if( (count + info->offset) > info->length )
        count = info->length - info->offset;
    memcpy( buffer, &info->string[ info->offset ], count );
    *pcb = count;
    info->offset += count;

    TRACE("%ld/%ld\n", info->offset, info->length);

    return 0;
}

static UINT msi_dialog_scrolltext_control( msi_dialog *dialog, MSIRECORD *rec )
{
    static const WCHAR szRichEdit20W[] = {
    	'R','i','c','h','E','d','i','t','2','0','W',0
    };
    struct msi_streamin_info info;
    msi_control *control;
    LPCWSTR text;
    EDITSTREAM es;
    DWORD style;
    HMODULE hRichedit;

    hRichedit = LoadLibraryA("riched20");

    style = WS_BORDER | ES_MULTILINE | WS_VSCROLL |
            ES_READONLY | ES_AUTOVSCROLL | WS_TABSTOP;
    control = msi_dialog_add_control( dialog, rec, szRichEdit20W, style );
    if (!control)
        return ERROR_FUNCTION_FAILED;

    control->hDll = hRichedit;

    text = MSI_RecordGetString( rec, 10 );
    info.string = strdupWtoA( text );
    info.offset = 0;
    info.length = lstrlenA( info.string ) + 1;

    es.dwCookie = (DWORD_PTR) &info;
    es.dwError = 0;
    es.pfnCallback = msi_richedit_stream_in;

    SendMessageW( control->hwnd, EM_STREAMIN, SF_RTF, (LPARAM) &es );

    msi_free( info.string );

    return ERROR_SUCCESS;
}

static HBITMAP msi_load_picture( MSIDATABASE *db, LPCWSTR name,
                                 INT cx, INT cy, DWORD flags )
{
    HBITMAP hOleBitmap = 0, hBitmap = 0, hOldSrcBitmap, hOldDestBitmap;
    MSIRECORD *rec = NULL;
    IStream *stm = NULL;
    IPicture *pic = NULL;
    HDC srcdc, destdc;
    BITMAP bm;
    UINT r;

    rec = msi_get_binary_record( db, name );
    if( !rec )
        goto end;

    r = MSI_RecordGetIStream( rec, 2, &stm );
    msiobj_release( &rec->hdr );
    if( r != ERROR_SUCCESS )
        goto end;

    r = OleLoadPicture( stm, 0, TRUE, &IID_IPicture, (LPVOID*) &pic );
    IStream_Release( stm );
    if( FAILED( r ) )
    {
        ERR("failed to load picture\n");
        goto end;
    }

    r = IPicture_get_Handle( pic, (OLE_HANDLE*) &hOleBitmap );
    if( FAILED( r ) )
    {
        ERR("failed to get bitmap handle\n");
        goto end;
    }
 
    /* make the bitmap the desired size */
    r = GetObjectW( hOleBitmap, sizeof bm, &bm );
    if (r != sizeof bm )
    {
        ERR("failed to get bitmap size\n");
        goto end;
    }

    if (flags & LR_DEFAULTSIZE)
    {
        cx = bm.bmWidth;
        cy = bm.bmHeight;
    }

    srcdc = CreateCompatibleDC( NULL );
    hOldSrcBitmap = SelectObject( srcdc, hOleBitmap );
    destdc = CreateCompatibleDC( NULL );
    hBitmap = CreateCompatibleBitmap( srcdc, cx, cy );
    hOldDestBitmap = SelectObject( destdc, hBitmap );
    StretchBlt( destdc, 0, 0, cx, cy,
                srcdc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
    SelectObject( srcdc, hOldSrcBitmap );
    SelectObject( destdc, hOldDestBitmap );
    DeleteDC( srcdc );
    DeleteDC( destdc );

end:
    if ( pic )
        IPicture_Release( pic );
    return hBitmap;
}

static UINT msi_dialog_bitmap_control( msi_dialog *dialog, MSIRECORD *rec )
{
    UINT cx, cy, flags, style, attributes;
    msi_control *control;
    LPWSTR text;

    flags = LR_LOADFROMFILE;
    style = SS_BITMAP | SS_LEFT | WS_GROUP;

    attributes = MSI_RecordGetInteger( rec, 8 );
    if( attributes & msidbControlAttributesFixedSize )
    {
        flags |= LR_DEFAULTSIZE;
        style |= SS_CENTERIMAGE;
    }

    control = msi_dialog_add_control( dialog, rec, szStatic, style );
    cx = MSI_RecordGetInteger( rec, 6 );
    cy = MSI_RecordGetInteger( rec, 7 );
    cx = msi_dialog_scale_unit( dialog, cx );
    cy = msi_dialog_scale_unit( dialog, cy );

    text = msi_get_deformatted_field( dialog->package, rec, 10 );
    control->hBitmap = msi_load_picture( dialog->package->db, text, cx, cy, flags );
    if( control->hBitmap )
        SendMessageW( control->hwnd, STM_SETIMAGE,
                      IMAGE_BITMAP, (LPARAM) control->hBitmap );
    else
        ERR("Failed to load bitmap %s\n", debugstr_w(text));

    msi_free( text );
    
    return ERROR_SUCCESS;
}

static UINT msi_dialog_icon_control( msi_dialog *dialog, MSIRECORD *rec )
{
    msi_control *control;
    DWORD attributes;
    LPWSTR text;

    TRACE("\n");

    control = msi_dialog_add_control( dialog, rec, szStatic,
                            SS_ICON | SS_CENTERIMAGE | WS_GROUP );
            
    attributes = MSI_RecordGetInteger( rec, 8 );
    text = msi_get_deformatted_field( dialog->package, rec, 10 );
    control->hIcon = msi_load_icon( dialog->package->db, text, attributes );
    if( control->hIcon )
        SendMessageW( control->hwnd, STM_SETICON, (WPARAM) control->hIcon, 0 );
    else
        ERR("Failed to load bitmap %s\n", debugstr_w(text));
    msi_free( text );
    return ERROR_SUCCESS;
}

static UINT msi_dialog_combo_control( msi_dialog *dialog, MSIRECORD *rec )
{
    static const WCHAR szCombo[] = { 'C','O','M','B','O','B','O','X',0 };

    msi_dialog_add_control( dialog, rec, szCombo,
                            SS_BITMAP | SS_LEFT | SS_CENTERIMAGE );
    return ERROR_SUCCESS;
}

static UINT msi_dialog_edit_control( msi_dialog *dialog, MSIRECORD *rec )
{
    msi_control *control;
    LPCWSTR prop;
    LPWSTR val;

    control = msi_dialog_add_control( dialog, rec, szEdit,
                                      WS_BORDER | WS_TABSTOP );
    control->handler = msi_dialog_edit_handler;
    prop = MSI_RecordGetString( rec, 9 );
    if( prop )
        control->property = strdupW( prop );
    val = msi_dup_property( dialog->package, control->property );
    SetWindowTextW( control->hwnd, val );
    msi_free( val );
    return ERROR_SUCCESS;
}

/******************** Masked Edit ********************************************/

#define MASK_MAX_GROUPS 10

struct msi_mask_group
{
    UINT len;
    UINT ofs;
    WCHAR type;
    HWND hwnd;
};

struct msi_maskedit_info
{
    msi_dialog *dialog;
    WNDPROC oldproc;
    HWND hwnd;
    LPWSTR prop;
    UINT num_chars;
    UINT num_groups;
    struct msi_mask_group group[MASK_MAX_GROUPS];
};

static BOOL msi_mask_editable( WCHAR type )
{
    switch (type)
    {
    case '%':
    case '#':
    case '&':
    case '`':
    case '?':
        return TRUE;
    }
    return FALSE;
}

static void msi_mask_control_change( struct msi_maskedit_info *info )
{
    LPWSTR val;
    UINT i, n, r;

    val = msi_alloc( (info->num_chars+1)*sizeof(WCHAR) );
    for( i=0, n=0; i<info->num_groups; i++ )
    {
        if( (info->group[i].len + n) > info->num_chars )
        {
            ERR("can't fit control %d text into template\n",i);
            break;
        }
        if (!msi_mask_editable(info->group[i].type))
        {
            for(r=0; r<info->group[i].len; r++)
                val[n+r] = info->group[i].type;
            val[n+r] = 0;
        }
        else
        {
            r = GetWindowTextW( info->group[i].hwnd, &val[n], info->group[i].len+1 );
            if( r != info->group[i].len )
                break;
        }
        n += r;
    }

    TRACE("%d/%d controls were good\n", i, info->num_groups);

    if( i == info->num_groups )
    {
        TRACE("Set property %s to %s\n",
              debugstr_w(info->prop), debugstr_w(val) );
        CharUpperBuffW( val, info->num_chars );
        MSI_SetPropertyW( info->dialog->package, info->prop, val );
        msi_dialog_evaluate_control_conditions( info->dialog );
    }
    msi_free( val );
}

/* now move to the next control if necessary */
static VOID msi_mask_next_control( struct msi_maskedit_info *info, HWND hWnd )
{
    HWND hWndNext;
    UINT len, i;

    for( i=0; i<info->num_groups; i++ )
        if( info->group[i].hwnd == hWnd )
            break;

    /* don't move from the last control */
    if( i >= (info->num_groups-1) )
        return;

    len = SendMessageW( hWnd, WM_GETTEXTLENGTH, 0, 0 );
    if( len < info->group[i].len )
        return;

    hWndNext = GetNextDlgTabItem( GetParent( hWnd ), hWnd, FALSE );
    SetFocus( hWndNext );
}

static LRESULT WINAPI
MSIMaskedEdit_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    struct msi_maskedit_info *info;
    HRESULT r;

    TRACE("%p %04x %08x %08lx\n", hWnd, msg, wParam, lParam);

    info = GetPropW(hWnd, szButtonData);

    r = CallWindowProcW(info->oldproc, hWnd, msg, wParam, lParam);

    switch( msg )
    {
    case WM_COMMAND:
        if (HIWORD(wParam) == EN_CHANGE)
        {
            msi_mask_control_change( info );
            msi_mask_next_control( info, (HWND) lParam );
        }
        break;
    case WM_NCDESTROY:
        msi_free( info->prop );
        msi_free( info );
        RemovePropW( hWnd, szButtonData );
        break;
    }

    return r;
}

/* fish the various bits of the property out and put them in the control */
static void
msi_maskedit_set_text( struct msi_maskedit_info *info, LPCWSTR text )
{
    LPCWSTR p;
    UINT i;

    p = text;
    for( i = 0; i < info->num_groups; i++ )
    {
        if( info->group[i].len < lstrlenW( p ) )
        {
            LPWSTR chunk = strdupW( p );
            chunk[ info->group[i].len ] = 0;
            SetWindowTextW( info->group[i].hwnd, chunk );
            msi_free( chunk );
        }
        else
        {
            SetWindowTextW( info->group[i].hwnd, p );
            break;
        }
        p += info->group[i].len;
    }
}

static struct msi_maskedit_info * msi_dialog_parse_groups( LPCWSTR mask )
{
    struct msi_maskedit_info * info = NULL;
    int i = 0, n = 0, total = 0;
    LPCWSTR p;

    TRACE("masked control, template %s\n", debugstr_w(mask));

    if( !mask )
        return info;

    p = strchrW(mask, '<');
    if( !p )
        return info;

    info = msi_alloc_zero( sizeof *info );
    if( !info )
        return info;

    p++;
    for( i=0; i<MASK_MAX_GROUPS; i++ )
    {
        /* stop at the end of the string */
        if( p[0] == 0 || p[0] == '>' )
            break;

        /* count the number of the same identifier */
        for( n=0; p[n] == p[0]; n++ )
            ;
        info->group[i].ofs = total;
        info->group[i].type = p[0];
        if( p[n] == '=' )
        {
            n++;
            total++; /* an extra not part of the group */
        }
        info->group[i].len = n;
        total += n;
        p += n;
    }

    TRACE("%d characters in %d groups\n", total, i );
    if( i == MASK_MAX_GROUPS )
        ERR("too many groups in PIDTemplate %s\n", debugstr_w(mask));

    info->num_chars = total;
    info->num_groups = i;

    return info;
}

static void
msi_maskedit_create_children( struct msi_maskedit_info *info, LPCWSTR font )
{
    DWORD width, height, style, wx, ww;
    RECT rect;
    HWND hwnd;
    UINT i;

    style = WS_CHILD | WS_BORDER | WS_VISIBLE | WS_TABSTOP;

    GetClientRect( info->hwnd, &rect );

    width = rect.right - rect.left;
    height = rect.bottom - rect.top;

    for( i = 0; i < info->num_groups; i++ )
    {
        if (!msi_mask_editable( info->group[i].type ))
            continue;
        wx = (info->group[i].ofs * width) / info->num_chars;
        ww = (info->group[i].len * width) / info->num_chars;

        hwnd = CreateWindowW( szEdit, NULL, style, wx, 0, ww, height,
                              info->hwnd, NULL, NULL, NULL );
        if( !hwnd )
        {
            ERR("failed to create mask edit sub window\n");
            break;
        }

        SendMessageW( hwnd, EM_LIMITTEXT, info->group[i].len, 0 );

        msi_dialog_set_font( info->dialog, hwnd,
                             font?font:info->dialog->default_font );
        info->group[i].hwnd = hwnd;
    }
}

/*
 * office 2003 uses "73931<````=````=````=````=`````>@@@@@"
 * delphi 7 uses "<????-??????-??????-????>" and "<???-???>"
 */
static UINT msi_dialog_maskedit_control( msi_dialog *dialog, MSIRECORD *rec )
{
    LPWSTR font_mask, val = NULL, font;
    struct msi_maskedit_info *info = NULL;
    UINT ret = ERROR_SUCCESS;
    msi_control *control;
    LPCWSTR prop, mask;

    TRACE("\n");

    font_mask = msi_get_deformatted_field( dialog->package, rec, 10 );
    font = msi_dialog_get_style( font_mask, &mask );
    if( !mask )
    {
        ERR("mask template is empty\n");
        goto end;
    }

    info = msi_dialog_parse_groups( mask );
    if( !info )
    {
        ERR("template %s is invalid\n", debugstr_w(mask));
        goto end;
    }

    info->dialog = dialog;

    control = msi_dialog_add_control( dialog, rec, szStatic,
                   SS_OWNERDRAW | WS_GROUP | WS_VISIBLE );
    if( !control )
    {
        ERR("Failed to create maskedit container\n");
        ret = ERROR_FUNCTION_FAILED;
        goto end;
    }
    SetWindowLongPtrW( control->hwnd, GWL_EXSTYLE, WS_EX_CONTROLPARENT );

    info->hwnd = control->hwnd;

    /* subclass the static control */
    info->oldproc = (WNDPROC) SetWindowLongPtrW( info->hwnd, GWLP_WNDPROC,
                                          (LONG_PTR)MSIMaskedEdit_WndProc );
    SetPropW( control->hwnd, szButtonData, info );

    prop = MSI_RecordGetString( rec, 9 );
    if( prop )
        info->prop = strdupW( prop );

    msi_maskedit_create_children( info, font );

    if( prop )
    {
        val = msi_dup_property( dialog->package, prop );
        if( val )
        {
            msi_maskedit_set_text( info, val );
            msi_free( val );
        }
    }

end:
    if( ret != ERROR_SUCCESS )
        msi_free( info );
    msi_free( font_mask );
    msi_free( font );
    return ret;
}

/******************** Progress Bar *****************************************/

static UINT msi_dialog_progress_bar( msi_dialog *dialog, MSIRECORD *rec )
{
    msi_dialog_add_control( dialog, rec, PROGRESS_CLASSW, WS_VISIBLE );
    return ERROR_SUCCESS;
}

/******************** Path Edit ********************************************/

static UINT msi_dialog_pathedit_control( msi_dialog *dialog, MSIRECORD *rec )
{
    FIXME("not implemented properly\n");
    return msi_dialog_edit_control( dialog, rec );
}

/* radio buttons are a bit different from normal controls */
static UINT msi_dialog_create_radiobutton( MSIRECORD *rec, LPVOID param )
{
    radio_button_group_descr *group = (radio_button_group_descr *)param;
    msi_dialog *dialog = group->dialog;
    msi_control *control;
    LPCWSTR prop, text, name;
    DWORD style, attributes = group->attributes;

    style = WS_CHILD | BS_AUTORADIOBUTTON | BS_MULTILINE | WS_TABSTOP;
    name = MSI_RecordGetString( rec, 3 );
    text = MSI_RecordGetString( rec, 8 );
    if( attributes & 1 )
        style |= WS_VISIBLE;
    if( ~attributes & 2 )
        style |= WS_DISABLED;

    control = msi_dialog_create_window( dialog, rec, szButton, name, text,
                                        style, group->parent->hwnd );
    if (!control)
        return ERROR_FUNCTION_FAILED;
    control->handler = msi_dialog_radiogroup_handler;

    prop = MSI_RecordGetString( rec, 1 );
    if( prop )
        control->property = strdupW( prop );

    return ERROR_SUCCESS;
}

static UINT msi_dialog_radiogroup_control( msi_dialog *dialog, MSIRECORD *rec )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ',
        'F','R','O','M',' ','R','a','d','i','o','B','u','t','t','o','n',' ',
        'W','H','E','R','E',' ',
           '`','P','r','o','p','e','r','t','y','`',' ','=',' ','\'','%','s','\'',0};
    UINT r;
    LPCWSTR prop;
    msi_control *control;
    MSIQUERY *view = NULL;
    radio_button_group_descr group;
    MSIPACKAGE *package = dialog->package;
    WNDPROC oldproc;

    prop = MSI_RecordGetString( rec, 9 );

    TRACE("%p %p %s\n", dialog, rec, debugstr_w( prop ));

    /* Create parent group box to hold radio buttons */
    control = msi_dialog_add_control( dialog, rec, szButton, BS_OWNERDRAW|WS_GROUP );
    if( !control )
        return ERROR_FUNCTION_FAILED;

    oldproc = (WNDPROC) SetWindowLongPtrW( control->hwnd, GWLP_WNDPROC,
                                           (LONG_PTR)MSIRadioGroup_WndProc );
    SetPropW(control->hwnd, szButtonData, oldproc);
    SetWindowLongPtrW( control->hwnd, GWL_EXSTYLE, WS_EX_CONTROLPARENT );

    if( prop )
        control->property = strdupW( prop );

    /* query the Radio Button table for all control in this group */
    r = MSI_OpenQuery( package->db, &view, query, prop );
    if( r != ERROR_SUCCESS )
    {
        ERR("query failed for dialog %s radio group %s\n", 
            debugstr_w(dialog->name), debugstr_w(prop));
        return ERROR_INVALID_PARAMETER;
    }

    group.dialog = dialog;
    group.parent = control;
    group.attributes = MSI_RecordGetInteger( rec, 8 );

    r = MSI_IterateRecords( view, 0, msi_dialog_create_radiobutton, &group );
    msiobj_release( &view->hdr );

    return r;
}

/******************** Selection Tree ***************************************/

static void
msi_dialog_tv_add_child_features( MSIPACKAGE *package, HWND hwnd,
                                  LPCWSTR parent, HTREEITEM hParent )
{
    MSIFEATURE *feature;
    TVINSERTSTRUCTW tvis;
    HTREEITEM hitem;

    LIST_FOR_EACH_ENTRY( feature, &package->features, MSIFEATURE, entry )
    {
        if ( lstrcmpW( parent, feature->Feature_Parent ) )
            continue;

        if ( !feature->Title )
            continue;

        memset( &tvis, 0, sizeof tvis );
        tvis.hParent = hParent;
        tvis.hInsertAfter = TVI_SORT;
        if (feature->Title)
        {
            tvis.u.item.mask = TVIF_TEXT;
            tvis.u.item.pszText = feature->Title;
        }
        tvis.u.item.lParam = (LPARAM) feature;
        hitem = (HTREEITEM) SendMessageW( hwnd, TVM_INSERTITEMW, 0, (LPARAM) &tvis );
        if (!hitem)
            continue;

        msi_dialog_tv_add_child_features( package, hwnd,
                                          feature->Feature, hitem );
    }
}

static UINT msi_dialog_selection_tree( msi_dialog *dialog, MSIRECORD *rec )
{
    msi_control *control;
    LPCWSTR prop;
    LPWSTR val;
    MSIPACKAGE *package = dialog->package;

    prop = MSI_RecordGetString( rec, 9 );
    val = msi_dup_property( package, prop );
    control = msi_dialog_add_control( dialog, rec, WC_TREEVIEWW,
                                      TVS_HASBUTTONS | WS_GROUP | WS_VSCROLL );
    if (!control)
        return ERROR_FUNCTION_FAILED;

    msi_dialog_tv_add_child_features( package, control->hwnd, NULL, NULL );

    msi_free( val );

    return ERROR_SUCCESS;
}

struct control_handler msi_dialog_handler[] =
{
    { szText, msi_dialog_text_control },
    { szPushButton, msi_dialog_button_control },
    { szLine, msi_dialog_line_control },
    { szBitmap, msi_dialog_bitmap_control },
    { szCheckBox, msi_dialog_checkbox_control },
    { szScrollableText, msi_dialog_scrolltext_control },
    { szComboBox, msi_dialog_combo_control },
    { szEdit, msi_dialog_edit_control },
    { szMaskedEdit, msi_dialog_maskedit_control },
    { szPathEdit, msi_dialog_pathedit_control },
    { szProgressBar, msi_dialog_progress_bar },
    { szRadioButtonGroup, msi_dialog_radiogroup_control },
    { szIcon, msi_dialog_icon_control },
    { szSelectionTree, msi_dialog_selection_tree },
};

#define NUM_CONTROL_TYPES (sizeof msi_dialog_handler/sizeof msi_dialog_handler[0])

static UINT msi_dialog_create_controls( MSIRECORD *rec, LPVOID param )
{
    msi_dialog *dialog = param;
    LPCWSTR control_type;
    UINT i;

    /* find and call the function that can create this type of control */
    control_type = MSI_RecordGetString( rec, 3 );
    for( i=0; i<NUM_CONTROL_TYPES; i++ )
        if (!strcmpiW( msi_dialog_handler[i].control_type, control_type ))
            break;
    if( i != NUM_CONTROL_TYPES )
        msi_dialog_handler[i].func( dialog, rec );
    else
        ERR("no handler for element type %s\n", debugstr_w(control_type));

    return ERROR_SUCCESS;
}

static UINT msi_dialog_fill_controls( msi_dialog *dialog )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ',
        'F','R','O','M',' ','C','o','n','t','r','o','l',' ',
        'W','H','E','R','E',' ',
           '`','D','i','a','l','o','g','_','`',' ','=',' ','\'','%','s','\'',0};
    UINT r;
    MSIQUERY *view = NULL;
    MSIPACKAGE *package = dialog->package;

    TRACE("%p %s\n", dialog, debugstr_w(dialog->name) );

    /* query the Control table for all the elements of the control */
    r = MSI_OpenQuery( package->db, &view, query, dialog->name );
    if( r != ERROR_SUCCESS )
    {
        ERR("query failed for dialog %s\n", debugstr_w(dialog->name));
        return ERROR_INVALID_PARAMETER;
    }

    r = MSI_IterateRecords( view, 0, msi_dialog_create_controls, dialog );
    msiobj_release( &view->hdr );

    return r;
}

static UINT msi_dialog_set_control_condition( MSIRECORD *rec, LPVOID param )
{
    static const WCHAR szHide[] = { 'H','i','d','e',0 };
    static const WCHAR szShow[] = { 'S','h','o','w',0 };
    static const WCHAR szDisable[] = { 'D','i','s','a','b','l','e',0 };
    static const WCHAR szEnable[] = { 'E','n','a','b','l','e',0 };
    msi_dialog *dialog = param;
    msi_control *control;
    LPCWSTR name, action, condition;
    UINT r;

    name = MSI_RecordGetString( rec, 2 );
    action = MSI_RecordGetString( rec, 3 );
    condition = MSI_RecordGetString( rec, 4 );
    r = MSI_EvaluateConditionW( dialog->package, condition );
    control = msi_dialog_find_control( dialog, name );
    if( r == MSICONDITION_TRUE && control )
    {
        TRACE("%s control %s\n", debugstr_w(action), debugstr_w(name));

        /* FIXME: case sensitive? */
        if(!lstrcmpW(action, szHide))
            ShowWindow(control->hwnd, SW_HIDE);
        else if(!strcmpW(action, szShow))
            ShowWindow(control->hwnd, SW_SHOW);
        else if(!strcmpW(action, szDisable))
            EnableWindow(control->hwnd, FALSE);
        else if(!strcmpW(action, szEnable))
            EnableWindow(control->hwnd, TRUE);
        else
            FIXME("Unhandled action %s\n", debugstr_w(action));
    }

    return ERROR_SUCCESS;
}

static UINT msi_dialog_evaluate_control_conditions( msi_dialog *dialog )
{
    static const WCHAR query[] = {
      'S','E','L','E','C','T',' ','*',' ',
      'F','R','O','M',' ',
        'C','o','n','t','r','o','l','C','o','n','d','i','t','i','o','n',' ',
      'W','H','E','R','E',' ',
        '`','D','i','a','l','o','g','_','`',' ','=',' ','\'','%','s','\'',0
    };
    UINT r;
    MSIQUERY *view = NULL;
    MSIPACKAGE *package = dialog->package;

    TRACE("%p %s\n", dialog, debugstr_w(dialog->name) );

    /* query the Control table for all the elements of the control */
    r = MSI_OpenQuery( package->db, &view, query, dialog->name );
    if( r != ERROR_SUCCESS )
    {
        ERR("query failed for dialog %s\n", debugstr_w(dialog->name));
        return ERROR_INVALID_PARAMETER;
    }

    r = MSI_IterateRecords( view, 0, msi_dialog_set_control_condition, dialog );
    msiobj_release( &view->hdr );

    return r;
}

/* figure out the height of 10 point MS Sans Serif */
static INT msi_dialog_get_sans_serif_height( HWND hwnd )
{
    static const WCHAR szSansSerif[] = {
        'M','S',' ','S','a','n','s',' ','S','e','r','i','f',0 };
    LOGFONTW lf;
    TEXTMETRICW tm;
    BOOL r;
    LONG height = 0;
    HFONT hFont, hOldFont;
    HDC hdc;

    hdc = GetDC( hwnd );
    if (hdc)
    {
        memset( &lf, 0, sizeof lf );
        lf.lfHeight = MulDiv(10, GetDeviceCaps(hdc, LOGPIXELSY), 72);
        strcpyW( lf.lfFaceName, szSansSerif );
        hFont = CreateFontIndirectW(&lf);
        if (hFont)
        {
            hOldFont = SelectObject( hdc, hFont );
            r = GetTextMetricsW( hdc, &tm );
            if (r)
                height = tm.tmHeight;
            SelectObject( hdc, hOldFont );
            DeleteObject( hFont );
        }
        ReleaseDC( hwnd, hdc );
    }
    return height;
}

/* fetch the associated record from the Dialog table */
static MSIRECORD *msi_get_dialog_record( msi_dialog *dialog )
{
    static const WCHAR query[] = {
        'S','E','L','E','C','T',' ','*',' ',
        'F','R','O','M',' ','D','i','a','l','o','g',' ',
        'W','H','E','R','E',' ',
           '`','D','i','a','l','o','g','`',' ','=',' ','\'','%','s','\'',0};
    MSIPACKAGE *package = dialog->package;
    MSIRECORD *rec = NULL;

    TRACE("%p %s\n", dialog, debugstr_w(dialog->name) );

    rec = MSI_QueryGetRecord( package->db, query, dialog->name );
    if( !rec )
        ERR("query failed for dialog %s\n", debugstr_w(dialog->name));

    return rec;
}

static void msi_dialog_adjust_dialog_size( msi_dialog *dialog, LPSIZE sz )
{
    RECT rect;
    LONG style;

    /* turn the client size into the window rectangle */
    rect.left = 0;
    rect.top = 0;
    rect.right = msi_dialog_scale_unit( dialog, sz->cx );
    rect.bottom = msi_dialog_scale_unit( dialog, sz->cy );
    style = GetWindowLongPtrW( dialog->hwnd, GWL_STYLE );
    AdjustWindowRect( &rect, style, FALSE );
    sz->cx = rect.right - rect.left;
    sz->cy = rect.bottom - rect.top;
}

static BOOL msi_control_set_next( msi_control *control, msi_control *next )
{
    return SetWindowPos( next->hwnd, control->hwnd, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOREDRAW |
                         SWP_NOREPOSITION | SWP_NOSENDCHANGING | SWP_NOSIZE );
}

static UINT msi_dialog_set_tab_order( msi_dialog *dialog )
{
    msi_control *control, *tab_next;

    LIST_FOR_EACH_ENTRY( control, &dialog->controls, msi_control, entry )
    {
        tab_next = msi_dialog_find_control( dialog, control->tabnext );
        if( !tab_next )
            continue;
        msi_control_set_next( control, tab_next );
    }

    return ERROR_SUCCESS;
}

static void msi_dialog_set_first_control( msi_dialog* dialog, LPCWSTR name )
{
    msi_control *control;

    control = msi_dialog_find_control( dialog, name );
    if( control )
        dialog->hWndFocus = control->hwnd;
    else
        dialog->hWndFocus = NULL;
}

static LRESULT msi_dialog_oncreate( HWND hwnd, LPCREATESTRUCTW cs )
{
    static const WCHAR df[] = {
        'D','e','f','a','u','l','t','U','I','F','o','n','t',0 };
    msi_dialog *dialog = (msi_dialog*) cs->lpCreateParams;
    MSIRECORD *rec = NULL;
    LPWSTR title = NULL;
    SIZE size;

    TRACE("%p %p\n", dialog, dialog->package);

    dialog->hwnd = hwnd;
    SetWindowLongPtrW( hwnd, GWLP_USERDATA, (LONG_PTR) dialog );

    rec = msi_get_dialog_record( dialog );
    if( !rec )
    {
        TRACE("No record found for dialog %s\n", debugstr_w(dialog->name));
        return -1;
    }

    dialog->scale = msi_dialog_get_sans_serif_height(dialog->hwnd);

    size.cx = MSI_RecordGetInteger( rec, 4 );
    size.cy = MSI_RecordGetInteger( rec, 5 );
    msi_dialog_adjust_dialog_size( dialog, &size );

    dialog->attributes = MSI_RecordGetInteger( rec, 6 );

    dialog->default_font = msi_dup_property( dialog->package, df );

    title = msi_get_deformatted_field( dialog->package, rec, 7 );
    SetWindowTextW( hwnd, title );
    msi_free( title );

    SetWindowPos( hwnd, 0, 0, 0, size.cx, size.cy,
                  SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW );


    msi_dialog_build_font_list( dialog );
    msi_dialog_fill_controls( dialog );
    msi_dialog_evaluate_control_conditions( dialog );
    msi_dialog_set_tab_order( dialog );
    msi_dialog_set_first_control( dialog, MSI_RecordGetString( rec, 8 ) );
    msiobj_release( &rec->hdr );

    return 0;
}

static UINT msi_dialog_send_event( msi_dialog *dialog, LPCWSTR event, LPCWSTR arg )
{
    LPWSTR event_fmt = NULL, arg_fmt = NULL;

    TRACE("Sending control event %s %s\n", debugstr_w(event), debugstr_w(arg));

    deformat_string( dialog->package, event, &event_fmt );
    deformat_string( dialog->package, arg, &arg_fmt );

    dialog->event_handler( dialog->package, event_fmt, arg_fmt, dialog );

    msi_free( event_fmt );
    msi_free( arg_fmt );

    return ERROR_SUCCESS;
}

static UINT msi_dialog_set_property( msi_dialog *dialog, LPCWSTR event, LPCWSTR arg )
{
    static const WCHAR szNullArg[] = { '{','}',0 };
    LPWSTR p, prop, arg_fmt = NULL;
    UINT len;

    len = strlenW(event);
    prop = msi_alloc( len*sizeof(WCHAR));
    strcpyW( prop, &event[1] );
    p = strchrW( prop, ']' );
    if( p && p[1] == 0 )
    {
        *p = 0;
        if( strcmpW( szNullArg, arg ) )
            deformat_string( dialog->package, arg, &arg_fmt );
        MSI_SetPropertyW( dialog->package, prop, arg_fmt );
        msi_free( arg_fmt );
    }
    else
        ERR("Badly formatted property string - what happens?\n");
    msi_free( prop );
    return ERROR_SUCCESS;
}

static UINT msi_dialog_control_event( MSIRECORD *rec, LPVOID param )
{
    msi_dialog *dialog = param;
    LPCWSTR condition, event, arg;
    UINT r;

    condition = MSI_RecordGetString( rec, 5 );
    r = MSI_EvaluateConditionW( dialog->package, condition );
    if( r == MSICONDITION_TRUE )
    {
        event = MSI_RecordGetString( rec, 3 );
        arg = MSI_RecordGetString( rec, 4 );
        if( event[0] == '[' )
            msi_dialog_set_property( dialog, event, arg );
        else
            msi_dialog_send_event( dialog, event, arg );
    }

    return ERROR_SUCCESS;
}

static UINT msi_dialog_button_handler( msi_dialog *dialog,
                                       msi_control *control, WPARAM param )
{
    static const WCHAR query[] = {
      'S','E','L','E','C','T',' ','*',' ',
      'F','R','O','M',' ','C','o','n','t','r','o','l','E','v','e','n','t',' ',
      'W','H','E','R','E',' ',
         '`','D','i','a','l','o','g','_','`',' ','=',' ','\'','%','s','\'',' ',
      'A','N','D',' ',
         '`','C','o','n','t','r','o','l','_','`',' ','=',' ','\'','%','s','\'',' ',
      'O','R','D','E','R',' ','B','Y',' ','`','O','r','d','e','r','i','n','g','`',0
    };
    MSIQUERY *view = NULL;
    UINT r;

    if( HIWORD(param) != BN_CLICKED )
        return ERROR_SUCCESS;

    r = MSI_OpenQuery( dialog->package->db, &view, query,
                       dialog->name, control->name );
    if( r != ERROR_SUCCESS )
    {
        ERR("query failed\n");
        return 0;
    }

    r = MSI_IterateRecords( view, 0, msi_dialog_control_event, dialog );
    msiobj_release( &view->hdr );

    return r;
}

static UINT msi_dialog_get_checkbox_state( msi_dialog *dialog,
                msi_control *control )
{
    WCHAR state[2] = { 0 };
    DWORD sz = 2;

    MSI_GetPropertyW( dialog->package, control->property, state, &sz );
    return state[0] ? 1 : 0;
}

static void msi_dialog_set_checkbox_state( msi_dialog *dialog,
                msi_control *control, UINT state )
{
    static const WCHAR szState[] = { '1', 0 };
    LPCWSTR val;

    /* if uncheck then the property is set to NULL */
    if (!state)
    {
        MSI_SetPropertyW( dialog->package, control->property, NULL );
        return;
    }

    /* check for a custom state */
    if (control->value && control->value[0])
        val = control->value;
    else
        val = szState;

    MSI_SetPropertyW( dialog->package, control->property, val );
}

static void msi_dialog_checkbox_sync_state( msi_dialog *dialog,
                msi_control *control )
{
    UINT state;

    state = msi_dialog_get_checkbox_state( dialog, control );
    SendMessageW( control->hwnd, BM_SETCHECK,
                  state ? BST_CHECKED : BST_UNCHECKED, 0 );
}

static UINT msi_dialog_checkbox_handler( msi_dialog *dialog,
                msi_control *control, WPARAM param )
{
    UINT state;

    if( HIWORD(param) != BN_CLICKED )
        return ERROR_SUCCESS;

    TRACE("clicked checkbox %s, set %s\n", debugstr_w(control->name),
          debugstr_w(control->property));

    state = msi_dialog_get_checkbox_state( dialog, control );
    state = state ? 0 : 1;
    msi_dialog_set_checkbox_state( dialog, control, state );
    msi_dialog_checkbox_sync_state( dialog, control );

    return msi_dialog_button_handler( dialog, control, param );
}

static UINT msi_dialog_edit_handler( msi_dialog *dialog,
                msi_control *control, WPARAM param )
{
    UINT sz, r;
    LPWSTR buf;

    if( HIWORD(param) != EN_CHANGE )
        return ERROR_SUCCESS;

    TRACE("edit %s contents changed, set %s\n", debugstr_w(control->name),
          debugstr_w(control->property));

    sz = 0x20;
    buf = msi_alloc( sz*sizeof(WCHAR) );
    while( buf )
    {
        r = GetWindowTextW( control->hwnd, buf, sz );
        if( r < (sz-1) )
            break;
        sz *= 2;
        buf = msi_realloc( buf, sz*sizeof(WCHAR) );
    }

    MSI_SetPropertyW( dialog->package, control->property, buf );

    msi_free( buf );

    return ERROR_SUCCESS;
}

static UINT msi_dialog_radiogroup_handler( msi_dialog *dialog,
                msi_control *control, WPARAM param )
{
    if( HIWORD(param) != BN_CLICKED )
        return ERROR_SUCCESS;

    TRACE("clicked radio button %s, set %s\n", debugstr_w(control->name),
          debugstr_w(control->property));

    MSI_SetPropertyW( dialog->package, control->property, control->name );

    return msi_dialog_button_handler( dialog, control, param );
}

static LRESULT msi_dialog_oncommand( msi_dialog *dialog, WPARAM param, HWND hwnd )
{
    msi_control *control = NULL;

    TRACE("%p %p %08x\n", dialog, hwnd, param);

    switch (param)
    {
    case 1: /* enter */
        control = msi_dialog_find_control( dialog, dialog->control_default );
        break;
    case 2: /* escape */
        control = msi_dialog_find_control( dialog, dialog->control_cancel );
        break;
    default: 
        control = msi_dialog_find_control_by_hwnd( dialog, hwnd );
    }

    if( control )
    {
        if( control->handler )
        {
            control->handler( dialog, control, param );
            msi_dialog_evaluate_control_conditions( dialog );
        }
    }
    else
        ERR("button click from nowhere %p %d %p\n", dialog, param, hwnd);
    return 0;
}

static void msi_dialog_setfocus( msi_dialog *dialog )
{
    HWND hwnd = dialog->hWndFocus;

    hwnd = GetNextDlgTabItem( dialog->hwnd, hwnd, TRUE);
    hwnd = GetNextDlgTabItem( dialog->hwnd, hwnd, FALSE);
    SetFocus( hwnd );
    dialog->hWndFocus = hwnd;
}

static LRESULT WINAPI MSIDialog_WndProc( HWND hwnd, UINT msg,
                WPARAM wParam, LPARAM lParam )
{
    msi_dialog *dialog = (LPVOID) GetWindowLongPtrW( hwnd, GWLP_USERDATA );

    TRACE("0x%04x\n", msg);

    switch (msg)
    {
    case WM_CREATE:
        return msi_dialog_oncreate( hwnd, (LPCREATESTRUCTW)lParam );

    case WM_COMMAND:
        return msi_dialog_oncommand( dialog, wParam, (HWND)lParam );

    case WM_ACTIVATE:
        if( LOWORD(wParam) == WA_INACTIVE )
            dialog->hWndFocus = GetFocus();
        else
            msi_dialog_setfocus( dialog );
        return 0;

    case WM_SETFOCUS:
        msi_dialog_setfocus( dialog );
        return 0;

    /* bounce back to our subclassed static control */
    case WM_CTLCOLORSTATIC:
        return SendMessageW( (HWND) lParam, WM_CTLCOLORSTATIC, wParam, lParam );

    case WM_DESTROY:
        dialog->hwnd = NULL;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static LRESULT WINAPI MSIRadioGroup_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldproc = (WNDPROC) GetPropW(hWnd, szButtonData);

    TRACE("hWnd %p msg %04x wParam 0x%08x lParam 0x%08lx\n", hWnd, msg, wParam, lParam);

    if (msg == WM_COMMAND) /* Forward notifications to dialog */
        SendMessageW(GetParent(hWnd), msg, wParam, lParam);

    return CallWindowProcW(oldproc, hWnd, msg, wParam, lParam);
}

static LRESULT WINAPI MSIHiddenWindowProc( HWND hwnd, UINT msg,
                WPARAM wParam, LPARAM lParam )
{
    msi_dialog *dialog = (msi_dialog*) lParam;

    TRACE("%d %p\n", msg, dialog);

    switch (msg)
    {
    case WM_MSI_DIALOG_CREATE:
        return msi_dialog_run_message_loop( dialog );
    case WM_MSI_DIALOG_DESTROY:
        msi_dialog_destroy( dialog );
        return 0;
    }
    return DefWindowProcW( hwnd, msg, wParam, lParam );
}

/* functions that interface to other modules within MSI */

msi_dialog *msi_dialog_create( MSIPACKAGE* package, LPCWSTR szDialogName,
                                msi_dialog_event_handler event_handler )
{
    MSIRECORD *rec = NULL;
    msi_dialog *dialog;

    TRACE("%p %s\n", package, debugstr_w(szDialogName));

    /* allocate the structure for the dialog to use */
    dialog = msi_alloc_zero( sizeof *dialog + sizeof(WCHAR)*strlenW(szDialogName) );
    if( !dialog )
        return NULL;
    strcpyW( dialog->name, szDialogName );
    msiobj_addref( &package->hdr );
    dialog->package = package;
    dialog->event_handler = event_handler;
    dialog->finished = 0;
    list_init( &dialog->controls );

    /* verify that the dialog exists */
    rec = msi_get_dialog_record( dialog );
    if( !rec )
    {
        msiobj_release( &package->hdr );
        msi_free( dialog );
        return NULL;
    }
    dialog->attributes = MSI_RecordGetInteger( rec, 6 );
    dialog->control_default = strdupW( MSI_RecordGetString( rec, 9 ) );
    dialog->control_cancel = strdupW( MSI_RecordGetString( rec, 10 ) );
    msiobj_release( &rec->hdr );

    return dialog;
}

static void msi_process_pending_messages( HWND hdlg )
{
    MSG msg;

    while( PeekMessageW( &msg, 0, 0, 0, PM_REMOVE ) )
    {
        if( hdlg && IsDialogMessageW( hdlg, &msg ))
            continue;
        TranslateMessage( &msg );
        DispatchMessageW( &msg );
    }
}

void msi_dialog_end_dialog( msi_dialog *dialog )
{
    TRACE("%p\n", dialog);
    dialog->finished = 1;
    PostMessageW(dialog->hwnd, WM_NULL, 0, 0);
}

void msi_dialog_check_messages( HANDLE handle )
{
    DWORD r;

    /* in threads other than the UI thread, block */
    if( uiThreadId != GetCurrentThreadId() )
    {
        if( handle )
            WaitForSingleObject( handle, INFINITE );
        return;
    }

    /* there's two choices for the UI thread */
    while (1)
    {
        msi_process_pending_messages( NULL );

        if( !handle )
            break;

        /*
         * block here until somebody creates a new dialog or
         * the handle we're waiting on becomes ready
         */
        r = MsgWaitForMultipleObjects( 1, &handle, 0, INFINITE, QS_ALLINPUT );
        if( r == WAIT_OBJECT_0 )
            break;
    }
}

UINT msi_dialog_run_message_loop( msi_dialog *dialog )
{
    HWND hwnd;

    if( !(dialog->attributes & msidbDialogAttributesVisible) )
        return ERROR_SUCCESS;

    if( uiThreadId != GetCurrentThreadId() )
        return SendMessageW( hMsiHiddenWindow, WM_MSI_DIALOG_CREATE, 0, (LPARAM) dialog );

    /* create the dialog window, don't show it yet */
    hwnd = CreateWindowW( szMsiDialogClass, dialog->name, WS_OVERLAPPEDWINDOW,
                     CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                     NULL, NULL, NULL, dialog );
    if( !hwnd )
    {
        ERR("Failed to create dialog %s\n", debugstr_w( dialog->name ));
        return ERROR_FUNCTION_FAILED;
    }

    ShowWindow( hwnd, SW_SHOW );
    /* UpdateWindow( hwnd ); - and causes the transparent static controls not to paint */

    if( dialog->attributes & msidbDialogAttributesModal )
    {
        while( !dialog->finished )
        {
            MsgWaitForMultipleObjects( 0, NULL, 0, INFINITE, QS_ALLEVENTS );
            msi_process_pending_messages( dialog->hwnd );
        }
    }
    else
        return ERROR_IO_PENDING;

    return ERROR_SUCCESS;
}

void msi_dialog_do_preview( msi_dialog *dialog )
{
    TRACE("\n");
    dialog->attributes |= msidbDialogAttributesVisible;
    dialog->attributes &= ~msidbDialogAttributesModal;
    msi_dialog_run_message_loop( dialog );
}

void msi_dialog_destroy( msi_dialog *dialog )
{
    if( uiThreadId != GetCurrentThreadId() )
    {
        SendMessageW( hMsiHiddenWindow, WM_MSI_DIALOG_DESTROY, 0, (LPARAM) dialog );
        return;
    }

    if( dialog->hwnd )
        ShowWindow( dialog->hwnd, SW_HIDE );
    
    if( dialog->hwnd )
        DestroyWindow( dialog->hwnd );

    /* destroy the list of controls */
    while( !list_empty( &dialog->controls ) )
    {
        msi_control *t = LIST_ENTRY( list_head( &dialog->controls ),
                                     msi_control, entry );
        list_remove( &t->entry );
        /* leave dialog->hwnd - destroying parent destroys child windows */
        msi_free( t->property );
        msi_free( t->value );
        if( t->hBitmap )
            DeleteObject( t->hBitmap );
        if( t->hIcon )
            DestroyIcon( t->hIcon );
        msi_free( t->tabnext );
        msi_free( t );
        if (t->hDll)
            FreeLibrary( t->hDll );
    }

    /* destroy the list of fonts */
    while( dialog->font_list )
    {
        msi_font *t = dialog->font_list;
        dialog->font_list = t->next;
        DeleteObject( t->hfont );
        msi_free( t );
    }
    msi_free( dialog->default_font );

    msi_free( dialog->control_default );
    msi_free( dialog->control_cancel );
    msiobj_release( &dialog->package->hdr );
    dialog->package = NULL;
    msi_free( dialog );
}

BOOL msi_dialog_register_class( void )
{
    WNDCLASSW cls;

    ZeroMemory( &cls, sizeof cls );
    cls.lpfnWndProc   = MSIDialog_WndProc;
    cls.hInstance     = NULL;
    cls.hIcon         = LoadIconW(0, (LPWSTR)IDI_APPLICATION);
    cls.hCursor       = LoadCursorW(0, (LPWSTR)IDC_ARROW);
    cls.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    cls.lpszMenuName  = NULL;
    cls.lpszClassName = szMsiDialogClass;

    if( !RegisterClassW( &cls ) )
        return FALSE;

    cls.lpfnWndProc   = MSIHiddenWindowProc;
    cls.lpszClassName = szMsiHiddenWindow;

    if( !RegisterClassW( &cls ) )
        return FALSE;

    uiThreadId = GetCurrentThreadId();

    hMsiHiddenWindow = CreateWindowW( szMsiHiddenWindow, NULL, WS_OVERLAPPED,
                                   0, 0, 100, 100, NULL, NULL, NULL, NULL );
    if( !hMsiHiddenWindow )
        return FALSE;

    return TRUE;
}

void msi_dialog_unregister_class( void )
{
    DestroyWindow( hMsiHiddenWindow );
    hMsiHiddenWindow = NULL;
    UnregisterClassW( szMsiDialogClass, NULL );
    UnregisterClassW( szMsiHiddenWindow, NULL );
    uiThreadId = 0;
}
