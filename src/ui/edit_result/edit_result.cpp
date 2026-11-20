#include "edit_result.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include "ui/result_list/result_list.hpp"
#include <commctrl.h>
#include <windowsx.h>
#include <algorithm>
#include <string>
#include <opencv2/imgproc.hpp>

static constexpr wchar_t CLASS_NAME[] = L"MotionTracker_EditResult";
static constexpr UINT    TIMER_PLAY   = 1;

// ボタンの並び
enum Btn : int {
    BTN_TO_START, BTN_PREV_EDGE, BTN_PLAY_BACK, BTN_STEP_BACK, BTN_STEP_FWD,
    BTN_PLAY_FWD, BTN_NEXT_EDGE, BTN_TO_END, BTN_MODE,
    BTN_TRACK_LEFT, BTN_TRACK_LEFT1, BTN_TRACK_RIGHT1, BTN_TRACK_RIGHT, BTN_COUNT
};
static constexpr int ID_BTN_FIRST = 300;
static constexpr int ID_SPEED     = 320;
static constexpr int ID_METHOD    = 321;

static const wchar_t* BTN_TEXT[BTN_COUNT] = {
    L"|◀◀", L"|◀T", L"◀", L"◀|", L"|▶", L"▶", L"T▶|", L"▶▶|", L"->",
    L"◀T", L"◀T|", L"|T▶", L"T▶"
};

bool EditResult::s_registered = false;

int EditResult::TopHeight() const {
    return utils::FromDIP(32, m_dpi);
}

int EditResult::TrackbarHeight() const {
    return utils::FromDIP(32, m_dpi);
}

int EditResult::ButtonRowHeight() const {
    return utils::FromDIP(36, m_dpi);
}

// スライダーとボタンの合計の高さ
int EditResult::BottomHeight() const {
    return TrackbarHeight() + ButtonRowHeight();
}

int EditResult::LabelWidth() const {
    return utils::FromDIP(64, m_dpi);
}

int EditResult::LastPos() const {
    return std::max(0, static_cast<int>(m_tracker->Results().size()) - 1);
}

// 再生のタイマー間隔。FPS と倍率から計算
int EditResult::TimerInterval() const {
    return std::max(1, static_cast<int>(1000.0 / (m_fps * m_speed)));
}

// 指定位置のフレーム画像を取得
void EditResult::ShowFrame(int pos) {
    m_pos = pos;
    int frame = m_tracker->RangeStart() + pos;

    cv::Mat img;
    m_edit_handle->rendering_scene_video(frame, &img,
        [](void* param, int, const void* buffer, int w, int h, int pitch) {
            cv::Mat rgba(h, w, CV_8UC4, const_cast<void*>(buffer), static_cast<size_t>(pitch));
            cv::cvtColor(rgba, *static_cast<cv::Mat*>(param), cv::COLOR_RGBA2BGRA);
        });
    m_edit_handle->wait_rendering_task();

    if (!img.empty()) m_frame = img;
    SetWindowTextW(m_frameLabel, std::to_wstring(frame).c_str());
    if (m_list) m_list->SetCurrent(pos);
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

// スライダーを動かして、フレームを表示
void EditResult::Goto(int pos) {
    pos = std::clamp(pos, 0, LastPos());
    SendMessage(m_trackbar, TBM_SETPOS, TRUE, pos);
    ShowFrame(pos);
}

// 再生・解析を開始したボタンを ■ にし、それ以外のボタンを無効にする
// 再生中は、折り返して逆方向に進んでいても、■ は開始したボタンのまま
// 再生中は、モードのボタンだけ変更できる
void EditResult::UpdateButtons() {
    SetWindowTextW(m_buttons[BTN_PLAY_BACK], m_playOrigin < 0 ? L"■" : BTN_TEXT[BTN_PLAY_BACK]);
    SetWindowTextW(m_buttons[BTN_PLAY_FWD],  m_playOrigin > 0 ? L"■" : BTN_TEXT[BTN_PLAY_FWD]);
    SetWindowTextW(m_buttons[BTN_TRACK_LEFT],  m_trackDir < 0 ? L"■" : BTN_TEXT[BTN_TRACK_LEFT]);
    SetWindowTextW(m_buttons[BTN_TRACK_RIGHT], m_trackDir > 0 ? L"■" : BTN_TEXT[BTN_TRACK_RIGHT]);

    int active = -1;
    if (m_playOrigin != 0) active = m_playOrigin < 0 ? BTN_PLAY_BACK : BTN_PLAY_FWD;
    else if (m_trackDir != 0) active = m_trackDir < 0 ? BTN_TRACK_LEFT : BTN_TRACK_RIGHT;

    for (int i = 0; i < BTN_COUNT; i++) {
        bool enable = active < 0 || i == active || (m_playOrigin != 0 && i == BTN_MODE);
        EnableWindow(m_buttons[i], enable);
    }
}

// 今のフレームから dir 方向へ、解析を続ける
void EditResult::StartTrack(int dir) {
    m_trackDir = dir;
    UpdateButtons();
    // TODO: 解析処理
}

void EditResult::StopTrack() {
    m_trackDir = 0;
    UpdateButtons();
    // TODO: 解析の中止
}

// 1フレーム分だけ dir 方向へ解析する
void EditResult::TrackStep(int dir) {
    // TODO: 解析処理
    (void)dir;
}

void EditResult::StartPlay(int dir) {
    m_playDir    = dir;
    m_playOrigin = dir;
    SetTimer(m_hwnd, TIMER_PLAY, TimerInterval(), nullptr);
    UpdateButtons();
}

void EditResult::StopPlay() {
    if (m_playDir == 0) return;
    m_playDir    = 0;
    m_playOrigin = 0;
    KillTimer(m_hwnd, TIMER_PLAY);
    UpdateButtons();
}

// 再生中の1フレーム分の処理
void EditResult::StepPlay() {
    int next = m_pos + m_playDir;
    if (next < 0 || next > LastPos()) {
        if (LastPos() == 0) {
            StopPlay();
            return;
        }
        switch (m_mode) {
        case PlayMode::Once:
            StopPlay();
            return;
        case PlayMode::Bounce:
            // 端で折り返す
            m_playDir = -m_playDir;
            next = m_pos + m_playDir;
            break;
        case PlayMode::Loop:
            if (m_playDir > 0) {
                // 右端から左端へジャンプ
                next = 0;
            } else {
                // 逆再生で左端まで行ったら、順方向に切り替える
                m_playDir = 1;
                next = m_pos + 1;
            }
            break;
        }
    }
    Goto(next);
}

// 追跡できているフレームの端 (連続して追跡できている区間の先頭と末尾) へ移動
// 端がなければ、先頭または末尾へ移動
void EditResult::JumpToEdge(int dir) {
    const auto& found = m_tracker->Found();
    int n = static_cast<int>(found.size());
    auto isEdge = [&](int i) {
        if (!found[i]) return false;
        return i == 0 || !found[i - 1] || i == n - 1 || !found[i + 1];
    };
    for (int i = m_pos + dir; i >= 0 && i < n; i += dir) {
        if (isEdge(i)) {
            Goto(i);
            return;
        }
    }
    Goto(dir < 0 ? 0 : LastPos());
}

void EditResult::OnButton(int index) {
    // 再生・モード以外のボタンは、再生を止める
    if (index != BTN_PLAY_BACK && index != BTN_PLAY_FWD && index != BTN_MODE) StopPlay();

    switch (index) {
    case BTN_TO_START:  Goto(0); break;
    case BTN_PREV_EDGE: JumpToEdge(-1); break;
    case BTN_PLAY_BACK: if (m_playOrigin == -1) StopPlay(); else StartPlay(-1); break;
    case BTN_STEP_BACK: Goto(m_pos - 1); break;
    case BTN_STEP_FWD:  Goto(m_pos + 1); break;
    case BTN_PLAY_FWD:  if (m_playOrigin == 1) StopPlay(); else StartPlay(1); break;
    case BTN_NEXT_EDGE: JumpToEdge(1); break;
    case BTN_TO_END:    Goto(LastPos()); break;
    case BTN_MODE: {
        // -> / <-> / loop の順に切り替え
        static const wchar_t* text[] = { L"->", L"<->", L"Loop" };
        m_mode = static_cast<PlayMode>((static_cast<int>(m_mode) + 1) % 3);
        SetWindowTextW(m_buttons[BTN_MODE], text[static_cast<int>(m_mode)]);
        break;
    }
    case BTN_TRACK_LEFT:   if (m_trackDir == -1) StopTrack(); else StartTrack(-1); break;
    case BTN_TRACK_LEFT1:  TrackStep(-1); break;
    case BTN_TRACK_RIGHT1: TrackStep(1); break;
    case BTN_TRACK_RIGHT:  if (m_trackDir == 1) StopTrack(); else StartTrack(1); break;
    }
}

// 再生速度の倍率
void EditResult::OnSpeedChanged() {
    wchar_t buf[32] = {};
    GetWindowTextW(m_speedEdit, buf, 32);
    wchar_t* end = nullptr;
    double v = wcstod(buf, &end);
    if (end == buf || v <= 0.0) return;
    m_speed = std::clamp(v, 0.05, 20.0);
    // 再生中なら、すぐ反映
    if (m_playDir != 0) SetTimer(m_hwnd, TIMER_PLAY, TimerInterval(), nullptr);
}

// 画像の表示領域。縦横比を保って、画像領域に収める
EditResult::View EditResult::GetView() const {
    View v;
    RECT client;
    GetClientRect(m_hwnd, &client);
    v.top    = TopHeight();
    v.area_w = client.right;
    v.area_h = client.bottom - v.top - BottomHeight();
    if (m_frame.empty() || v.area_w <= 0 || v.area_h <= 0) return v;

    v.scale = std::min(static_cast<double>(v.area_w) / m_frame.cols,
                       static_cast<double>(v.area_h) / m_frame.rows);
    v.w = static_cast<int>(m_frame.cols * v.scale);
    v.h = static_cast<int>(m_frame.rows * v.scale);
    v.x = (v.area_w - v.w) / 2;
    v.y = (v.area_h - v.h) / 2;
    return v;
}

// rendering_scene_video で取得した画像
void EditResult::DrawFrame(HDC hdc, const View& v) {
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize        = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth       = m_frame.cols;
    bi.bmiHeader.biHeight      = -m_frame.rows; // 上から下
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    SetStretchBltMode(hdc, HALFTONE);
    SetBrushOrgEx(hdc, 0, 0, nullptr);
    StretchDIBits(hdc, v.x, v.y, v.w, v.h, 0, 0, m_frame.cols, m_frame.rows,
                  m_frame.data, &bi, DIB_RGB_COLORS, SRCCOPY);
}

// 画像座標の四角を、画面座標にする
RECT EditResult::ToView(const cv::Rect2d& r, const View& v, int yoff) const {
    RECT out;
    out.left   = v.x + static_cast<int>(r.x * v.scale);
    out.top    = yoff + v.y + static_cast<int>(r.y * v.scale);
    out.right  = v.x + static_cast<int>((r.x + r.width) * v.scale);
    out.bottom = yoff + v.y + static_cast<int>((r.y + r.height) * v.scale);
    return out;
}

// 画面座標を、画像座標にする
cv::Point2d EditResult::ToImage(POINT p, const View& v) const {
    double x = (p.x - v.x) / v.scale;
    double y = (p.y - v.top - v.y) / v.scale;
    return { std::clamp(x, 0.0, static_cast<double>(m_frame.cols)),
             std::clamp(y, 0.0, static_cast<double>(m_frame.rows)) };
}

// つまむ場所の判定
EditResult::Hit EditResult::HitTest(POINT p) const {
    const auto& results = m_tracker->Results();
    if (m_frame.empty() || m_pos < 0 || static_cast<size_t>(m_pos) >= results.size()) return HIT_NONE;
    View v = GetView();

    if (!m_tracker->Found()[m_pos]) {
        // 解析できなかったフレームは、画像上のどこでも描き直せる
        RECT img = { v.x, v.top + v.y, v.x + v.w, v.top + v.y + v.h };
        return PtInRect(&img, p) ? HIT_EMPTY : HIT_NONE;
    }

    RECT r = ToView(results[m_pos], v, v.top);
    int rad = utils::FromDIP(8, m_dpi);
    POINT corners[4] = { { r.left, r.top }, { r.right, r.top }, { r.right, r.bottom }, { r.left, r.bottom } };
    for (int i = 0; i < 4; i++) {
        if (std::abs(p.x - corners[i].x) <= rad && std::abs(p.y - corners[i].y) <= rad) return static_cast<Hit>(i);
    }
    return PtInRect(&r, p) ? HIT_INSIDE : HIT_NONE;
}

void EditResult::OnMouseDown(POINT p) {
    if (m_playDir != 0 || m_trackDir != 0) return;
    Hit h = HitTest(p);
    if (h == HIT_NONE) return;

    View v = GetView();
    cv::Point2d ip = ToImage(p, v);
    cv::Rect2d r = m_tracker->Results()[m_pos];
    if (h == HIT_INSIDE) {
        m_drag   = Drag::Move;
        m_anchor = ip - r.tl();
    } else if (h == HIT_EMPTY) {
        m_drag   = Drag::Corner;
        m_anchor = ip;
    } else {
        // つまんだ角の対角を固定
        cv::Point2d c[4] = { r.tl(), { r.x + r.width, r.y }, r.br(), { r.x, r.y + r.height } };
        m_drag   = Drag::Corner;
        m_anchor = c[(h + 2) % 4];
    }
    SetCapture(m_hwnd);
}

void EditResult::OnMouseMove(POINT p) {
    if (m_drag == Drag::None) return;
    View v = GetView();
    cv::Point2d ip = ToImage(p, v);
    cv::Rect2d r = m_tracker->Results()[m_pos];

    if (m_drag == Drag::Corner) {
        r = cv::Rect2d(std::min(m_anchor.x, ip.x), std::min(m_anchor.y, ip.y),
                       std::abs(ip.x - m_anchor.x), std::abs(ip.y - m_anchor.y));
        if (r.width < 1 || r.height < 1) return;
    } else {
        r.x = std::clamp(ip.x - m_anchor.x, 0.0, m_frame.cols - r.width);
        r.y = std::clamp(ip.y - m_anchor.y, 0.0, m_frame.rows - r.height);
    }
    m_tracker->SetResult(m_pos, r);
    if (m_list) m_list->Refresh();
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

// 解析結果。画像の上に GDI で重ねる
void EditResult::DrawResult(HDC hdc, const View& v) {
    const auto& results = m_tracker->Results();
    const auto& found   = m_tracker->Found();
    if (m_pos < 0 || static_cast<size_t>(m_pos) >= results.size()) return;

    SetBkMode(hdc, TRANSPARENT);
    HGDIOBJ old_font = SelectObject(hdc, m_font);
    if (found[m_pos]) {
        // 半透明の四角
        cv::Scalar c = utils::hue_to_scalar(m_hueValue); // BGR
        RECT r = ToView(results[m_pos], v, 0);
        HDC src = CreateCompatibleDC(hdc);
        HBITMAP bm = CreateCompatibleBitmap(hdc, 1, 1);
        HGDIOBJ old_bm = SelectObject(src, bm);
        SetPixelV(src, 0, 0, RGB(static_cast<int>(c[2]), static_cast<int>(c[1]), static_cast<int>(c[0])));
        BLENDFUNCTION bf = { AC_SRC_OVER, 0, 128, 0 };
        AlphaBlend(hdc, r.left, r.top, r.right - r.left, r.bottom - r.top, src, 0, 0, 1, 1, bf);
        SelectObject(src, old_bm);
        DeleteObject(bm);
        DeleteDC(src);

        // 四隅のつまみ
        int hs = utils::FromDIP(4, m_dpi);
        POINT corners[4] = { { r.left, r.top }, { r.right, r.top }, { r.right, r.bottom }, { r.left, r.bottom } };
        for (const POINT& p : corners) {
            RECT h = { p.x - hs, p.y - hs, p.x + hs, p.y + hs };
            FillRect(hdc, &h, (HBRUSH)GetStockObject(WHITE_BRUSH));
            FrameRect(hdc, &h, (HBRUSH)GetStockObject(BLACK_BRUSH));
        }
        SetTextColor(hdc, RGB(0, 255, 0));
        TextOutW(hdc, v.x + 6, v.y + 4, L"OK", 2);
    } else {
        SetTextColor(hdc, RGB(255, 0, 0));
        TextOutW(hdc, v.x + 6, v.y + 4, L"ERROR", 5);
    }
    SelectObject(hdc, old_font);
}

void EditResult::Paint(HDC hdc) {
    View v = GetView();
    RECT client;
    GetClientRect(m_hwnd, &client);

    // 上下のコントロール領域の背景
    HBRUSH face = GetSysColorBrush(COLOR_BTNFACE);
    RECT top = { 0, 0, client.right, v.top };
    RECT bottom = { 0, client.bottom - BottomHeight(), client.right, client.bottom };
    FillRect(hdc, &top, face);
    FillRect(hdc, &bottom, face);

    if (v.area_w <= 0 || v.area_h <= 0) return;

    // ちらつき防止のため、メモリ DC に描いてから転送
    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, v.area_w, v.area_h);
    HGDIOBJ old_bmp = SelectObject(mem, bmp);

    RECT area = { 0, 0, v.area_w, v.area_h };
    FillRect(mem, &area, (HBRUSH)GetStockObject(BLACK_BRUSH));

    if (!m_frame.empty()) {
        DrawFrame(mem, v);
        DrawResult(mem, v);
    }

    BitBlt(hdc, 0, v.top, v.area_w, v.area_h, mem, 0, 0, SRCCOPY);
    SelectObject(mem, old_bmp);
    DeleteObject(bmp);
    DeleteDC(mem);
}

LRESULT CALLBACK EditResult::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    EditResult* self = reinterpret_cast<EditResult*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg) {
    case WM_NCCREATE:
        self = reinterpret_cast<EditResult*>(
            reinterpret_cast<CREATESTRUCT*>(lp)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        self->m_hwnd = hwnd;
        return DefWindowProc(hwnd, msg, wp, lp);

    case WM_SIZE: {
        if (self->m_trackbar) {
            int W  = LOWORD(lp);
            int H  = HIWORD(lp);
            auto DIP = [self](int dip) { return utils::FromDIP(dip, self->m_dpi); };

            // 上: 再生速度
            int ey = (self->TopHeight() - DIP(22)) / 2;
            SetWindowPos(self->m_speedLabel, nullptr, DIP(8), ey + DIP(3), DIP(44), DIP(20), SWP_NOZORDER);
            SetWindowPos(self->m_speedEdit, nullptr, DIP(56), ey, DIP(56), DIP(22), SWP_NOZORDER);
            SetWindowPos(self->m_speedX, nullptr, DIP(116), ey + DIP(3), DIP(16), DIP(20), SWP_NOZORDER);

            // 上: トラッキング手法
            SetWindowPos(self->m_methodLabel, nullptr, DIP(148), ey + DIP(3), DIP(50), DIP(20), SWP_NOZORDER);
            SetWindowPos(self->m_methodCombo, nullptr, DIP(202), ey, DIP(120), DIP(200), SWP_NOZORDER);

            // 下: スライダー
            int h   = self->TrackbarHeight();
            int lw  = self->LabelWidth();
            int top = H - self->BottomHeight();
            SetWindowPos(self->m_frameLabel, nullptr, 0, top, lw, h, SWP_NOZORDER);
            SetWindowPos(self->m_trackbar, nullptr, lw, top, W - lw, h, SWP_NOZORDER);

            // ボタンは、スライダーの下に中央寄せで並べる
            // モードのボタンの右に、"Track" と解析のボタンを並べる
            int bw  = DIP(44);
            int bh  = DIP(28);
            int gap = DIP(2);
            int tw  = DIP(40); // "Track" の幅
            int total = bw * BTN_COUNT + tw + gap * BTN_COUNT;
            int x   = (W - total) / 2;
            int y   = top + h + (self->ButtonRowHeight() - bh) / 2;
            for (int i = 0; i < BTN_COUNT; i++) {
                SetWindowPos(self->m_buttons[i], nullptr, x, y, bw, bh, SWP_NOZORDER);
                x += bw + gap;
                if (i == BTN_MODE) {
                    SetWindowPos(self->m_trackLabel, nullptr, x, y + DIP(4), tw, DIP(20), SWP_NOZORDER);
                    x += tw + gap;
                }
            }
        }
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        self->Paint(hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN:
        self->OnMouseDown({ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) });
        return 0;

    case WM_MOUSEMOVE:
        self->OnMouseMove({ GET_X_LPARAM(lp), GET_Y_LPARAM(lp) });
        return 0;

    case WM_LBUTTONUP:
        if (self->m_drag != Drag::None) {
            self->m_drag = Drag::None;
            ReleaseCapture();
        }
        return 0;

    case WM_SETCURSOR:
        if (LOWORD(lp) == HTCLIENT && self->m_playDir == 0 && self->m_trackDir == 0) {
            POINT p;
            GetCursorPos(&p);
            ScreenToClient(hwnd, &p);
            LPCWSTR cur = nullptr;
            switch (self->HitTest(p)) {
            case HIT_TL: case HIT_BR: cur = IDC_SIZENWSE; break;
            case HIT_TR: case HIT_BL: cur = IDC_SIZENESW; break;
            case HIT_INSIDE: cur = IDC_SIZEALL; break;
            case HIT_EMPTY:  cur = IDC_CROSS; break;
            default: break;
            }
            if (cur) {
                SetCursor(LoadCursor(nullptr, cur));
                return TRUE;
            }
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wp) == ID_SPEED && HIWORD(wp) == EN_CHANGE) {
            self->OnSpeedChanged();
        } else if (HIWORD(wp) == BN_CLICKED && LOWORD(wp) >= ID_BTN_FIRST && LOWORD(wp) < ID_BTN_FIRST + BTN_COUNT) {
            self->OnButton(LOWORD(wp) - ID_BTN_FIRST);
        }
        return 0;

    case WM_TIMER:
        if (wp == TIMER_PLAY) self->StepPlay();
        return 0;

    case WM_HSCROLL:
        if (reinterpret_cast<HWND>(lp) == self->m_trackbar) {
            self->StopPlay();
            self->ShowFrame(static_cast<int>(SendMessage(self->m_trackbar, TBM_GETPOS, 0, 0)));
        }
        return 0;

    case WM_APP_RESULTLIST_CLOSED:
        // 結果一覧が閉じられた
        self->m_list = nullptr;
        return 0;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        // 本体へ通知 (操作ボタンの再有効化用)
        PostMessage(self->m_parent, WM_APP_EDITRESULT_CLOSED, 0, 0);
        self->StopPlay();
        if (self->m_font) DeleteObject(self->m_font);
        if (self->m_labelFont) DeleteObject(self->m_labelFont);
        delete self;
        return 0;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

EditResult* EditResult::Create(HWND parent, Tracker* tracker, HINSTANCE hInst, EDIT_HANDLE* edit_handle, int hueValue, double wndScale, int method) {
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);

    if (!s_registered) {
        WNDCLASSEXW wcex   = {};
        wcex.cbSize        = sizeof(wcex);
        wcex.lpszClassName = CLASS_NAME;
        wcex.lpfnWndProc   = WndProc;
        wcex.hInstance     = hInst;
        wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassExW(&wcex);
        s_registered = true;
    }

    auto* dlg = new EditResult();
    dlg->m_tracker     = tracker;
    dlg->m_edit_handle = edit_handle;
    dlg->m_hueValue    = hueValue;
    dlg->m_parent      = parent;
    dlg->m_dpi         = GetDpiForWindow(parent);

    // Wnd Scale が有効ならモニタに対する割合
    // 無効ならプロジェクトの解像度で表示する。
    EDIT_INFO info{};
    edit_handle->get_edit_info(&info, sizeof(info));
    double scale = 1.0;
    if (wndScale >= 0.0 && wndScale < 1.01) {
        MONITORINFO mi = { sizeof(mi) };
        GetMonitorInfo(MonitorFromWindow(parent, MONITOR_DEFAULTTONEAREST), &mi);
        scale = std::min((mi.rcMonitor.right - mi.rcMonitor.left) * wndScale / info.width,
                         (mi.rcMonitor.bottom - mi.rcMonitor.top) * wndScale / info.height);
    }
    // FPS 取得
    if (info.rate > 0 && info.scale > 0) {
        dlg->m_fps = static_cast<double>(info.rate) / info.scale;
    }
    int client_w = std::max(utils::FromDIP(700, dlg->m_dpi), static_cast<int>(info.width * scale));
    int client_h = static_cast<int>(info.height * scale) + dlg->TopHeight() + dlg->BottomHeight();

    DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
    DWORD exStyle = WS_EX_APPWINDOW;
    RECT rc = { 0, 0, client_w, client_h };
    AdjustWindowRectExForDpi(&rc, style, FALSE, exStyle, dlg->m_dpi);

    HWND hwnd = CreateWindowExW(
        exStyle, CLASS_NAME, L"Edit Result", style,
        CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top,
        parent, nullptr, hInst, dlg);
    if (!hwnd) {
        delete dlg;
        return nullptr;
    }

    dlg->m_font = CreateFontW(
        -utils::FromDIP(16, dlg->m_dpi), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Yu Gothic UI");
    dlg->m_labelFont = CreateFontW(
        -utils::FromDIP(12, dlg->m_dpi), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Yu Gothic UI");

    // 再生速度の倍率
    dlg->m_speedLabel = CreateWindowExW(0, WC_STATIC, L"Speed", WS_VISIBLE | WS_CHILD | SS_LEFT,
        0, 0, 0, 0, hwnd, nullptr, hInst, nullptr);
    dlg->m_speedEdit = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDIT, L"1.0",
        WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
        0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_SPEED)), hInst, nullptr);
    dlg->m_speedX = CreateWindowExW(0, WC_STATIC, L"x", WS_VISIBLE | WS_CHILD | SS_LEFT,
        0, 0, 0, 0, hwnd, nullptr, hInst, nullptr);

    // トラッキング手法。内容は mainframe と同じ
    dlg->m_methodLabel = CreateWindowExW(0, WC_STATIC, L"Method", WS_VISIBLE | WS_CHILD | SS_LEFT,
        0, 0, 0, 0, hwnd, nullptr, hInst, nullptr);
    dlg->m_methodCombo = CreateWindowExW(0, WC_COMBOBOX, nullptr,
        WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
        0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_METHOD)), hInst, nullptr);
    for (int i = 0; i < METHOD_N; i++) {
        SendMessage(dlg->m_methodCombo, CB_ADDSTRING, 0, (LPARAM)track_method[i]);
    }
    SendMessage(dlg->m_methodCombo, CB_SETCURSEL, std::clamp(method, 0, METHOD_N - 1), 0);

    // フレーム番号
    dlg->m_frameLabel = CreateWindowExW(
        0, WC_STATIC, L"0",
        WS_VISIBLE | WS_CHILD | SS_CENTER | SS_CENTERIMAGE,
        0, 0, 0, 0, hwnd, nullptr, hInst, nullptr);

    // フレームのスライダー
    dlg->m_trackbar = CreateWindowExW(
        0, TRACKBAR_CLASS, nullptr,
        WS_VISIBLE | WS_CHILD | WS_TABSTOP | TBS_HORZ | TBS_AUTOTICKS,
        0, 0, 0, 0, hwnd, nullptr, hInst, nullptr);
    SendMessage(dlg->m_trackbar, TBM_SETRANGE, TRUE, MAKELPARAM(0, dlg->LastPos()));
    SendMessage(dlg->m_trackbar, TBM_SETPOS, TRUE, 0);

    // 再生・コマ送り・Track のボタン
    dlg->m_trackLabel = CreateWindowExW(0, WC_STATIC, L"Track", WS_VISIBLE | WS_CHILD | SS_CENTER,
        0, 0, 0, 0, hwnd, nullptr, hInst, nullptr);
    for (int i = 0; i < BTN_COUNT; i++) {
        dlg->m_buttons[i] = CreateWindowExW(
            0, WC_BUTTON, BTN_TEXT[i], WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
            0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_BTN_FIRST + i)), hInst, nullptr);
    }

    // フォント適用
    EnumChildWindows(hwnd, [](HWND ctrl, LPARAM font) -> BOOL {
        SendMessage(ctrl, WM_SETFONT, static_cast<WPARAM>(font), TRUE);
        return TRUE;
    }, reinterpret_cast<LPARAM>(dlg->m_labelFont));

    // 配置を反映
    RECT cr;
    GetClientRect(hwnd, &cr);
    SendMessage(hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(cr.right, cr.bottom));

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

    // 結果一覧を右隣に表示
    dlg->m_list = ResultList::Create(hwnd, tracker, hInst);
    dlg->ShowFrame(0);

    return dlg;
}
