#include "result_list.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include <commctrl.h>
#include <algorithm>
#include <format>
#include <string>

static constexpr wchar_t CLASS_NAME[] = L"MotionTracker_ResultList";

// 行の背景色
static constexpr COLORREF COLOR_OK      = RGB(0xcc, 0xff, 0xcc);
static constexpr COLORREF COLOR_CURRENT = RGB(0x00, 0xff, 0xff);
static constexpr COLORREF COLOR_ERROR   = RGB(0xff, 0xc0, 0xc0);
static constexpr COLORREF COLOR_NONE    = RGB(0xff, 0xff, 0xff); // 未解析

bool ResultList::s_registered = false;

// 選択範囲のフレーム数
int ResultList::RowCount() const {
    int analyzed = static_cast<int>(m_tracker->Results().size());
    int range    = m_tracker->RangeEnd() - m_tracker->RangeStart() + 1;
    return std::max(analyzed, range);
}

COLORREF ResultList::RowColor(int row) const {
    if (row == m_current) return COLOR_CURRENT;
    if (static_cast<size_t>(row) >= m_tracker->Results().size()) return COLOR_NONE;
    return m_tracker->Found()[row] ? COLOR_OK : COLOR_ERROR;
}

// 列: frame, x, y, width, height
void ResultList::GetCellText(int row, int col, wchar_t* buf, int len) const {
    std::wstring text;
    const auto& results = m_tracker->Results();
    if (col == 0) {
        text = std::to_wstring(m_tracker->RangeStart() + row);
    } else if (static_cast<size_t>(row) < results.size()) {
        const cv::Rect2d& r = results[row];
        double v = col == 1 ? r.x : col == 2 ? r.y : col == 3 ? r.width : r.height;
        text = std::format(L"{:.2f}", v);
    }
    wcsncpy_s(buf, len, text.c_str(), _TRUNCATE);
}

void ResultList::Refresh() {
    if (m_current >= 0) ListView_RedrawItems(m_listView, m_current, m_current);
}

void ResultList::SetCurrent(int pos) {
    int old = m_current;
    m_current = pos;
    if (old >= 0) ListView_RedrawItems(m_listView, old, old);
    ListView_RedrawItems(m_listView, pos, pos);
    ListView_EnsureVisible(m_listView, pos, FALSE);
}

LRESULT CALLBACK ResultList::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    ResultList* self = reinterpret_cast<ResultList*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_NCCREATE:
        self = reinterpret_cast<ResultList*>(
            reinterpret_cast<CREATESTRUCT*>(lp)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hwnd = hwnd;
        return DefWindowProc(hwnd, msg, wp, lp);

    case WM_SIZE:
        if (self->m_listView) {
            SetWindowPos(self->m_listView, nullptr, 0, 0, LOWORD(lp), HIWORD(lp), SWP_NOZORDER);
        }
        return 0;

    case WM_NOTIFY: {
        auto* hdr = reinterpret_cast<NMHDR*>(lp);
        if (hdr->hwndFrom != self->m_listView) break;

        // 仮想リストの表示内容
        if (hdr->code == LVN_GETDISPINFOW) {
            auto* di = reinterpret_cast<NMLVDISPINFOW*>(lp);
            if (di->item.mask & LVIF_TEXT) {
                self->GetCellText(di->item.iItem, di->item.iSubItem, di->item.pszText, di->item.cchTextMax);
            }
            return 0;
        }
        // 行の背景色
        if (hdr->code == NM_CUSTOMDRAW) {
            auto* cd = reinterpret_cast<NMLVCUSTOMDRAW*>(lp);
            if (cd->nmcd.dwDrawStage == CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;
            if (cd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
                cd->clrTextBk = self->RowColor(static_cast<int>(cd->nmcd.dwItemSpec));
                return CDRF_NEWFONT;
            }
            return CDRF_DODEFAULT;
        }
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        // 所有者へ通知 (ポインタを無効化してもらう)
        PostMessage(self->m_owner, WM_APP_RESULTLIST_CLOSED, 0, 0);
        if (self->m_font) DeleteObject(self->m_font);
        delete self;
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

ResultList* ResultList::Create(HWND owner, Tracker* tracker, HINSTANCE hInst) {
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icc);

    if (!s_registered) {
        WNDCLASSEXW wcex   = {};
        wcex.cbSize        = sizeof(wcex);
        wcex.lpszClassName = CLASS_NAME;
        wcex.lpfnWndProc   = WndProc;
        wcex.hInstance     = hInst;
        wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        RegisterClassExW(&wcex);
        s_registered = true;
    }

    auto* dlg = new ResultList();
    dlg->m_owner   = owner;
    dlg->m_tracker = tracker;

    UINT dpi = GetDpiForWindow(owner);
    auto DIP = [dpi](int dip) { return utils::FromDIP(dip, dpi); };

    // 所有者の右隣、同じ高さに配置
    RECT orc;
    GetWindowRect(owner, &orc);

    // EditResult と一緒に閉じる
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME;
    RECT rc = { 0, 0, DIP(80 + 90 * 4 + 24), 0 };
    AdjustWindowRectExForDpi(&rc, style, FALSE, 0, dpi);

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"Result List", style,
        orc.right, orc.top, rc.right - rc.left, orc.bottom - orc.top,
        owner, nullptr, hInst, dlg);
    if (!hwnd) {
        delete dlg;
        return nullptr;
    }

    dlg->m_font = CreateFontW(
        -DIP(12), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Yu Gothic UI");

    RECT client;
    GetClientRect(hwnd, &client);

    // 行数が多くても軽いように、仮想リストにする
    dlg->m_listView = CreateWindowExW(
        0, WC_LISTVIEWW, nullptr,
        WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_OWNERDATA | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        0, 0, client.right, client.bottom,
        hwnd, nullptr, hInst, nullptr);
    SendMessage(dlg->m_listView, WM_SETFONT, (WPARAM)dlg->m_font, TRUE);
    ListView_SetExtendedListViewStyle(dlg->m_listView,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

    auto AddColumn = [&](int idx, const wchar_t* text, int colWidth) {
        LVCOLUMNW col = {};
        col.mask    = LVCF_TEXT | LVCF_WIDTH;
        col.pszText = const_cast<LPWSTR>(text);
        col.cx      = DIP(colWidth);
        ListView_InsertColumn(dlg->m_listView, idx, &col);
    };
    AddColumn(0, L"frame", 80);
    AddColumn(1, L"x", 90);
    AddColumn(2, L"y", 90);
    AddColumn(3, L"width", 90);
    AddColumn(4, L"height", 90);

    ListView_SetItemCountEx(dlg->m_listView, dlg->RowCount(), LVSICF_NOINVALIDATEALL);

    ShowWindow(hwnd, SW_SHOWNA);
    UpdateWindow(hwnd);
    return dlg;
}
