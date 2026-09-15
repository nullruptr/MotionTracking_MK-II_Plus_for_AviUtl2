#pragma once
#include <windows.h>
#include <vector>

// トラッキング結果(平滑化前後)をListViewで一覧表示するダイアログ
class NumericDataDlg {
public:
    static void Create(
        HWND parent, HINSTANCE hInst, int rangeStart,
        const std::vector<double>& x, const std::vector<double>& y,
        const std::vector<double>& width, const std::vector<double>& height,
        const std::vector<double>& smoothX, const std::vector<double>& smoothY);

private:
    HWND m_hwnd     = nullptr;
    HWND m_listView = nullptr;
    HWND m_parent   = nullptr; // 破棄時に通知を送る本体ウィンドウ

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    static bool s_registered;
};
