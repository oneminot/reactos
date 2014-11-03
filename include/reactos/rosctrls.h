
#pragma once

class CListView: public CWindow
{
public:

    HWND Create(HWND hWndParent, _U_RECT rect, LPCTSTR szWindowName = NULL, DWORD dwStyle = 0,
                    DWORD dwExStyle = 0, _U_MENUorID MenuOrID = 0U, LPVOID lpCreateParam = NULL)
    {
        m_hWnd = ::CreateWindowEx(dwExStyle, 
                                  WC_LISTVIEW, 
                                  szWindowName, 
                                  dwStyle,
                                  rect.m_lpRect->left, 
                                  rect.m_lpRect->top, 
                                  rect.m_lpRect->right - rect.m_lpRect->left, 
                                  rect.m_lpRect->bottom - rect.m_lpRect->top,
                                  hWndParent, 
                                  MenuOrID.m_hMenu, 
                                  _AtlBaseModule.GetModuleInstance(), 
                                  lpCreateParam);

        return m_hWnd;
    }

    void SetRedraw(BOOL redraw)
    {
        SendMessage(WM_SETREDRAW, redraw);
    }

    BOOL SetTextBkColor(COLORREF cr)
    {
        return (BOOL)SendMessage(LVM_SETTEXTBKCOLOR, 0, cr);
    }

    BOOL SetBkColor(COLORREF cr)
    {
        return (BOOL)SendMessage(LVM_SETBKCOLOR, 0, cr);
    }

    BOOL SetTextColor(COLORREF cr)
    {
        return (BOOL)SendMessage(LVM_SETTEXTCOLOR, 0, cr);
    }

    DWORD SetExtendedListViewStyle(DWORD dw, DWORD dwMask = 0)
    {
        return (DWORD)SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, dwMask, dw);
    }

    int InsertColumn(int iCol, LV_COLUMN* pcol)
    {
        return (int)SendMessage(LVM_INSERTCOLUMN, iCol, reinterpret_cast<LPARAM>(pcol));
    }

    int InsertColumn(int iCol, LPWSTR pszText, int fmt, int width = -1, int iSubItem = -1, int iImage = -1, int iOrder = -1)
    {
        LV_COLUMN column = {0};
        column.mask = LVCF_TEXT|LVCF_FMT;
        column.pszText = pszText;
        column.fmt = fmt;
        if(width != -1)
        {
            column.mask |= LVCF_WIDTH;
            column.cx = width;
        }
        if(iSubItem != -1)
        {
            column.mask |= LVCF_SUBITEM;
            column.iSubItem = iSubItem;
        }
        if(iImage != -1)
        {
            column.mask |= LVCF_IMAGE;
            column.iImage = iImage;
        }
        if(iOrder != -1)
        {
            column.mask |= LVCF_ORDER;
            column.iOrder = iOrder;
        }
        return InsertColumn(iCol, &column);
    }
    
    int GetColumnWidth(int iCol)
    {
        return (int)SendMessage(LVM_GETCOLUMNWIDTH, iCol);
    }

    HIMAGELIST SetImageList(HIMAGELIST himl, int iImageList)
    {
        return (HIMAGELIST)SendMessage(LVM_SETIMAGELIST, iImageList, reinterpret_cast<LPARAM>(himl));
    }

    int InsertItem(const LV_ITEM * pitem)
    {
        return (int)SendMessage(LVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(pitem));
    }

    BOOL DeleteItem(int i)
    {
        return (BOOL)SendMessage(LVM_DELETEITEM, i, 0);
    }
        
    BOOL GetItem(LV_ITEM* pitem)
    {
        return (BOOL)SendMessage(LVM_GETITEM, 0, reinterpret_cast<LPARAM>(pitem));
    }

    BOOL SetItem(const LV_ITEM * pitem)
    {
        return (BOOL)SendMessage(LVM_SETITEM, 0, reinterpret_cast<LPARAM>(pitem));
    }
    
    int GetItemCount()
    {
        return SendMessage(LVM_GETITEMCOUNT);
    }
    
    BOOL DeleteAllItems()
    {
        return (BOOL)SendMessage(LVM_DELETEALLITEMS);
    }

    BOOL Update(int i)
    {
        return (BOOL)SendMessage(LVM_UPDATE, i, 0);
    }

    UINT GetSelectedCount()
    {
        return (UINT)SendMessage(LVM_GETSELECTEDCOUNT);
    }

    BOOL SortItems(PFNLVCOMPARE pfnCompare, PVOID lParam)
    {
        return (BOOL)SendMessage(LVM_SORTITEMS, (WPARAM)lParam, (LPARAM) pfnCompare);
    }

    BOOL EnsureVisible(int i, BOOL fPartialOK)
    {
        return (BOOL)SendMessage(LVM_ENSUREVISIBLE, i, MAKELPARAM((fPartialOK),0));
    }

    HWND EditLabel(int i)
    {
        return (HWND)SendMessage(LVM_EDITLABEL, i, 0);
    }

    int GetSelectionMark()
    {
        return (int)SendMessage(LVM_GETSELECTIONMARK);
    }

    int GetNextItem(int i, WORD flags)
    {
        return (int)SendMessage(LVM_GETNEXTITEM, i, MAKELPARAM((flags),0));
    }

    void GetItemSpacing(SIZE& spacing, BOOL bSmallIconView = FALSE)
    {
        DWORD ret = SendMessage(LVM_GETITEMSPACING, bSmallIconView);
        spacing.cx = LOWORD(ret);
        spacing.cy = HIWORD(ret);
    }

    void SetItemState(int i, UINT state, UINT mask)
    {
        LV_ITEM item;
        item.stateMask = mask; 
        item.state = state;
        SendMessage(LVM_SETITEMSTATE, i, reinterpret_cast<LPARAM>(&item));
    }

    int HitTest(LV_HITTESTINFO * phtInfo)
    {
        return (int)SendMessage(LVM_HITTEST, 0, reinterpret_cast<LPARAM>(&phtInfo));
    }

    DWORD_PTR GetItemData(int i)
    {
        LVITEMW lvItem;
        lvItem.iItem = i;
        lvItem.mask = LVIF_PARAM;
        BOOL ret = GetItem(&lvItem);
        return (DWORD_PTR)(ret ? lvItem.lParam : NULL);
    }

    BOOL GetSelectedItem(LV_ITEM* pItem)
    {
        pItem->iItem = GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
        if (pItem->iItem == -1)
            return FALSE;
        return GetItem(pItem);
    }

};

template<typename TItemData>
class CToolbar :
    public CWindow
{
public: // Configuration methods

    HWND Create(HWND hWndParent, DWORD dwStyles = 0, DWORD dwExStyles = 0)
    {
        if (!dwStyles)
        {
            dwStyles = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN;
        }

        if (!dwExStyles)
        {
            dwExStyles = WS_EX_TOOLWINDOW;
        }

        m_hWnd = CreateWindowEx(dwExStyles,
                                TOOLBARCLASSNAME,
                                NULL,
                                dwStyles,
                                0,
                                0,
                                0,
                                0,
                                hWndParent,
                                NULL,
                                _AtlBaseModule.GetModuleInstance(),
                                NULL);

        if (!m_hWnd)
            return NULL;

        /* Identify the version we're using */
        SetButtonStructSize();

        return m_hWnd;
    }

    DWORD SetButtonStructSize()
    {
        return SendMessageW(TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    }

public: // Button list management methods
    int GetButtonCount()
    {
        return SendMessageW(TB_BUTTONCOUNT);
    }

    DWORD GetButton(int index, TBBUTTON * btn)
    {
        return SendMessageW(TB_GETBUTTON, index, (LPARAM) btn);
    }

    DWORD AddButton(TBBUTTON * btn)
    {
        return SendMessageW(TB_ADDBUTTONS, 1, (LPARAM) btn);
    }

    DWORD AddButtons(int count, TBBUTTON * buttons)
    {
        return SendMessageW(TB_ADDBUTTONS, count, (LPARAM) buttons);
    }

    DWORD InsertButton(int insertAt, TBBUTTON * btn)
    {
        return SendMessageW(TB_INSERTBUTTON, insertAt, (LPARAM) btn);
    }

    DWORD MoveButton(int oldIndex, int newIndex)
    {
        return SendMessageW(TB_MOVEBUTTON, oldIndex, newIndex);
    }

    DWORD DeleteButton(int index)
    {
        return SendMessageW(TB_DELETEBUTTON, index, 0);
    }

    DWORD GetButtonInfo(int cmdId, TBBUTTONINFO * info)
    {
        return SendMessageW(TB_GETBUTTONINFO, cmdId, (LPARAM) info);
    }

    DWORD SetButtonInfo(int cmdId, TBBUTTONINFO * info)
    {
        return SendMessageW(TB_SETBUTTONINFO, cmdId, (LPARAM) info);
    }

public: // Layout management methods
    DWORD SetButtonSize(int w, int h)
    {
        return SendMessageW(TB_SETBUTTONSIZE, 0, MAKELONG(w, h));
    }

    DWORD AutoSize()
    {
        return SendMessageW(TB_AUTOSIZE);
    }

public: // Image list management methods
    DWORD SetImageList(HIMAGELIST himl)
    {
        return SendMessageW(TB_SETIMAGELIST, 0, (LPARAM) himl);
    }

    DWORD SetMetrics(TBMETRICS * tbm)
    {
        return SendMessageW(TB_SETMETRICS, 0, (LPARAM) tbm);
    }

public: // Utility methods
    TItemData * GetItemData(int index)
    {
        TBBUTTON btn;
        GetButton(index, &btn);
        return (TItemData*) btn.dwData;
    }

    DWORD SetItemData(int index, TItemData * data)
    {
        TBBUTTONINFOW info = { 0 };
        info.cbSize = sizeof(info);
        info.dwMask = TBIF_BYINDEX | TBIF_LPARAM;
        info.lParam = (DWORD_PTR) data;
        return SetButtonInfo(index, &info);
    }
};
