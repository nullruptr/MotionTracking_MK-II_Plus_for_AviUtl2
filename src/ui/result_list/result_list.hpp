#pragma once
#include <windows.h>
#include "tracker/tracker.hpp"

// 解析結果を一覧表示するウィンドウ。EditResult から開く
class ResultList {
public:
    static ResultList* Create(HWND owner, Tracker* tracker, HINSTANCE hInst);

    // スライダーが指しているフレーム位置を設定し、該当行を強調表示
    void SetCurrent(int pos);

    // 現在の行を再描画
    void Refresh();

private:
    HWND     m_hwnd     = nullptr;
    HWND     m_owner    = nullptr; // 閉じたことを通知する先
    HWND     m_listView = nullptr;
    HFONT    m_font     = nullptr;
    Tracker* m_tracker  = nullptr;
    int      m_current  = -1;

    int      RowCount() const;
    COLORREF RowColor(int row) const;
    void     GetCellText(int row, int col, wchar_t* buf, int len) const;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static bool s_registered;
};
