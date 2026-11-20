#pragma once
#include <windows.h>

namespace constants {
    constexpr const wchar_t* WindowName = L"MotionTracking MK-II Plus for AviUtl2";
    constexpr const wchar_t* bgs = L"Pre-track:BGSubtraction";
    constexpr const wchar_t* hsv = L"Pre-track:HSV Cvt";
    constexpr const wchar_t* version = L"${ver}";
    constexpr const wchar_t* APIerr = L"AviUtl2 API Error";
}

// トラッキング手法 (mainframe と EditResult で共通)
inline constexpr const wchar_t* track_method[] = { L"MIL", L"KCF", L"CSRT", L"DaSiamRPN", L"Nano", L"Vit" };
inline constexpr int METHOD_N = sizeof(track_method) / sizeof(track_method[0]);

enum class IDC_Menu : UINT {
    ExportCSV    = 3001,
    ExportObject = 3002,
};

enum class IDC_Toolbar : UINT {
    Bar     = 4000,
    File    = 4001,
    Options = 4002,
    Info    = 4003,
};

// ProgressDlg がバックグラウンド解析の完了を検知したときに、本体ウィンドウへ通知するメッセージ
constexpr UINT WM_APP_ANALYZE_DONE = WM_APP + 1;
// NumericDataDlg が閉じられたときに、本体ウィンドウへ通知するメッセージ
constexpr UINT WM_APP_NUMERIC_DATA_CLOSED = WM_APP + 2;
// PreTrack が閉じられたときに、本体ウィンドウへ通知するメッセージ
constexpr UINT WM_APP_PRETRACK_CLOSED = WM_APP + 3;
// EditResult が閉じられたときに、本体ウィンドウへ通知するメッセージ
constexpr UINT WM_APP_EDITRESULT_CLOSED = WM_APP + 4;
// ResultList が閉じられたときに、所有者(EditResult)へ通知するメッセージ
constexpr UINT WM_APP_RESULTLIST_CLOSED = WM_APP + 5;
