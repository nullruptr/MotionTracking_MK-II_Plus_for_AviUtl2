#include "mainframe.hpp"
#include "ownerdraw.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include <winuser.h>
#include <cmath>

extern LOG_HANDLE*    logger;
extern CONFIG_HANDLE* config;

constexpr const wchar_t* track_method[] = { L"MIL", L"KCF", L"CSRT", L"DaSiamRPN", L"Nano", L"Vit" };
constexpr int METHOD_N = sizeof(track_method) / sizeof(track_method[0]);

void MainFrame::CreateControls() {
    // File ボタン（クリックでポップアップメニュー）
    // Options ボタン（クリックで設定ウィンドウ）
    // ボタン幅は均等に並べる（各145px）
    // 色情報の取得
    m_colors.background       = m_colors.background;
    m_colors.buttonBody       = config->get_color_code(config, "ButtonBody");
    m_colors.buttonBodyPress  = config->get_color_code(config, "ButtonBodyPress");
    m_colors.buttonBodyHover  = config->get_color_code(config, "ButtonBodyHover");
    m_colors.buttonBodyDisable = config->get_color_code(config, "ButtonBodyDisable");
    m_colors.text             = config->get_color_code(config, "Text");
    m_colors.textDisable      = config->get_color_code(config, "TextDisable");

    // AviUtl2側が「拡大サイズ表示」の時だけ、config->get_font_info/get_layout_sizeの戻り値が
    // 実DPI/96の比率で既に拡大されて返ってくる。その場合だけ、自前で決め打ちしている
    // 座標・サイズも同じ比率でスケールしないと、文字だけ大きくなりレイアウトが崩れる。
    bool high_dpi = utils::is_high_dpi_mode(m_hInst);
    UINT dpi = GetDpiForWindow(m_hwnd);
    auto DIP = [high_dpi, dpi](int dip) { return high_dpi ? utils::FromDIP(dip, dpi) : dip; };

    // フォント情報の取得とフォント作成
    FONT_INFO* font_info = config->get_font_info(config, "Control");
    LOGFONT logfont = {};
    logfont.lfHeight = -static_cast<int>(font_info->size * 96 / 72);
    logfont.lfCharSet = DEFAULT_CHARSET;
    logfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logfont.lfQuality = DEFAULT_QUALITY;
    logfont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    wcscpy_s(logfont.lfFaceName, LF_FACESIZE, font_info->name);
    HFONT hfont = CreateFontIndirect(&logfont);

    int item_height = config->get_layout_size(config, "SettingItemHeight");
    int y_pos = DIP(10);

    // ラベル等にホバーで説明を出すためのツールチップ
    HWND hwndTip = CreateWindowEx(
        WS_EX_TOPMOST,
        TOOLTIPS_CLASS, nullptr,
        WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        m_hwnd, nullptr, m_hInst, nullptr);
    SendMessage(hwndTip, TTM_SETMAXTIPWIDTH, 0, DIP(300));
    auto AddTooltip = [hwndTip, this](HWND target, const wchar_t* text) {
        TOOLINFOW ti = {};
        ti.cbSize   = sizeof(ti);
        ti.uFlags   = TTF_IDISHWND | TTF_SUBCLASS;
        ti.hwnd     = m_hwnd;
        ti.uId      = reinterpret_cast<UINT_PTR>(target);
        ti.lpszText = const_cast<LPWSTR>(text);
        SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
    };

    // メニューバーが使えなかったので、代替
    // File ボタン
    HWND button_file = CreateWindowEx(
        0, WC_BUTTON, L"File",
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        DIP(10), y_pos, DIP(75), item_height,
        m_hwnd, (HMENU)IDC_Toolbar::File, m_hInst, nullptr);
    SendMessage(button_file, WM_SETFONT, (WPARAM)hfont, TRUE);

    // Info ボタン
    HWND button_info = CreateWindowEx(
        0, WC_BUTTON, L"Info",
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        DIP(95), y_pos, DIP(75), item_height,
        m_hwnd, (HMENU)IDC_Toolbar::Info, m_hInst, nullptr);
    SendMessage(button_info, WM_SETFONT, (WPARAM)hfont, TRUE);

    y_pos += item_height + DIP(30);

    // Tracking Method ラベルを作成
    HWND label_track = CreateWindowEx(
        0,
        WC_STATIC,
        config->translate(config, L"Method"),
        WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP,
        DIP(10), y_pos, DIP(100), item_height,
        m_hwnd,
        (HMENU)-1,
        m_hInst,
        nullptr);
    SendMessage(label_track, WM_SETFONT, (WPARAM)hfont, TRUE);

    // Tracking Method コンボボックスを作成
    HWND combo_track = CreateWindowEx(
        0,
        WC_COMBOBOX,
        nullptr,
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS,
        DIP(95), y_pos, DIP(180), DIP(200), // ドロップダウンが開くように高さを大きめに確保
        m_hwnd,
        (HMENU)IDC_Button::TrackingMethodCombo,
        m_hInst,
        nullptr);
    SendMessage(combo_track, WM_SETFONT, (WPARAM)hfont, TRUE);
    for (int i = 0; i < METHOD_N; i++) {
        SendMessage(combo_track, CB_ADDSTRING, 0, (LPARAM)track_method[i]);
    }
    SendMessage(combo_track, CB_SETCURSEL, 2, 0); // Default to CSRT

    y_pos += item_height + DIP(5);

    // Hueラベルを作成
    HWND label_hue = CreateWindowEx(
        0,
        WC_STATIC,
        config->translate(config, L"Hue"),
        WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP | SS_NOTIFY,
        DIP(10), y_pos, DIP(80), item_height,
        m_hwnd,
        (HMENU)-1,
        m_hInst,
        nullptr);
    SendMessage(label_hue, WM_SETFONT, (WPARAM)hfont, TRUE);
    AddTooltip(label_hue, L"EN: Specifies the hue of the rectangle displayed in Object Selection and View Result.\r\nJA: Object SelectionやView Resultで表示される矩形の色相を指定します。");

    // Hueトラックバーを作成
    HWND trackbar_hue = CreateWindowEx(
        0,
        TRACKBAR_CLASS,
        L"Hue",
        WS_VISIBLE | WS_CHILD,
        DIP(95), y_pos, DIP(180), item_height,
        m_hwnd,
        (HMENU)IDC_Button::HueTrackbar,
        m_hInst,
        nullptr);
    SendMessage(trackbar_hue, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 359));
    SendMessage(trackbar_hue, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)m_hueValue);
    SendMessage(trackbar_hue, WM_SETFONT, (WPARAM)hfont, TRUE);
    AddTooltip(trackbar_hue, L"EN: Specifies the hue of the rectangle displayed in Object Selection and View Result.\r\nJA: Object SelectionやView Resultで表示される矩形の色相を指定します。");

    // Hue数値表示を作成
    HWND hue_value_display = CreateWindowEx(
        0,
        WC_STATIC,
        L"180",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        DIP(275), y_pos, DIP(40), item_height,
        m_hwnd,
        (HMENU)IDC_Button::HueValue,
        m_hInst,
        nullptr);
    SendMessage(hue_value_display, WM_SETFONT, (WPARAM)hfont, TRUE);

    y_pos += item_height + DIP(5);

    // Wnd Scaleラベルを作成
    HWND label_scale = CreateWindowEx(
        0,
        WC_STATIC,
        config->translate(config, L"Wnd Scale"),
        WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP | SS_NOTIFY,
        DIP(10), y_pos, DIP(80), item_height,
        m_hwnd,
        (HMENU)-1,
        m_hInst,
        nullptr);
    SendMessage(label_scale, WM_SETFONT, (WPARAM)hfont, TRUE);
    AddTooltip(label_scale, L"EN: This is the window scale of Select Object / View Result window. \r\nYou can select a value from 0.00 to 1.00. If you select track bar as far right(--), it becomes disabled.\r\nThe trackbar in View Result can become extremely small, because this plugin forcibly overrides the window size from outside. Please be careful when using this in View Result.\r\nJA: Select Object / View Result で表示されるウィンドウの表示倍率を設定します。\r\n0.00~1.00 の範囲で指定できます。一番右(--)で無効になります。\r\n外部から強制的に画面サイズを変更している関係上、View Result でトラックバーが極端に小さくなる不具合があります。View Result で利用する場合は、お気を付けください。");

    // Scaleトラックバーを作成
    // 0~100 が倍率 0.00~1.00、101 は「無効」(既定倍率を使用)を表す
    HWND trackbar_scale = CreateWindowEx(
        0,
        TRACKBAR_CLASS,
        L"Scale",
        WS_VISIBLE | WS_CHILD,
        DIP(95), y_pos, DIP(180), item_height,
        m_hwnd,
        (HMENU)IDC_Button::ScaleTrackbar,
        m_hInst,
        nullptr);
    SendMessage(trackbar_scale, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 101));
    SendMessage(trackbar_scale, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)std::lround(m_wndScale * 100));
    SendMessage(trackbar_scale, WM_SETFONT, (WPARAM)hfont, TRUE);
    AddTooltip(trackbar_scale, L"EN: This is the window scale of Select Object / View Result window. \r\nYou can select a value from 0.00 to 1.00. If you select track bar as far right(--), it becomes disabled.\r\nThe trackbar in View Result can become extremely small, because this plugin forcibly overrides the window size from outside. Please be careful when using this in View Result.\r\nJA: Select Object / View Result で表示されるウィンドウの表示倍率を設定します。\r\n0.00~1.00 の範囲で指定できます。一番右(--)で無効になります。\r\n外部から強制的に画面サイズを変更している関係上、View Result でトラックバーが極端に小さくなる不具合があります。View Result で利用する場合は、お気を付けください。");

    // Scale倍率表示を作成
    HWND hue_value_scale = CreateWindowEx(
        0,
        WC_STATIC,
        L"--",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        DIP(275), y_pos, DIP(40), item_height,
        m_hwnd,
        (HMENU)IDC_Button::WndScale,
        m_hInst,
        nullptr);
    SendMessage(hue_value_scale, WM_SETFONT, (WPARAM)hfont, TRUE);

    y_pos += item_height + DIP(5);

    // Select Object ボタンを作成
    HWND button0 = CreateWindowEx(
        0,
        WC_BUTTON,
        config->translate(config, L"Select Object"),
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        DIP(10), y_pos, DIP(300), item_height,
        m_hwnd,
        (HMENU)IDC_Button::SelectObject,
        m_hInst,
        nullptr);
    SendMessage(button0, WM_SETFONT, (WPARAM)hfont, TRUE);

    y_pos += item_height + DIP(5);

    // Analyze ボタンを作成
    HWND button1 = CreateWindowEx(
        0,
        WC_BUTTON,
        config->translate(config, L"Analyze"),
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        DIP(10), y_pos, DIP(300), item_height,
        m_hwnd,
        (HMENU)IDC_Button::Analyze,
        m_hInst,
        nullptr);
    SendMessage(button1, WM_SETFONT, (WPARAM)hfont, TRUE);

    y_pos += item_height + DIP(5);

    // View Result ボタンを作成
    HWND button_view_result = CreateWindowEx(
        0,
        WC_BUTTON,
        config->translate(config, L"View Result"),
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        DIP(10), y_pos, DIP(300), item_height,
        m_hwnd,
        (HMENU)IDC_Button::ViewResult,
        m_hInst,
        nullptr);
    SendMessage(button_view_result, WM_SETFONT, (WPARAM)hfont, TRUE);

    y_pos += item_height + DIP(5);

    // Clear Result ボタンを作成
    HWND button2 = CreateWindowEx(
        0,
        WC_BUTTON,
        config->translate(config, L"Clear Result"),
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        DIP(10), y_pos, DIP(300), item_height,
        m_hwnd,
        (HMENU)IDC_Button::ClearResult,
        m_hInst,
        nullptr);
    SendMessage(button2, WM_SETFONT, (WPARAM)hfont, TRUE);

    y_pos += item_height + DIP(5);

    // As Sub-filter/部分フィルター? チェックボックスを作成
    HWND check_sub_filter = CreateWindowEx(
        0,
        WC_BUTTON,
        config->translate(config, L"As Sub-filter/部分フィルタ?"),
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        DIP(10), y_pos, DIP(300), item_height,
        m_hwnd,
        (HMENU)IDC_Button::AsSubFilter,
        m_hInst,
        nullptr);
    SendMessage(check_sub_filter, WM_SETFONT, (WPARAM)hfont, TRUE);

    y_pos += item_height + DIP(5);

    // Invert Position チェックボックスを作成
    HWND check_invert = CreateWindowEx(
        0,
        WC_BUTTON,
        config->translate(config, L"Invert Position"),
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        DIP(10), y_pos, DIP(300), item_height,
        m_hwnd,
        (HMENU)IDC_Button::InvertPosition,
        m_hInst,
        nullptr);
    SendMessage(check_invert, WM_SETFONT, (WPARAM)hfont, TRUE);

    y_pos += item_height + DIP(5);

    // Ignore Aspect Ratio チェックボックスを作成
    HWND check_ignore_aspect = CreateWindowEx(
        0,
        WC_BUTTON,
        config->translate(config, L"Ignore Aspect Ratio"),
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        DIP(10), y_pos, DIP(300), item_height,
        m_hwnd,
        (HMENU)IDC_Button::IgnoreAspectRatio,
        m_hInst,
        nullptr);
    SetWindowLongPtr(check_ignore_aspect, GWLP_USERDATA, 1);
    SendMessage(check_ignore_aspect, WM_SETFONT, (WPARAM)hfont, TRUE);

    y_pos += item_height + DIP(5);

    // Insert Object ボタンを作成
    HWND button_save = CreateWindowEx(
        0,
        WC_BUTTON,
        config->translate(config, L"Insert Object"),
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        DIP(10), y_pos, DIP(300), item_height,
        m_hwnd,
        (HMENU)IDC_Button::InsertObject,
        m_hInst,
        nullptr);
    SendMessage(button_save, WM_SETFONT, (WPARAM)hfont, TRUE);

    y_pos += item_height + DIP(5);
}
