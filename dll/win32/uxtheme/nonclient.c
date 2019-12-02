/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS uxtheme.dll
 * FILE:            dll/win32/uxtheme/nonclient.c
 * PURPOSE:         uxtheme non client area management
 * PROGRAMMER:      Giannis Adamopoulos
 */
 
#include "uxthemep.h"

static BOOL 
IsWindowActive(HWND hWnd, DWORD ExStyle)
{
    BOOL ret;

    if (ExStyle & WS_EX_MDICHILD)
    {
        ret = IsChild(GetForegroundWindow(), hWnd);
        if (ret)
            ret = (hWnd == (HWND)SendMessageW(GetParent(hWnd), WM_MDIGETACTIVE, 0, 0));
    }
    else
    {
        ret = (GetForegroundWindow() == hWnd);
    }

    return ret;
}

BOOL
IsScrollBarVisible(HWND hWnd, INT hBar)
{
  SCROLLBARINFO sbi = {sizeof(SCROLLBARINFO)};
  if(!GetScrollBarInfo(hWnd, hBar, &sbi))
    return FALSE;

  return !(sbi.rgstate[0] & STATE_SYSTEM_OFFSCREEN);
}

static BOOL 
UserHasWindowEdge(DWORD Style, DWORD ExStyle)
{
    if (Style & WS_MINIMIZE)
        return TRUE;
    if (ExStyle & WS_EX_DLGMODALFRAME)
        return TRUE;
    if (ExStyle & WS_EX_STATICEDGE)
        return FALSE;
    if (Style & WS_THICKFRAME)
        return TRUE;
    Style &= WS_CAPTION;
    if (Style == WS_DLGFRAME || Style == WS_CAPTION)
        return TRUE;
   return FALSE;
}

static HICON
UserGetWindowIcon(PDRAW_CONTEXT pcontext)
{
    HICON hIcon = NULL;

    SendMessageTimeout(pcontext->hWnd, WM_GETICON, ICON_SMALL2, 0, SMTO_ABORTIFHUNG, 1000, (PDWORD_PTR)&hIcon);

    if (!hIcon)
        SendMessageTimeout(pcontext->hWnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 1000, (PDWORD_PTR)&hIcon);

    if (!hIcon)
        SendMessageTimeout(pcontext->hWnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG, 1000, (PDWORD_PTR)&hIcon);

    if (!hIcon)
        hIcon = (HICON)GetClassLongPtr(pcontext->hWnd, GCLP_HICONSM);

    if (!hIcon)
        hIcon = (HICON)GetClassLongPtr(pcontext->hWnd, GCLP_HICON);

    // See also win32ss/user/ntuser/nonclient.c!NC_IconForWindow
    if (!hIcon && !(pcontext->wi.dwExStyle & WS_EX_DLGMODALFRAME))
        hIcon = LoadIconW(NULL, (LPCWSTR)IDI_WINLOGO);

    return hIcon;
}

HRESULT WINAPI ThemeDrawCaptionText(PDRAW_CONTEXT pcontext, RECT* pRect, int iPartId, int iStateId)
{
    HRESULT hr;
    HFONT hFont = NULL;
    HGDIOBJ oldFont = NULL;
    LOGFONTW logfont;
    COLORREF textColor;
    COLORREF oldTextColor;
    int align = CA_LEFT;
    int drawStyles = DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS;

    WCHAR buffer[50];
    WCHAR *pszText = buffer;
    INT len;

    len = GetWindowTextLengthW(pcontext->hWnd);
    if (!len)
        return S_OK;

    len++; /* From now on this is the size of the buffer so include the null */

    if (len > ARRAYSIZE(buffer))
    {
        pszText = HeapAlloc(GetProcessHeap(), 0, len  * sizeof(WCHAR));
        if (!pszText)
            return E_OUTOFMEMORY;
    }

    InternalGetWindowText(pcontext->hWnd, pszText, len);

    hr = GetThemeSysFont(0,TMT_CAPTIONFONT,&logfont);
    if(SUCCEEDED(hr))
        hFont = CreateFontIndirectW(&logfont);

    if(hFont)
        oldFont = SelectObject(pcontext->hDC, hFont);
        
    if (!pcontext->Active)
        textColor = GetSysColor(COLOR_INACTIVECAPTIONTEXT);
    else
        textColor = GetSysColor(COLOR_CAPTIONTEXT);

    GetThemeEnumValue(pcontext->theme, iPartId, iStateId, TMT_CONTENTALIGNMENT, &align);
    if (align == CA_CENTER)
        drawStyles |= DT_CENTER;
    else if (align == CA_RIGHT)
        drawStyles |= DT_RIGHT;

    oldTextColor = SetTextColor(pcontext->hDC, textColor);
    DrawThemeText(pcontext->theme, 
                  pcontext->hDC, 
                  iPartId, 
                  iStateId, 
                  pszText, 
                  len - 1, 
                  drawStyles, 
                  0, 
                  pRect);
    SetTextColor(pcontext->hDC, oldTextColor);

    if (hFont)
    {
        SelectObject(pcontext->hDC, oldFont);
        DeleteObject(hFont);
    }
    if (pszText != buffer)
    {
        HeapFree(GetProcessHeap(), 0, pszText);
    }
    return S_OK;
}

void
ThemeInitDrawContext(PDRAW_CONTEXT pcontext,
                     HWND hWnd,
                     HRGN hRgn)
{
    pcontext->wi.cbSize = sizeof(pcontext->wi);
    GetWindowInfo(hWnd, &pcontext->wi);
    pcontext->hWnd = hWnd;
    pcontext->Active = IsWindowActive(hWnd, pcontext->wi.dwExStyle);
    pcontext->theme = GetNCCaptionTheme(hWnd, pcontext->wi.dwStyle);
    pcontext->scrolltheme = GetNCScrollbarTheme(hWnd, pcontext->wi.dwStyle);

    pcontext->CaptionHeight = pcontext->wi.cyWindowBorders;
    pcontext->CaptionHeight += GetSystemMetrics(pcontext->wi.dwExStyle & WS_EX_TOOLWINDOW ? SM_CYSMCAPTION : SM_CYCAPTION );

    if(hRgn <= (HRGN)1)
    {
        hRgn = CreateRectRgnIndirect(&pcontext->wi.rcWindow);
    }
    pcontext->hRgn = hRgn;

    pcontext->hDC = GetDCEx(hWnd, hRgn, DCX_WINDOW | DCX_INTERSECTRGN | DCX_USESTYLE | DCX_KEEPCLIPRGN);
}

void
ThemeCleanupDrawContext(PDRAW_CONTEXT pcontext)
{
    ReleaseDC(pcontext->hWnd ,pcontext->hDC);

    if(pcontext->hRgn != NULL)
    {
        DeleteObject(pcontext->hRgn);
    }
}

static void 
ThemeStartBufferedPaint(PDRAW_CONTEXT pcontext, int cx, int cy)
{
    HBITMAP hbmp;

    pcontext->hDCScreen = pcontext->hDC;
    pcontext->hDC = CreateCompatibleDC(pcontext->hDCScreen);
    hbmp = CreateCompatibleBitmap(pcontext->hDCScreen, cx, cy); 
    pcontext->hbmpOld = (HBITMAP)SelectObject(pcontext->hDC, hbmp);
}

static void
ThemeEndBufferedPaint(PDRAW_CONTEXT pcontext, int x, int y, int cx, int cy)
{
    HBITMAP hbmp;
    BitBlt(pcontext->hDCScreen, 0, 0, cx, cy, pcontext->hDC, x, y, SRCCOPY);
    hbmp = (HBITMAP) SelectObject(pcontext->hDC, pcontext->hbmpOld);
    DeleteObject(pcontext->hDC);
    DeleteObject(hbmp);

    pcontext->hDC = pcontext->hDCScreen;
}

void ThemeCalculateCaptionButtonsPos(HWND hWnd, HTHEME htheme)
{
    PWND_DATA pwndData;
    DWORD style;
    INT ButtonWidth, ButtonHeight, iPartId, i;
    WINDOWINFO wi = {sizeof(wi)};
    RECT rcCurrent;
    SIZE ButtonSize;

    /* First of all check if we have something to do here */
    style = GetWindowLongW(hWnd, GWL_STYLE);
    if((style & (WS_CAPTION | WS_SYSMENU)) != (WS_CAPTION | WS_SYSMENU))
        return;

    /* Get theme data for this window */
    pwndData = ThemeGetWndData(hWnd);
    if (pwndData == NULL)
        return;

    if (!htheme)
    {
        htheme = GetNCCaptionTheme(hWnd, style);
        if (!htheme)
            return;
    }

    if(!GetWindowInfo(hWnd, &wi))
        return;

    /* Calculate the area of the caption */
    rcCurrent.top = rcCurrent.left = 0;
    rcCurrent.right = wi.rcWindow.right - wi.rcWindow.left;
    rcCurrent.bottom = wi.rcWindow.bottom - wi.rcWindow.top;

    /* Add a padding around the objects of the caption */
    InflateRect(&rcCurrent, -(int)wi.cyWindowBorders-BUTTON_GAP_SIZE, 
                            -(int)wi.cyWindowBorders-BUTTON_GAP_SIZE);

    iPartId = wi.dwExStyle & WS_EX_TOOLWINDOW ? WP_SMALLCLOSEBUTTON : WP_CLOSEBUTTON;

    GetThemePartSize(htheme, NULL, iPartId, 0, NULL, TS_MIN, &ButtonSize);

    ButtonHeight = GetSystemMetrics( wi.dwExStyle & WS_EX_TOOLWINDOW ? SM_CYSMSIZE : SM_CYSIZE);
    ButtonWidth = MulDiv(ButtonSize.cx, ButtonHeight, ButtonSize.cy);

    ButtonHeight -= 4;
    ButtonWidth -= 4;

    for (i = CLOSEBUTTON; i <= HELPBUTTON; i++)
    {
        SetRect(&pwndData->rcCaptionButtons[i],
                rcCurrent.right - ButtonWidth,
                rcCurrent.top,
                rcCurrent.right,
                rcCurrent.top + ButtonHeight);

        rcCurrent.right -= ButtonWidth + BUTTON_GAP_SIZE;
    }
}

static void 
ThemeDrawCaptionButton(PDRAW_CONTEXT pcontext, 
                       RECT* prcCurrent,
                       CAPTIONBUTTON buttonId, 
                       INT iStateId)
{
    INT iPartId;
    PWND_DATA pwndData = ThemeGetWndData(pcontext->hWnd);
    if (!pwndData)
        return;

    switch(buttonId)
    {
    case CLOSEBUTTON:
        iPartId = pcontext->wi.dwExStyle & WS_EX_TOOLWINDOW ? WP_SMALLCLOSEBUTTON : WP_CLOSEBUTTON;
        break;

    case MAXBUTTON:
        if (!(pcontext->wi.dwStyle & WS_MAXIMIZEBOX))
        {
            if (!(pcontext->wi.dwStyle & WS_MINIMIZEBOX))
                return;
            else
                iStateId = (pcontext->Active ? BUTTON_DISABLED : BUTTON_INACTIVE_DISABLED);
        }

        iPartId = pcontext->wi.dwStyle & WS_MAXIMIZE ? WP_RESTOREBUTTON : WP_MAXBUTTON;
        break;

    case MINBUTTON:
        if (!(pcontext->wi.dwStyle & WS_MINIMIZEBOX))
        {
            if (!(pcontext->wi.dwStyle & WS_MAXIMIZEBOX))
                return;
            else
                iStateId = (pcontext->Active ? BUTTON_DISABLED : BUTTON_INACTIVE_DISABLED);
        }
 
        iPartId = pcontext->wi.dwStyle & WS_MINIMIZE ? WP_RESTOREBUTTON : WP_MINBUTTON;
        break;

    default:
        //FIXME: Implement Help Button 
        return;
    }

    if (prcCurrent)
        prcCurrent->right = pwndData->rcCaptionButtons[buttonId].left;

    DrawThemeBackground(pcontext->theme, pcontext->hDC, iPartId, iStateId, &pwndData->rcCaptionButtons[buttonId], NULL);
}

static DWORD
ThemeGetButtonState(DWORD htCurrect, DWORD htHot, DWORD htDown, BOOL Active)
{
    if (htHot == htCurrect)
        return (Active ? BUTTON_HOT : BUTTON_INACTIVE_HOT);
    if (htDown == htCurrect)
        return (Active ? BUTTON_PRESSED : BUTTON_INACTIVE_PRESSED);

    return (Active ? BUTTON_NORMAL : BUTTON_INACTIVE);
}

/* Used only from mouse event handlers */
static void 
ThemeDrawCaptionButtons(PDRAW_CONTEXT pcontext, DWORD htHot, DWORD htDown)
{
    /* Draw the buttons */
    ThemeDrawCaptionButton(pcontext, NULL, CLOSEBUTTON, 
                           ThemeGetButtonState(HTCLOSE, htHot, htDown, pcontext->Active));
    ThemeDrawCaptionButton(pcontext, NULL, MAXBUTTON,  
                           ThemeGetButtonState(HTMAXBUTTON, htHot, htDown, pcontext->Active));
    ThemeDrawCaptionButton(pcontext, NULL, MINBUTTON,
                           ThemeGetButtonState(HTMINBUTTON, htHot, htDown, pcontext->Active));
    ThemeDrawCaptionButton(pcontext, NULL, HELPBUTTON,
                           ThemeGetButtonState(HTHELP, htHot, htDown, pcontext->Active));
}

/* Used from WM_NCPAINT and WM_NCACTIVATE handlers */
static void 
ThemeDrawCaption(PDRAW_CONTEXT pcontext, RECT* prcCurrent)
{
    RECT rcPart;
    int iPart, iState;
    HICON hIcon;

    // See also win32ss/user/ntuser/nonclient.c!UserDrawCaptionBar
    // and win32ss/user/ntuser/nonclient.c!UserDrawCaption
    if ((pcontext->wi.dwStyle & WS_SYSMENU) && !(pcontext->wi.dwExStyle & WS_EX_TOOLWINDOW))
        hIcon = UserGetWindowIcon(pcontext);
    else
        hIcon = NULL;

    /* Get the caption part and state id */
    if (pcontext->wi.dwStyle & WS_MINIMIZE)
        iPart = WP_MINCAPTION;
    else if (pcontext->wi.dwExStyle & WS_EX_TOOLWINDOW)
        iPart = WP_SMALLCAPTION;
    else if (pcontext->wi.dwStyle & WS_MAXIMIZE)
        iPart = WP_MAXCAPTION;
    else
        iPart = WP_CAPTION;

    iState = pcontext->Active ? FS_ACTIVE : FS_INACTIVE;

    /* Draw the caption background */
    rcPart = *prcCurrent;
    rcPart.bottom = rcPart.top + pcontext->CaptionHeight;
    prcCurrent->top = rcPart.bottom;
    DrawThemeBackground(pcontext->theme, pcontext->hDC,iPart,iState,&rcPart,NULL);

    /* Add a padding around the objects of the caption */
    InflateRect(&rcPart, -(int)pcontext->wi.cyWindowBorders-BUTTON_GAP_SIZE, 
                         -(int)pcontext->wi.cyWindowBorders-BUTTON_GAP_SIZE);

    /* Draw the caption buttons */
    if (pcontext->wi.dwStyle & WS_SYSMENU)
    {
        iState = pcontext->Active ? BUTTON_NORMAL : BUTTON_INACTIVE;

        ThemeDrawCaptionButton(pcontext, &rcPart, CLOSEBUTTON, iState);
        ThemeDrawCaptionButton(pcontext, &rcPart, MAXBUTTON, iState);
        ThemeDrawCaptionButton(pcontext, &rcPart, MINBUTTON, iState);
        ThemeDrawCaptionButton(pcontext, &rcPart, HELPBUTTON, iState);
    }
    
    rcPart.top += 3 ;

    /* Draw the icon */
    if (hIcon)
    {
        int IconHeight = GetSystemMetrics(SM_CYSMICON);
        int IconWidth = GetSystemMetrics(SM_CXSMICON);
        DrawIconEx(pcontext->hDC, rcPart.left, rcPart.top , hIcon, IconWidth, IconHeight, 0, NULL, DI_NORMAL);
        rcPart.left += IconWidth + 4;
    }

    rcPart.right -= 4;

    /* Draw the caption */
    ThemeDrawCaptionText(pcontext, &rcPart, iPart, iState);
}

static void 
ThemeDrawBorders(PDRAW_CONTEXT pcontext, RECT* prcCurrent)
{
    RECT rcPart;
    int iState = pcontext->Active ? FS_ACTIVE : FS_INACTIVE;

    /* Draw the bottom border */
    rcPart = *prcCurrent;
    rcPart.top = rcPart.bottom - pcontext->wi.cyWindowBorders;
    prcCurrent->bottom = rcPart.top;
    DrawThemeBackground(pcontext->theme, pcontext->hDC, WP_FRAMEBOTTOM, iState, &rcPart, NULL);

    /* Draw the left border */
    rcPart = *prcCurrent;
    rcPart.right = rcPart.left + pcontext->wi.cxWindowBorders ;
    prcCurrent->left = rcPart.right;
    DrawThemeBackground(pcontext->theme, pcontext->hDC,WP_FRAMELEFT, iState, &rcPart, NULL);

    /* Draw the right border */
    rcPart = *prcCurrent;
    rcPart.left = rcPart.right - pcontext->wi.cxWindowBorders;
    prcCurrent->right = rcPart.left;
    DrawThemeBackground(pcontext->theme, pcontext->hDC,WP_FRAMERIGHT, iState, &rcPart, NULL);
}

static void 
DrawClassicFrame(PDRAW_CONTEXT context, RECT* prcCurrent)
{
    /* Draw outer edge */
    if (UserHasWindowEdge(context->wi.dwStyle, context->wi.dwExStyle))
    {
        DrawEdge(context->hDC, prcCurrent, EDGE_RAISED, BF_RECT | BF_ADJUST);
    } 
    else if (context->wi.dwExStyle & WS_EX_STATICEDGE)
    {
        DrawEdge(context->hDC, prcCurrent, BDR_SUNKENINNER, BF_RECT | BF_ADJUST | BF_FLAT);
    }

    /* Firstly the "thick" frame */
    if ((context->wi.dwStyle & WS_THICKFRAME) && !(context->wi.dwStyle & WS_MINIMIZE))
    {
        INT Width =
            (GetSystemMetrics(SM_CXFRAME) - GetSystemMetrics(SM_CXDLGFRAME)) *
            GetSystemMetrics(SM_CXBORDER);
        INT Height =
            (GetSystemMetrics(SM_CYFRAME) - GetSystemMetrics(SM_CYDLGFRAME)) *
            GetSystemMetrics(SM_CYBORDER);

        SelectObject(context->hDC, GetSysColorBrush(
                     context->Active ? COLOR_ACTIVEBORDER : COLOR_INACTIVEBORDER));

        /* Draw frame */
        PatBlt(context->hDC, prcCurrent->left, prcCurrent->top, 
               prcCurrent->right - prcCurrent->left, Height, PATCOPY);
        PatBlt(context->hDC, prcCurrent->left, prcCurrent->top, 
               Width, prcCurrent->bottom - prcCurrent->top, PATCOPY);
        PatBlt(context->hDC, prcCurrent->left, prcCurrent->bottom - 1, 
               prcCurrent->right - prcCurrent->left, -Height, PATCOPY);
        PatBlt(context->hDC, prcCurrent->right - 1, prcCurrent->top, 
               -Width, prcCurrent->bottom - prcCurrent->top, PATCOPY);

        InflateRect(prcCurrent, -Width, -Height);
    }

    /* Now the other bit of the frame */
    if (context->wi.dwStyle & (WS_DLGFRAME | WS_BORDER) || (context->wi.dwExStyle & WS_EX_DLGMODALFRAME))
    {
        INT Width = GetSystemMetrics(SM_CXBORDER);
        INT Height = GetSystemMetrics(SM_CYBORDER);

        SelectObject(context->hDC, GetSysColorBrush(
            (context->wi.dwExStyle & (WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE)) ? COLOR_3DFACE :
            (context->wi.dwExStyle & WS_EX_STATICEDGE) ? COLOR_WINDOWFRAME :
            (context->wi.dwStyle & (WS_DLGFRAME | WS_THICKFRAME)) ? COLOR_3DFACE :
            COLOR_WINDOWFRAME));

        /* Draw frame */
        PatBlt(context->hDC, prcCurrent->left, prcCurrent->top, 
               prcCurrent->right - prcCurrent->left, Height, PATCOPY);
        PatBlt(context->hDC, prcCurrent->left, prcCurrent->top, 
               Width, prcCurrent->bottom - prcCurrent->top, PATCOPY);
        PatBlt(context->hDC, prcCurrent->left, prcCurrent->bottom - 1, 
               prcCurrent->right - prcCurrent->left, -Height, PATCOPY);
        PatBlt(context->hDC, prcCurrent->right - 1, prcCurrent->top, 
              -Width, prcCurrent->bottom - prcCurrent->top, PATCOPY);

        InflateRect(prcCurrent, -Width, -Height);
    }
}

static void ThemeDrawMenuBar(PDRAW_CONTEXT pcontext, RECT* prcCurrent)
{
    /* Let the window manager paint the menu */
    prcCurrent->top += PaintMenuBar(pcontext->hWnd, 
                                    pcontext->hDC, 
                                    pcontext->wi.cxWindowBorders, 
                                    pcontext->wi.cxWindowBorders,
                                    prcCurrent->top, 
                                    pcontext->Active);
}

static void ThemeDrawScrollBarsGrip(PDRAW_CONTEXT pcontext, RECT* prcCurrent)
{
    RECT rcPart;
    HWND hwndParent;
    RECT ParentClientRect;
    DWORD ParentStyle;

    rcPart = *prcCurrent;

    if (pcontext->wi.dwExStyle & WS_EX_LEFTSCROLLBAR)
       rcPart.right = rcPart.left + GetSystemMetrics(SM_CXVSCROLL);
    else
       rcPart.left = rcPart.right - GetSystemMetrics(SM_CXVSCROLL);

    rcPart.top = rcPart.bottom - GetSystemMetrics(SM_CYHSCROLL);

    FillRect(pcontext->hDC, &rcPart, GetSysColorBrush(COLOR_BTNFACE));

    hwndParent = GetParent(pcontext->hWnd);
    GetClientRect(hwndParent, &ParentClientRect);
    ParentStyle = GetWindowLongW(hwndParent, GWL_STYLE);

    if (HASSIZEGRIP(pcontext->wi.dwStyle, pcontext->wi.dwExStyle, ParentStyle, pcontext->wi.rcWindow, ParentClientRect))
    {
        int iState;
        if (pcontext->wi.dwExStyle & WS_EX_LEFTSCROLLBAR)
            iState = pcontext->wi.dwExStyle & WS_EX_LEFTSCROLLBAR;
        else
            iState = SZB_RIGHTALIGN;
        DrawThemeBackground(pcontext->scrolltheme, pcontext->hDC, SBP_SIZEBOX, iState, &rcPart, NULL);
    }
}

static void 
ThemePaintWindow(PDRAW_CONTEXT pcontext, RECT* prcCurrent, BOOL bDoDoubleBuffering)
{
    if(!(pcontext->wi.dwStyle & WS_VISIBLE))
        return;

    if((pcontext->wi.dwStyle & WS_CAPTION)==WS_CAPTION)
    {
        if (bDoDoubleBuffering)
            ThemeStartBufferedPaint(pcontext, prcCurrent->right, pcontext->CaptionHeight);
        ThemeDrawCaption(pcontext, prcCurrent);
        if (bDoDoubleBuffering)
            ThemeEndBufferedPaint(pcontext, 0, 0, prcCurrent->right, pcontext->CaptionHeight);
        ThemeDrawBorders(pcontext, prcCurrent);
    }
    else
    {
        DrawClassicFrame(pcontext, prcCurrent);
    }

    if(pcontext->wi.dwStyle & WS_MINIMIZE)
        return;

    if(HAS_MENU(pcontext->hWnd, pcontext->wi.dwStyle))
        ThemeDrawMenuBar(pcontext, prcCurrent);

    if (pcontext->wi.dwExStyle & WS_EX_CLIENTEDGE)
        DrawEdge(pcontext->hDC, prcCurrent, EDGE_SUNKEN, BF_RECT | BF_ADJUST);

    if((pcontext->wi.dwStyle & WS_HSCROLL) && IsScrollBarVisible(pcontext->hWnd, OBJID_HSCROLL))
        ThemeDrawScrollBar(pcontext, SB_HORZ , NULL);

    if((pcontext->wi.dwStyle & WS_VSCROLL) && IsScrollBarVisible(pcontext->hWnd, OBJID_VSCROLL))
        ThemeDrawScrollBar(pcontext, SB_VERT, NULL);

    if((pcontext->wi.dwStyle & (WS_HSCROLL|WS_VSCROLL)) == (WS_HSCROLL|WS_VSCROLL) &&
       IsScrollBarVisible(pcontext->hWnd, OBJID_HSCROLL) &&
       IsScrollBarVisible(pcontext->hWnd, OBJID_VSCROLL))
    {
        ThemeDrawScrollBarsGrip(pcontext, prcCurrent);
    }
}

/*
 * Message handlers
 */

static LRESULT 
ThemeHandleNCPaint(HWND hWnd, HRGN hRgn)
{
    DRAW_CONTEXT context;
    RECT rcCurrent;

    ThemeInitDrawContext(&context, hWnd, hRgn);

    rcCurrent = context.wi.rcWindow;
    OffsetRect( &rcCurrent, -context.wi.rcWindow.left, -context.wi.rcWindow.top);

    ThemePaintWindow(&context, &rcCurrent, TRUE);
    ThemeCleanupDrawContext(&context);

    return 0;
}

static LRESULT 
ThemeHandleNcMouseMove(HWND hWnd, DWORD ht, POINT* pt)
{
    DRAW_CONTEXT context;
    TRACKMOUSEEVENT tme;
    DWORD style;
    PWND_DATA pwndData;

    /* First of all check if we have something to do here */
    style = GetWindowLongW(hWnd, GWL_STYLE);
    if((style & (WS_CAPTION|WS_HSCROLL|WS_VSCROLL))==0)
        return 0;

    /* Get theme data for this window */
    pwndData = ThemeGetWndData(hWnd);
    if (pwndData == NULL)
        return 0;

    /* Begin tracking in the non client area if we are not tracking yet */
    tme.cbSize = sizeof(TRACKMOUSEEVENT);
    tme.dwFlags = TME_QUERY;
    tme.hwndTrack  = hWnd;
    TrackMouseEvent(&tme);
    if (tme.dwFlags != (TME_LEAVE | TME_NONCLIENT))
    {
        tme.hwndTrack  = hWnd;
        tme.dwFlags = TME_LEAVE | TME_NONCLIENT;
        TrackMouseEvent(&tme);
    }

    ThemeInitDrawContext(&context, hWnd, 0);
    if (context.wi.dwStyle & WS_SYSMENU)
    {
        if (HT_ISBUTTON(ht) || HT_ISBUTTON(pwndData->lastHitTest))
            ThemeDrawCaptionButtons(&context, ht, 0);
    }

   if (context.wi.dwStyle & WS_HSCROLL)
   {
       if (ht == HTHSCROLL || pwndData->lastHitTest == HTHSCROLL)
           ThemeDrawScrollBar(&context, SB_HORZ , ht == HTHSCROLL ? pt : NULL);
   }

    if (context.wi.dwStyle & WS_VSCROLL)
    {
        if (ht == HTVSCROLL || pwndData->lastHitTest == HTVSCROLL)
            ThemeDrawScrollBar(&context, SB_VERT, ht == HTVSCROLL ? pt : NULL);
    }
    ThemeCleanupDrawContext(&context);

    pwndData->lastHitTest = ht;

    return 0;
}

static LRESULT 
ThemeHandleNcMouseLeave(HWND hWnd)
{
    DRAW_CONTEXT context;
    DWORD style;
    PWND_DATA pwndData;

    /* First of all check if we have something to do here */
    style = GetWindowLongW(hWnd, GWL_STYLE);
    if((style & (WS_CAPTION|WS_HSCROLL|WS_VSCROLL))==0)
        return 0;

    /* Get theme data for this window */
    pwndData = ThemeGetWndData(hWnd);
    if (pwndData == NULL)
        return 0;

    ThemeInitDrawContext(&context, hWnd, 0);
    if (context.wi.dwStyle & WS_SYSMENU && HT_ISBUTTON(pwndData->lastHitTest))
        ThemeDrawCaptionButtons(&context, 0, 0);

   if (context.wi.dwStyle & WS_HSCROLL && pwndData->lastHitTest == HTHSCROLL)
        ThemeDrawScrollBar(&context, SB_HORZ,  NULL);

    if (context.wi.dwStyle & WS_VSCROLL && pwndData->lastHitTest == HTVSCROLL)
        ThemeDrawScrollBar(&context, SB_VERT, NULL);

    ThemeCleanupDrawContext(&context);

    pwndData->lastHitTest = HTNOWHERE;

    return 0;
}

static VOID
ThemeHandleButton(HWND hWnd, WPARAM wParam)
{
    MSG Msg;
    BOOL Pressed = TRUE;
    WPARAM SCMsg, ht;
    ULONG Style;
    DRAW_CONTEXT context;
    PWND_DATA pwndData;

    Style = GetWindowLongW(hWnd, GWL_STYLE);
    if (!((Style & WS_CAPTION) && (Style & WS_SYSMENU)))
        return ;

    switch (wParam)
    {
        case HTCLOSE:
            SCMsg = SC_CLOSE;
            break;
        case HTMINBUTTON:
            if (!(Style & WS_MINIMIZEBOX))
                return;
            SCMsg = ((Style & WS_MINIMIZE) ? SC_RESTORE : SC_MINIMIZE);
            break;
        case HTMAXBUTTON:
            if (!(Style & WS_MAXIMIZEBOX))
                return;
            SCMsg = ((Style & WS_MAXIMIZE) ? SC_RESTORE : SC_MAXIMIZE);
            break;
        default :
            return;
    }

    /* Get theme data for this window */
    pwndData = ThemeGetWndData(hWnd);
    if (pwndData == NULL)
        return;

    ThemeInitDrawContext(&context, hWnd, 0);
    ThemeDrawCaptionButtons(&context, 0,  wParam);
    pwndData->lastHitTest = wParam;

    SetCapture(hWnd);

    ht = wParam;

    for (;;)
    {
        if (GetMessageW(&Msg, 0, WM_MOUSEFIRST, WM_MOUSELAST) <= 0)
            break;

        if (Msg.message == WM_LBUTTONUP)
            break;

        if (Msg.message != WM_MOUSEMOVE)
            continue;

        ht = SendMessage(hWnd, WM_NCHITTEST, 0, MAKELPARAM(Msg.pt.x, Msg.pt.y));
        Pressed = (ht == wParam);

        /* Only draw the buttons if the hit test changed */
        if (ht != pwndData->lastHitTest &&
            (HT_ISBUTTON(ht) || HT_ISBUTTON(pwndData->lastHitTest)))
        {
            ThemeDrawCaptionButtons(&context, 0, Pressed ? wParam: 0);
            pwndData->lastHitTest = ht;
        }
    }

    ThemeDrawCaptionButtons(&context, ht, 0);
    ThemeCleanupDrawContext(&context);

    ReleaseCapture();

    if (Pressed)
        SendMessageW(hWnd, WM_SYSCOMMAND, SCMsg, 0);
}


static LRESULT
DefWndNCHitTest(HWND hWnd, POINT Point)
{
    RECT WindowRect;
    POINT ClientPoint;
    WINDOWINFO wi;

    wi.cbSize = sizeof(wi);
    GetWindowInfo(hWnd, &wi);

    if (!PtInRect(&wi.rcWindow, Point))
    {
        return HTNOWHERE;
    }
    WindowRect = wi.rcWindow;

    if (UserHasWindowEdge(wi.dwStyle, wi.dwExStyle))
    {
        LONG XSize, YSize;

        InflateRect(&WindowRect, -(int)wi.cxWindowBorders, -(int)wi.cyWindowBorders);
        XSize = GetSystemMetrics(SM_CXSIZE) * GetSystemMetrics(SM_CXBORDER);
        YSize = GetSystemMetrics(SM_CYSIZE) * GetSystemMetrics(SM_CYBORDER);
        if (!PtInRect(&WindowRect, Point))
        {
            BOOL ThickFrame;

            ThickFrame = (wi.dwStyle & WS_THICKFRAME);
            if (Point.y < WindowRect.top)
            {
                if(wi.dwStyle & WS_MINIMIZE)
                    return HTCAPTION;
                if(!ThickFrame)
                    return HTBORDER;
                if (Point.x < (WindowRect.left + XSize))
                    return HTTOPLEFT;
                if (Point.x >= (WindowRect.right - XSize))
                    return HTTOPRIGHT;
                return HTTOP;
            }
            if (Point.y >= WindowRect.bottom)
            {
                if(wi.dwStyle & WS_MINIMIZE)
                    return HTCAPTION;
                if(!ThickFrame)
                    return HTBORDER;
                if (Point.x < (WindowRect.left + XSize))
                    return HTBOTTOMLEFT;
                if (Point.x >= (WindowRect.right - XSize))
                    return HTBOTTOMRIGHT;
                return HTBOTTOM;
            }
            if (Point.x < WindowRect.left)
            {
                if(wi.dwStyle & WS_MINIMIZE)
                    return HTCAPTION;
                if(!ThickFrame)
                    return HTBORDER;
                if (Point.y < (WindowRect.top + YSize))
                    return HTTOPLEFT;
                if (Point.y >= (WindowRect.bottom - YSize))
                    return HTBOTTOMLEFT;
                return HTLEFT;
            }
            if (Point.x >= WindowRect.right)
            {
                if(wi.dwStyle & WS_MINIMIZE)
                    return HTCAPTION;
                if(!ThickFrame)
                    return HTBORDER;
                if (Point.y < (WindowRect.top + YSize))
                    return HTTOPRIGHT;
                if (Point.y >= (WindowRect.bottom - YSize))
                    return HTBOTTOMRIGHT;
                return HTRIGHT;
            }
        }
    }
    else
    {
        if (wi.dwExStyle & WS_EX_STATICEDGE)
            InflateRect(&WindowRect, -GetSystemMetrics(SM_CXBORDER),
                                     -GetSystemMetrics(SM_CYBORDER));
        if (!PtInRect(&WindowRect, Point))
            return HTBORDER;
    }

    if ((wi.dwStyle & WS_CAPTION) == WS_CAPTION)
    {
        if (wi.dwExStyle & WS_EX_TOOLWINDOW)
            WindowRect.top += GetSystemMetrics(SM_CYSMCAPTION);
        else
            WindowRect.top += GetSystemMetrics(SM_CYCAPTION);

        if (!PtInRect(&WindowRect, Point))
        {
            if (wi.dwStyle & WS_SYSMENU)
            {
                PWND_DATA pwndData = ThemeGetWndData(hWnd);

                if (!(wi.dwExStyle & WS_EX_TOOLWINDOW))
                {
                    // if(!(wi.dwExStyle & WS_EX_DLGMODALFRAME))
                    // FIXME: The real test should check whether there is
                    // an icon for the system window, and if so, do the
                    // rect.left increase.
                    // See win32ss/user/user32/windows/nonclient.c!DefWndNCHitTest
                    // and win32ss/user/ntuser/nonclient.c!GetNCHitEx which does
                    // the test better.
                        WindowRect.left += GetSystemMetrics(SM_CXSMICON);
                }

                if (pwndData)
                {
                    POINT pt = {Point.x - wi.rcWindow.left, Point.y - wi.rcWindow.top};
                    if (PtInRect(&pwndData->rcCaptionButtons[CLOSEBUTTON], pt))
                        return HTCLOSE;
                    if (PtInRect(&pwndData->rcCaptionButtons[MAXBUTTON], pt))
                        return HTMAXBUTTON;
                    if (PtInRect(&pwndData->rcCaptionButtons[MINBUTTON], pt))
                        return HTMINBUTTON;
                }
            }
            if (Point.x < WindowRect.left)
                return HTSYSMENU;
            return HTCAPTION;
        }
    }

    if(!(wi.dwStyle & WS_MINIMIZE))
    {
        HMENU menu;

        ClientPoint = Point;
        ScreenToClient(hWnd, &ClientPoint);
        GetClientRect(hWnd, &wi.rcClient);

        if (PtInRect(&wi.rcClient, ClientPoint))
        {
            return HTCLIENT;
        }

        if ((menu = GetMenu(hWnd)) && !(wi.dwStyle & WS_CHILD))
        {
            if (Point.x > 0 && Point.x < WindowRect.right && ClientPoint.y < 0)
                return HTMENU;
        }

        if (wi.dwExStyle & WS_EX_CLIENTEDGE)
        {
            InflateRect(&WindowRect, -2 * GetSystemMetrics(SM_CXBORDER),
                        -2 * GetSystemMetrics(SM_CYBORDER));
        }

        if ((wi.dwStyle & WS_VSCROLL) && (wi.dwStyle & WS_HSCROLL) &&
            (WindowRect.bottom - WindowRect.top) > GetSystemMetrics(SM_CYHSCROLL))
        {
            RECT ParentRect, TempRect = WindowRect, TempRect2 = WindowRect;
            HWND Parent = GetParent(hWnd);

            TempRect.bottom -= GetSystemMetrics(SM_CYHSCROLL);
            if ((wi.dwExStyle & WS_EX_LEFTSCROLLBAR) != 0)
                TempRect.right = TempRect.left + GetSystemMetrics(SM_CXVSCROLL);
            else
                TempRect.left = TempRect.right - GetSystemMetrics(SM_CXVSCROLL);
            if (PtInRect(&TempRect, Point))
                return HTVSCROLL;

            TempRect2.top = TempRect2.bottom - GetSystemMetrics(SM_CYHSCROLL);
            if ((wi.dwExStyle & WS_EX_LEFTSCROLLBAR) != 0)
                TempRect2.left += GetSystemMetrics(SM_CXVSCROLL);
            else
                TempRect2.right -= GetSystemMetrics(SM_CXVSCROLL);
            if (PtInRect(&TempRect2, Point))
                return HTHSCROLL;

            TempRect.top = TempRect2.top;
            TempRect.bottom = TempRect2.bottom;
            if(Parent)
                GetClientRect(Parent, &ParentRect);
            if (PtInRect(&TempRect, Point) && HASSIZEGRIP(wi.dwStyle, wi.dwExStyle,
                      GetWindowLongW(Parent, GWL_STYLE), wi.rcWindow, ParentRect))
            {
                if ((wi.dwExStyle & WS_EX_LEFTSCROLLBAR) != 0)
                    return HTBOTTOMLEFT;
                else
                    return HTBOTTOMRIGHT;
            }
        }
        else
        {
            if (wi.dwStyle & WS_VSCROLL)
            {
                RECT TempRect = WindowRect;

                if ((wi.dwExStyle & WS_EX_LEFTSCROLLBAR) != 0)
                    TempRect.right = TempRect.left + GetSystemMetrics(SM_CXVSCROLL);
                else
                    TempRect.left = TempRect.right - GetSystemMetrics(SM_CXVSCROLL);
                if (PtInRect(&TempRect, Point))
                    return HTVSCROLL;
            } 
            else if (wi.dwStyle & WS_HSCROLL)
            {
                RECT TempRect = WindowRect;
                TempRect.top = TempRect.bottom - GetSystemMetrics(SM_CYHSCROLL);
                if (PtInRect(&TempRect, Point))
                    return HTHSCROLL;
            }
        }
    }

    return HTNOWHERE;
}

LRESULT CALLBACK 
ThemeWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, WNDPROC DefWndProc)
{
    switch(Msg)
    {
    case WM_NCPAINT:
        return ThemeHandleNCPaint(hWnd, (HRGN)wParam);
    //
    // WM_NCUAHDRAWCAPTION : wParam are DC_* flags.
    //
    case WM_NCUAHDRAWCAPTION:
    //
    // WM_NCUAHDRAWFRAME : wParam is HDC, lParam are DC_ACTIVE and or DC_REDRAWHUNGWND.
    //
    case WM_NCUAHDRAWFRAME:
    case WM_NCACTIVATE:

        if ((GetWindowLongW(hWnd, GWL_STYLE) & WS_CAPTION) != WS_CAPTION)
            return TRUE;

        ThemeHandleNCPaint(hWnd, (HRGN)1);
        return TRUE;
    case WM_NCMOUSEMOVE:
    {
        POINT Point;
        Point.x = GET_X_LPARAM(lParam);
        Point.y = GET_Y_LPARAM(lParam);
        return ThemeHandleNcMouseMove(hWnd, wParam, &Point);
    }
    case WM_NCMOUSELEAVE:
        return ThemeHandleNcMouseLeave(hWnd);
    case WM_NCLBUTTONDOWN:
        switch (wParam)
        {
            case HTMINBUTTON:
            case HTMAXBUTTON:
            case HTCLOSE:
            {
                ThemeHandleButton(hWnd, wParam);
                return 0;
            }
            default:
                return DefWndProc(hWnd, Msg, wParam, lParam);
        }
    case WM_NCHITTEST:
    {
        POINT Point;
        Point.x = GET_X_LPARAM(lParam);
        Point.y = GET_Y_LPARAM(lParam);
        return DefWndNCHitTest(hWnd, Point);
    }
    case WM_SYSCOMMAND:
    {
        if((wParam & 0xfff0) == SC_VSCROLL ||
           (wParam & 0xfff0) == SC_HSCROLL)
        {
            POINT Pt;
            Pt.x = (short)LOWORD(lParam);
            Pt.y = (short)HIWORD(lParam);
            NC_TrackScrollBar(hWnd, wParam, Pt);
            return 0;
        }
        else
        {
            return DefWndProc(hWnd, Msg, wParam, lParam);
        }
    }
    default:
        return DefWndProc(hWnd, Msg, wParam, lParam);
    }
}

HRESULT WINAPI DrawNCPreview(HDC hDC, 
                             DWORD DNCP_Flag,
                             LPRECT prcPreview, 
                             LPCWSTR pszThemeFileName, 
                             LPCWSTR pszColorName,
                             LPCWSTR pszSizeName,
                             PNONCLIENTMETRICSW pncMetrics,
                             COLORREF* lpaRgbValues)
{
    WNDCLASSEXW DummyPreviewWindowClass;
    HWND hwndDummy;
    HRESULT hres;
    HTHEMEFILE hThemeFile;
    DRAW_CONTEXT context;
    RECT rcCurrent;

    /* FIXME: We also need to implement drawing the rest of the preview windows 
     *        and make use of the ncmetrics and colors passed as parameters */

    /* Create a dummy window that will be used to trick the paint funtions */
    memset(&DummyPreviewWindowClass, 0, sizeof(DummyPreviewWindowClass));
    DummyPreviewWindowClass.cbSize = sizeof(DummyPreviewWindowClass);
    DummyPreviewWindowClass.lpszClassName = L"DummyPreviewWindowClass";
    DummyPreviewWindowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    DummyPreviewWindowClass.hInstance = hDllInst;
    DummyPreviewWindowClass.lpfnWndProc = DefWindowProcW;
    if (!RegisterClassExW(&DummyPreviewWindowClass))
        return E_FAIL;

    hwndDummy = CreateWindowExW(0, L"DummyPreviewWindowClass", L"Active window", WS_OVERLAPPEDWINDOW,30,30,300,150,0,0,hDllInst,NULL);
    if (!hwndDummy)
        return E_FAIL;

    hres = OpenThemeFile(pszThemeFileName, pszColorName, pszSizeName, &hThemeFile,0);
    if (FAILED(hres))
        return hres;

    /* Initialize the special draw context for the preview */
    context.hDC = hDC;
    context.hWnd = hwndDummy;
    context.theme = OpenThemeDataFromFile(hThemeFile, hwndDummy, L"WINDOW", 0);
    if (!context.theme)
        return E_FAIL;
    context.scrolltheme = OpenThemeDataFromFile(hThemeFile, hwndDummy, L"SCROLLBAR", 0);
    if (!context.scrolltheme)
        return E_FAIL;
    context.Active = TRUE;
    context.wi.cbSize = sizeof(context.wi);
    if (!GetWindowInfo(hwndDummy, &context.wi))
        return E_FAIL;
    context.wi.dwStyle |= WS_VISIBLE;
    context.CaptionHeight = context.wi.cyWindowBorders;
    context.CaptionHeight += GetSystemMetrics(context.wi.dwExStyle & WS_EX_TOOLWINDOW ? SM_CYSMCAPTION : SM_CYCAPTION );
    context.hRgn = CreateRectRgnIndirect(&context.wi.rcWindow);

    /* Paint the window on the preview hDC */
    rcCurrent = context.wi.rcWindow;
    OffsetRect( &rcCurrent, -context.wi.rcWindow.left, -context.wi.rcWindow.top);
    SetViewportOrgEx(hDC, context.wi.rcWindow.left, context.wi.rcWindow.top, NULL);
    ThemeCalculateCaptionButtonsPos(hwndDummy, context.theme);
    ThemePaintWindow(&context, &rcCurrent, FALSE);
    SetViewportOrgEx(hDC, 0, 0, NULL);

    context.hDC = NULL;
    CloseThemeData (context.theme);
    CloseThemeData (context.scrolltheme);
    ThemeCleanupDrawContext(&context);

    /* Cleanup */
    DestroyWindow(hwndDummy);
    UnregisterClassW(L"DummyPreviewWindowClass", hDllInst);

    return S_OK;
}
