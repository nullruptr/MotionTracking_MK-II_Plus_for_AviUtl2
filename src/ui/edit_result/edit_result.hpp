#pragma once
#include <windows.h>
#include <opencv2/core.hpp>
#include "tracker/tracker.hpp"

class ResultList;

// 再生モード
enum class PlayMode {
    Once,   // -> 端まで行ったら停止
    Bounce, // <-> 端で折り返す
    Loop,   // loop 順方向は右端から左端へジャンプ。逆方向は左端で順方向に切り替える
};

class EditResult {
public:
    static EditResult* Create(HWND parent, Tracker* tracker, HINSTANCE hInst, EDIT_HANDLE* edit_handle, int hueValue, double wndScale, int method);

private:
    HWND         m_hwnd        = nullptr;
    HWND         m_parent      = nullptr; // 呼び出し元ウィンドウ
    HWND         m_trackbar    = nullptr;
    HWND         m_frameLabel  = nullptr; // フレーム番号
    HWND         m_speedLabel  = nullptr;
    HWND         m_speedEdit   = nullptr; // 再生速度の倍率
    HWND         m_speedX      = nullptr;
    HWND         m_methodLabel = nullptr;
    HWND         m_methodCombo = nullptr; // トラッキング手法
    HWND         m_trackLabel  = nullptr; // "Track"
    HWND         m_buttons[13] = {};      // 再生・コマ送り・Track のボタン
    HFONT        m_font        = nullptr;
    HFONT        m_labelFont   = nullptr;
    Tracker*     m_tracker     = nullptr;
    EDIT_HANDLE* m_edit_handle = nullptr;
    ResultList*  m_list        = nullptr; // 結果一覧ウィンドウ
    int          m_hueValue    = 180;
    UINT         m_dpi         = 96;
    int          m_pos         = 0;       // 表示中のフレーム位置
    cv::Mat      m_frame;                 // 表示中の画像 (BGRA)

    int          m_playDir     = 0;       // 現在の再生方向 (1: 順方向, -1: 逆方向, 0: 停止)
    int          m_playOrigin  = 0;       // 再生を開始したボタンの方向。折り返しても変化なし
    int          m_trackDir    = 0;       // 解析中の方向 (1: 右, -1: 左, 0: 停止)
    PlayMode     m_mode        = PlayMode::Once;
    double       m_fps         = 30.0;    // プロジェクトのフレームレート
    double       m_speed       = 1.0;     // 再生速度の倍率

    // マウス操作
    enum Hit { HIT_TL, HIT_TR, HIT_BR, HIT_BL, HIT_INSIDE, HIT_EMPTY, HIT_NONE };
    enum class Drag { None, Corner, Move };
    Drag         m_drag   = Drag::None;
    cv::Point2d  m_anchor;                // Corner: 固定する対角。Move: つかんだ位置の差

    // 画像の表示領域 (x, y, w, h) と、画像座標 -> 画面座標の倍率
    // top: 表示領域のウィンドウ内での上端
    struct View { int area_w = 0, area_h = 0, top = 0, x = 0, y = 0, w = 0, h = 0; double scale = 1.0; };

    View GetView() const;
    void ShowFrame(int pos);
    void Paint(HDC hdc);
    void DrawFrame(HDC hdc, const View& v);
    void DrawResult(HDC hdc, const View& v);
    RECT ToView(const cv::Rect2d& r, const View& v, int yoff) const;
    cv::Point2d ToImage(POINT p, const View& v) const;
    Hit  HitTest(POINT p) const;
    void OnMouseDown(POINT p);
    void OnMouseMove(POINT p);

    int  TopHeight() const;
    int  TrackbarHeight() const;
    int  ButtonRowHeight() const;
    int  BottomHeight() const;
    int  LabelWidth() const;
    int  LastPos() const;
    int  TimerInterval() const;

    void Goto(int pos);
    void StartPlay(int dir);
    void StopPlay();
    void StepPlay();
    void UpdateButtons();
    void StartTrack(int dir);
    void StopTrack();
    void TrackStep(int dir);
    void JumpToEdge(int dir);
    void OnButton(int index);
    void OnSpeedChanged();

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static bool s_registered;
};
