#include "numeric_data.hpp"
#include "utils.hpp"
#include "constants.hpp"
#include <commctrl.h>
#include <format>
#include <string>

static constexpr wchar_t CLASS_NAME[] = L"MotionTracker_NumericDataDlg";

bool NumericDataDlg::s_registered = false;

LRESULT CALLBACK NumericDataDlg::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    NumericDataDlg* self = reinterpret_cast<NumericDataDlg*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_NCCREATE:
        self = reinterpret_cast<NumericDataDlg*>(
            reinterpret_cast<CREATESTRUCT*>(lp)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hwnd = hwnd;
        return DefWindowProc(hwnd, msg, wp, lp);

    case WM_SIZE: {
        // ウィンドウのリサイズにListViewを追従させる
        RECT client;
        GetClientRect(hwnd, &client);
        SetWindowPos(self->m_listView, nullptr, 0, 0, client.right, client.bottom, SWP_NOZORDER);
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        // 本体ウィンドウへ破棄を通知し、無効化していた操作系ボタンを再度有効化してもらう
        PostMessage(self->m_parent, WM_APP_NUMERIC_DATA_CLOSED, 0, 0);
        delete self;
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

void NumericDataDlg::Create(
    HWND parent, HINSTANCE hInst, int rangeStart,
    const std::vector<double>& x, const std::vector<double>& y,
    const std::vector<double>& width, const std::vector<double>& height,
    const std::vector<double>& smoothX, const std::vector<double>& smoothY,
    const std::vector<double>& smoothWidth, const std::vector<double>& smoothHeight)
{
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icc);

    if (!s_registered) {
        WNDCLASSEXW wcex    = {};
        wcex.cbSize         = sizeof(wcex);
        wcex.lpszClassName  = CLASS_NAME;
        wcex.lpfnWndProc    = WndProc;
        wcex.hInstance      = hInst;
        wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
        RegisterClassExW(&wcex);
        s_registered = true;
    }

    auto* dlg = new NumericDataDlg();
    dlg->m_parent = parent;

    // 独立ウィンドウのため、progress_dlg同様に自前でDPIスケールする
    UINT dpi = GetDpiForWindow(parent);
    auto DIP = [dpi](int dip) { return utils::FromDIP(dip, dpi); };

    bool hasSmooth = !smoothX.empty();
    int  winW = hasSmooth ? 900 : 370;
    int  winH = 500;

    HWND hwnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        CLASS_NAME,
        L"Numeric Data",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, DIP(winW), DIP(winH),
        parent, nullptr, hInst, dlg);

    HFONT hfont = CreateFontW(
        -DIP(12), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Yu Gothic UI");

    RECT client;
    GetClientRect(hwnd, &client);

    dlg->m_listView = CreateWindowExW(
        0, WC_LISTVIEWW, nullptr,
        WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SINGLESEL,
        0, 0, client.right, client.bottom,
        hwnd, nullptr, hInst, nullptr);
    SendMessage(dlg->m_listView, WM_SETFONT, (WPARAM)hfont, TRUE);
    ListView_SetExtendedListViewStyle(dlg->m_listView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

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
    if (hasSmooth) {
        AddColumn(5,  L"smooth_x", 100);
        AddColumn(6,  L"smooth_y", 100);
        AddColumn(7,  L"smooth_width", 100);
        AddColumn(8,  L"smooth_height", 100);
        AddColumn(9,  L"diff_x", 90);
        AddColumn(10, L"diff_y", 90);
        AddColumn(11, L"diff_width", 90);
        AddColumn(12, L"diff_height", 90);
    }

    for (size_t i = 0; i < x.size(); i++) {
        std::wstring frameText = std::to_wstring(rangeStart + (int)i);
        LVITEMW item  = {};
        item.mask     = LVIF_TEXT;
        item.iItem    = (int)i;
        item.pszText  = const_cast<LPWSTR>(frameText.c_str());
        ListView_InsertItem(dlg->m_listView, &item);

        auto SetCell = [&](int col, double value) {
            std::wstring text = std::format(L"{:.2f}", value);
            ListView_SetItemText(dlg->m_listView, (int)i, col, const_cast<LPWSTR>(text.c_str()));
        };
        SetCell(1, x[i]);
        SetCell(2, y[i]);
        SetCell(3, width[i]);
        SetCell(4, height[i]);
        if (hasSmooth) {
            SetCell(5,  smoothX[i]);
            SetCell(6,  smoothY[i]);
            SetCell(7,  smoothWidth[i]);
            SetCell(8,  smoothHeight[i]);
            SetCell(9,  x[i] - smoothX[i]);
            SetCell(10, y[i] - smoothY[i]);
            SetCell(11, width[i] - smoothWidth[i]);
            SetCell(12, height[i] - smoothHeight[i]);
        }
    }

    // 親ウィンドウの中央に配置
    RECT pr, wr;
    GetWindowRect(parent, &pr);
    GetWindowRect(hwnd, &wr);
    int w = wr.right - wr.left;
    int h = wr.bottom - wr.top;
    SetWindowPos(hwnd, HWND_TOP,
        pr.left + (pr.right - pr.left - w) / 2,
        pr.top  + (pr.bottom - pr.top  - h) / 2,
        0, 0, SWP_NOSIZE);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
}
