#pragma once
#include <windows.h>
#include <string>
#include "tracker/tracker.hpp"

// Pre-track の設定を適用したフレーム画像を返す。失敗時は空
cv::Mat pretrack_apply(const PreTrackSettings& s, EDIT_HANDLE* edit_handle, int range_s, int range_e, int cur_frame);

class PreTrack {
public:
    static PreTrack* Create(HWND parent, Tracker* tracker, HINSTANCE hInst, EDIT_HANDLE* edit_handle, double wndScale);

private:
    HWND         m_hwnd       = nullptr;
    HWND         m_parent     = nullptr; // 呼び出し元ウィンドウ。GetParent に頼らずここで保持する
    HWND         m_progress   = nullptr;
    HWND         m_label      = nullptr;
    HFONT        m_font       = nullptr; // 子コントロール用フォント
    Tracker*     m_tracker    = nullptr;
    EDIT_HANDLE* m_edit_handle = nullptr;
    double       m_wndScale   = 1.01;    // Wnd Scale。1.01以上は無効
    bool         m_previewing = false;   // プレビュー表示中
    bool         m_closeReq   = false;   // プレビュー中に閉じる要求があった
    bool         m_saveReq    = false;   // 閉じる要求が OK だった

    void Preview();
    void Save();
    PreTrackSettings Read() const;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static bool s_registered;
};
