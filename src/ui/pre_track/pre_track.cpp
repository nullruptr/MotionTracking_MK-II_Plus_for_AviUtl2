#include "pre_track.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include <commctrl.h>
#include <initializer_list>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

static constexpr wchar_t CLASS_NAME[] = L"PreTrack";

enum : int {
    ID_BGS_RANGE = 100,
    ID_BGS_NMIX,
    ID_BGS_BGRATIO,
    ID_BGS_D2T,
    ID_BGS_SHADOW = 120,
    ID_BGS_MODE   = 121,
    ID_HSV_MODE   = 130,
    ID_BGS_ENABLE = 140,
    ID_HSV_ENABLE = 141,
    ID_PREVIEW    = 150,
    ID_VALUE_OFFSET = 1000,
};

bool PreTrack::s_registered = false;

// BGS 処理
enum class bgs_id : int {
    mog2 = 0,
    knn = 1,
    mog2_mask = 2,
    knn_mask = 3
};

// plugin2.h より、任意フレーム画像を取得する処理
static cv::Mat get_scene_frame(EDIT_HANDLE* edit_handle, int f) {
    cv::Mat img;
    bool render = edit_handle->rendering_scene_video (
        f,
        &img,
        [](void* param, int frame, const void* buffer, int width, int height, int pitch) {
            cv::Mat* out = static_cast<cv::Mat*>(param);
            cv::Mat rgba(height, width, CV_8UC4, const_cast<void*>(buffer), static_cast<size_t>(pitch));
            cv::cvtColor(rgba, *out, cv::COLOR_RGBA2BGR);
        });

    if (!render) {
        return cv::Mat();
    }

    edit_handle->wait_rendering_task();
    return img;
}

//MOG2 and KNN Background subtraction
static cv::Mat bgs_proc(const PreTrackSettings& s, EDIT_HANDLE* edit_handle, int obj_frame_s, int obj_frame_e, int cur_frame) {
    bool isShadow = false;
    if (s.bgs_shadow) {
        isShadow = true;
    }
    //boundary correction
    int frame_s, frame_e;

    // パラメータ の int
    int range_value   = s.bgs_range;
    int nmix_value    = s.bgs_nmix;
    int bgratio_value = s.bgs_bgratio;
    int d2t_value     = s.bgs_d2t;

    // Range 切り詰め処理
    // ex. オブジェクト範囲 100~200 で、cur_frame が 120 なら、手前10切り詰める。
    // 後ろも同様にrange超過分は切り詰める
    frame_s = cur_frame - range_value;
    if (frame_s < obj_frame_s) frame_s = obj_frame_s;
    frame_e = cur_frame + range_value;
    if (frame_e > obj_frame_e) frame_e = obj_frame_e;

    //Setup OCV subtracttor
    cv::Mat mask;
    const auto check = static_cast<bgs_id>(s.bgs_mode);
    cv::Ptr<cv::BackgroundSubtractor> bsub;
    if (check == bgs_id::mog2 || check == bgs_id::mog2_mask) {
        auto mog2 = cv::createBackgroundSubtractorMOG2();
        mog2->setDetectShadows(isShadow);
        mog2->setHistory(range_value);
        mog2->setNMixtures(nmix_value);
        mog2->setBackgroundRatio((double)bgratio_value / 100.0);
        bsub = mog2;
    } else {
        auto knn = cv::createBackgroundSubtractorKNN();
        knn->setDetectShadows(isShadow);
        knn->setHistory(range_value*2);
        knn->setDist2Threshold(d2t_value);
        bsub = knn;
    }
    for (int f = frame_s; f <= cur_frame - 1; f++) {// Scan all frames in range
        cv::Mat img = get_scene_frame(edit_handle, f);
        if (img.empty()) return cv::Mat(); // 画像がないとき抜ける
        bsub->apply(img, mask);
    }
    cv::Mat cur_img = get_scene_frame(edit_handle, cur_frame);
    bsub->apply(cur_img, mask);
    cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::erode(mask, mask, element);
    cv::dilate(mask, mask, element);
    cv::Mat rgba;
    if (check == bgs_id::mog2_mask || check == bgs_id::knn_mask) {
        cv::cvtColor(mask, rgba, cv::COLOR_GRAY2RGBA);
    } else {
        cv::Mat tempbuf;
        cur_img.copyTo(tempbuf, mask);
        cv::cvtColor(tempbuf, rgba, cv::COLOR_BGR2RGBA);
    }
    return rgba;
}

// HSV 処理
enum class hsv_id : int {
    hsv = 0,
    hsv_aviutl = 1,
    hue_only = 2,
    saturation_only = 3,
    value_only = 4
};

//HSV Conversion filter
// ocvImg_alpha は RGBA の入力画像
static cv::Mat hsv_proc(const PreTrackSettings& s, const cv::Mat& ocvImg_alpha) {
    const auto check = static_cast<hsv_id>(s.hsv_mode);

    cv::Mat ocvImg;
    cv::cvtColor(ocvImg_alpha, ocvImg, cv::COLOR_RGBA2RGB); // alpha 除去
    cv::Mat outImg;
    cv::cvtColor(ocvImg, outImg, cv::COLOR_RGB2HSV_FULL);
    if (check == hsv_id::hue_only) //Hue
    {
        std::vector<cv::Mat> channels;
        cv::split(outImg, channels);
        cv::cvtColor(channels[0], outImg, cv::COLOR_GRAY2BGR);
        channels.clear();
    }
    else if (check == hsv_id::saturation_only) //Sat
    {
        std::vector<cv::Mat> channels;
        cv::split(outImg, channels);
        cv::cvtColor(channels[1], outImg, cv::COLOR_GRAY2BGR);
        channels.clear();
    }
    else if (check == hsv_id::value_only) //Value
    {
        std::vector<cv::Mat> channels;
        cv::split(outImg, channels);
        cv::cvtColor(channels[2], outImg, cv::COLOR_GRAY2BGR);
        channels.clear();
    }

    cv::Mat outImg_alpha;
    // AviUtl2: R=H, G=S, B=V
    // AviUtl : R=V, G=S, B=H
    // AviUtl が BGR バッファに入れていたようなので、そのための対応
    if (check == hsv_id::hsv_aviutl) {cv::cvtColor(outImg, outImg, cv::COLOR_RGB2BGR);}
    cv::cvtColor(outImg, outImg_alpha, cv::COLOR_RGB2RGBA); // alpha 追加
    return outImg_alpha;
}

// 設定を適用したフレーム画像を返す。失敗時は空
cv::Mat pretrack_apply(const PreTrackSettings& s, EDIT_HANDLE* edit_handle, int range_s, int range_e, int cur_frame) {
    cv::Mat rgba;
    if (s.bgs_enable) {
        rgba = bgs_proc(s, edit_handle, range_s, range_e, cur_frame);
    } else {
        cv::Mat img = get_scene_frame(edit_handle, cur_frame);
        if (!img.empty()) cv::cvtColor(img, rgba, cv::COLOR_BGR2RGBA);
    }
    if (rgba.empty()) return cv::Mat();
    if (s.hsv_enable) rgba = hsv_proc(s, rgba);

    cv::Mat bgr;
    cv::cvtColor(rgba, bgr, cv::COLOR_RGBA2BGR);
    return bgr;
}

// コントロールの現在値を読む
PreTrackSettings PreTrack::Read() const {
    PreTrackSettings s;
    auto get = [this](int id, UINT msg) { return static_cast<int>(SendDlgItemMessage(m_hwnd, id, msg, 0, 0)); };
    s.bgs_enable  = get(ID_BGS_ENABLE, BM_GETCHECK) == BST_CHECKED;
    s.hsv_enable  = get(ID_HSV_ENABLE, BM_GETCHECK) == BST_CHECKED;
    s.bgs_range   = get(ID_BGS_RANGE, TBM_GETPOS);
    s.bgs_shadow  = get(ID_BGS_SHADOW, BM_GETCHECK) == BST_CHECKED;
    s.bgs_nmix    = get(ID_BGS_NMIX, TBM_GETPOS);
    s.bgs_bgratio = get(ID_BGS_BGRATIO, TBM_GETPOS);
    s.bgs_d2t     = get(ID_BGS_D2T, TBM_GETPOS);
    s.bgs_mode    = get(ID_BGS_MODE, CB_GETCURSEL);
    s.hsv_mode    = get(ID_HSV_MODE, CB_GETCURSEL);
    return s;
}

// OK で確定: Tracker に保存
void PreTrack::Save() {
    m_tracker->PreTrackParams() = Read();
}

// プレビュー
void PreTrack::Preview() {
    // フレーム選択範囲を取得
    RangeResult range = m_tracker->GetSelectRange(m_edit_handle);

    const char* win = "Pre-track Preview";
    cv::namedWindow(win, cv::WINDOW_NORMAL);
    cv::createTrackbar("Frame", win, nullptr, std::max(1, range.end - range.start));

    m_previewing = true;
    EnableWindow(GetDlgItem(m_hwnd, ID_PREVIEW), FALSE);

    // 前回の状態。変わったときだけ再描画する
    std::vector<int> prev;
    bool resized = false;
    // OpenCV の例外が WndProc の外に出るとクラッシュするので、ここで受ける
    try {
    while (true) {
        cv::waitKey(10);
        // × で閉じるまで待機
        if (m_closeReq || !static_cast<bool>(cv::getWindowProperty(win, cv::WND_PROP_VISIBLE))) {
            break;
        }

        int pos = cv::getTrackbarPos("Frame", win);
        bool bgs_on = SendDlgItemMessage(m_hwnd, ID_BGS_ENABLE, BM_GETCHECK, 0, 0) == BST_CHECKED;
        bool hsv_on = SendDlgItemMessage(m_hwnd, ID_HSV_ENABLE, BM_GETCHECK, 0, 0) == BST_CHECKED;
        std::vector<int> cur = {
            pos, bgs_on, hsv_on,
            static_cast<int>(SendDlgItemMessage(m_hwnd, ID_BGS_RANGE, TBM_GETPOS, 0, 0)),
            static_cast<int>(SendDlgItemMessage(m_hwnd, ID_BGS_NMIX, TBM_GETPOS, 0, 0)),
            static_cast<int>(SendDlgItemMessage(m_hwnd, ID_BGS_BGRATIO, TBM_GETPOS, 0, 0)),
            static_cast<int>(SendDlgItemMessage(m_hwnd, ID_BGS_D2T, TBM_GETPOS, 0, 0)),
            static_cast<int>(SendDlgItemMessage(m_hwnd, ID_BGS_SHADOW, BM_GETCHECK, 0, 0)),
            static_cast<int>(SendDlgItemMessage(m_hwnd, ID_BGS_MODE, CB_GETCURSEL, 0, 0)),
            static_cast<int>(SendDlgItemMessage(m_hwnd, ID_HSV_MODE, CB_GETCURSEL, 0, 0)),
        };
        if (cur == prev) continue;
        prev = cur;

        int cur_frame = std::min(range.start + pos, range.end);
        cv::Mat bgr = pretrack_apply(Read(), m_edit_handle, range.start, range.end, cur_frame);
        if (bgr.empty()) continue;
        cv::imshow(win, bgr);

        // Wnd Scale 適用
        if (!resized) {
            utils::ResizeWindow(FindWindowA(nullptr, win), m_edit_handle, m_wndScale);
            resized = true;
        }
    }
    } catch (const cv::Exception& e) {
        MessageBoxA(m_hwnd, e.what(), "Preview Error", MB_OK | MB_ICONERROR);
    }

    // × で閉じられた後に destroyWindow すると例外になるので、握りつぶす
    try { cv::destroyWindow(win); } catch (const cv::Exception&) {}
    EnableWindow(GetDlgItem(m_hwnd, ID_PREVIEW), TRUE);
    m_previewing = false;
}

LRESULT CALLBACK PreTrack::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    PreTrack* self = reinterpret_cast<PreTrack*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_NCCREATE:
        self = reinterpret_cast<PreTrack*>(
            reinterpret_cast<CREATESTRUCT*>(lp)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hwnd = hwnd;
        return DefWindowProc(hwnd, msg, wp, lp);

    case WM_HSCROLL: {
        // 値ラベル更新
        HWND bar = reinterpret_cast<HWND>(lp);
        int id = GetDlgCtrlID(bar);
        if (id >= ID_BGS_RANGE && id <= ID_BGS_D2T) {
            int pos = static_cast<int>(SendMessage(bar, TBM_GETPOS, 0, 0));
            SetWindowTextW(GetDlgItem(hwnd, id + ID_VALUE_OFFSET), std::to_wstring(pos).c_str());
        }
        return 0;
    }

    case WM_COMMAND:
        if (HIWORD(wp) != BN_CLICKED) return 0;
        if (LOWORD(wp) == ID_PREVIEW && !self->m_previewing) {
            self->Preview();
            // プレビュー中に閉じる要求があったら、ここで閉じる
            if (self->m_closeReq) {
                if (self->m_saveReq) self->Save();
                DestroyWindow(hwnd);
                return 0;
            }
            SetFocus(nullptr);
        } else if (LOWORD(wp) == IDOK || LOWORD(wp) == IDCANCEL) {
            bool ok = LOWORD(wp) == IDOK;
            if (self->m_previewing) {
                // プレビュー中は、Preview が終わってから閉じる
                self->m_closeReq = true;
                self->m_saveReq  = ok;
            } else {
                if (ok) self->Save();
                DestroyWindow(hwnd);
            }
        }
        return 0;

    case WM_CLOSE:
        // x = Cancel
        if (self->m_previewing) {
            self->m_closeReq = true;
            return 0;
        }
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        // 本体へ通知し、コントロールを有効化する。
        PostMessage(self->m_parent, WM_APP_PRETRACK_CLOSED, 0, 0);
        if (self->m_font) DeleteObject(self->m_font);
        delete self;
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

PreTrack* PreTrack::Create(HWND parent, Tracker* tracker, HINSTANCE hInst, EDIT_HANDLE* edit_handle, double wndScale) {
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

    auto* dlg = new PreTrack();
    dlg->m_tracker    = tracker;
    dlg->m_edit_handle = edit_handle;
    dlg->m_wndScale    = wndScale;
    dlg->m_parent     = parent;

    // DPI 処理
    UINT dpi = GetDpiForWindow(parent);
    auto DIP = [dpi](int dip) { return utils::FromDIP(dip, dpi); };

    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);

    // レイアウト
    constexpr int MARGIN = 12, COL_W = 290, GAP = 16, ROW_H = 30;
    constexpr int CLIENT_W = MARGIN * 2 + COL_W * 2 + GAP;
    constexpr int BTN_W = 75, BTN_H = 23, BTN_GAP = 6;
    constexpr int CLIENT_H = MARGIN * 2 + ROW_H * 7 + BTN_H;

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
    DWORD exStyle = WS_EX_APPWINDOW;
    RECT rc = { 0, 0, DIP(CLIENT_W), DIP(CLIENT_H) };
    AdjustWindowRectExForDpi(&rc, style, FALSE, exStyle, dpi);

    HWND hwnd = CreateWindowExW(
        exStyle,
        CLASS_NAME,
        L"Pre-track",
        style,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
        parent, nullptr, hInst, dlg);
    if (!hwnd) {
        delete dlg;
        return nullptr;
    }

    dlg->m_font = CreateFontW(
        -DIP(12), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Yu Gothic UI");

    // 子コントロール生成
    auto child = [&](const wchar_t* cls, const wchar_t* text, DWORD st,
                     int x, int y, int w, int h, int id) {
        return CreateWindowExW(0, cls, text, WS_VISIBLE | WS_CHILD | st,
            DIP(x), DIP(y), DIP(w), DIP(h),
            hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), hInst, nullptr);
    };
    // タイトル兼CHECKBOX
    auto heading = [&](int x, const wchar_t* text, int id, bool checked) {
        HWND chk = child(WC_BUTTON, text, BS_AUTOCHECKBOX | WS_TABSTOP, x, MARGIN, COL_W, 22, id);
        SendMessage(chk, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
    };
    // トラックバー
    auto track_row = [&](int x, int y, const wchar_t* name, int id, int vmin, int vmax, int vdef) {
        child(WC_STATIC, name, SS_LEFT, x, y + 4, 56, 20, 0);
        HWND bar = child(TRACKBAR_CLASS, nullptr, TBS_HORZ | TBS_NOTICKS | WS_TABSTOP,
                         x + 56, y, COL_W - 56 - 44, 26, id);
        SendMessage(bar, TBM_SETRANGE, TRUE, MAKELPARAM(vmin, vmax));
        SendMessage(bar, TBM_SETPOS, TRUE, vdef);
        child(WC_STATIC, std::to_wstring(vdef).c_str(), SS_RIGHT,
              x + COL_W - 40, y + 4, 40, 20, id + ID_VALUE_OFFSET);
    };
    // コンボボックス
    auto combo_row = [&](int x, int y, const wchar_t* name, int id,
                         std::initializer_list<const wchar_t*> items, int def) {
        child(WC_STATIC, name, SS_LEFT, x, y + 4, 56, 20, 0);
        HWND cb = child(WC_COMBOBOX, nullptr, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                        x + 56, y, COL_W - 56, 200, id);
        for (auto s : items) SendMessage(cb, CB_ADDSTRING, 0, (LPARAM)s);
        SendMessage(cb, CB_SETCURSEL, def, 0);
    };

    // 前回 OK で確定した値
    const PreTrackSettings& s = tracker->PreTrackParams();

    // 左列: BGS
    {
        const int x = MARGIN;
        int y = MARGIN;
        heading(x, constants::bgs, ID_BGS_ENABLE, s.bgs_enable);
        y += ROW_H;
        track_row(x, y, L"Range", ID_BGS_RANGE,   1, 250, s.bgs_range); y += ROW_H;
        HWND shadow = child(WC_BUTTON, L"Shadow", BS_AUTOCHECKBOX | WS_TABSTOP, x, y + 2, COL_W, 22, ID_BGS_SHADOW);
        SendMessage(shadow, BM_SETCHECK, s.bgs_shadow ? BST_CHECKED : BST_UNCHECKED, 0);
        y += ROW_H;
        track_row(x, y, L"NMix",  ID_BGS_NMIX,    1,  30, s.bgs_nmix); y += ROW_H;
        track_row(x, y, L"BG%",   ID_BGS_BGRATIO, 1,  99, s.bgs_bgratio); y += ROW_H;
        track_row(x, y, L"d2T",   ID_BGS_D2T,   100, 1000, s.bgs_d2t); y += ROW_H;
        combo_row(x, y, L"Mode",  ID_BGS_MODE,
                  { L"MOG2", L"KNN", L"MOG2(Mask)", L"KNN(Mask)" }, s.bgs_mode);
    }

    // 右列: HSV
    {
        const int x = MARGIN + COL_W + GAP;
        int y = MARGIN;
        heading(x, constants::hsv, ID_HSV_ENABLE, s.hsv_enable);
        y += ROW_H;
        combo_row(x, y, L"Mode", ID_HSV_MODE,
                  { L"HSV", L"HSV (AviUtl)", L"Hue only", L"Saturation only", L"Value only" }, s.hsv_mode);
    }

    // 最下部: Preview, OK, Cancel は右寄せ。サイズは標準ボタン
    const int btn_y    = MARGIN + ROW_H * 7;
    const int cancel_x = CLIENT_W - MARGIN - BTN_W;
    const int ok_x     = cancel_x - BTN_GAP - BTN_W;
    const int prev_x   = ok_x - BTN_GAP - BTN_W;
    child(WC_BUTTON, L"Preview", BS_PUSHBUTTON | WS_TABSTOP, prev_x, btn_y, BTN_W, BTN_H, ID_PREVIEW);
    child(WC_BUTTON, L"OK", BS_PUSHBUTTON | WS_TABSTOP, ok_x, btn_y, BTN_W, BTN_H, IDOK);
    child(WC_BUTTON, L"Cancel", BS_PUSHBUTTON | WS_TABSTOP, cancel_x, btn_y, BTN_W, BTN_H, IDCANCEL);

    // フォント適用
    EnumChildWindows(hwnd, [](HWND ctrl, LPARAM font) -> BOOL {
        SendMessage(ctrl, WM_SETFONT, static_cast<WPARAM>(font), TRUE);
        return TRUE;
    }, reinterpret_cast<LPARAM>(dlg->m_font));

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

    return dlg;
}
