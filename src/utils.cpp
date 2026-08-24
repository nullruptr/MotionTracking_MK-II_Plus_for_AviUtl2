#include "utils.hpp"
#include "aviutl2_sdk/plugin2.h"
#include <winuser.h>
#include <dwmapi.h>
#include <algorithm>

namespace utils {

cv::Scalar hue_to_scalar(int hue) {
    hue = hue % 360;

    if (hue < 60)
        return cv::Scalar(255, hue * 255 / 60, 0);
    else if (hue < 120)
        return cv::Scalar((120-hue) * 255 / 60, 255, 0);
    else if (hue < 180)
        return cv::Scalar(0, 255, (hue - 120) * 255 / 60);
    else if (hue < 240)
        return cv::Scalar(0, (240 - hue) * 255 / 60, 255);
    else if (hue < 300)
        return cv::Scalar((hue - 240) * 255 / 60, 0, 255);
    else
        return cv::Scalar(255, 0, (360 - hue) * 255 / 60);
}

std::string get_model_dir(HINSTANCE hInst) {
    char path[MAX_PATH * 2];
    if (GetModuleFileNameA(hInst, path, sizeof(path))) {
        char* p = strrchr(path, '\\');
        if (p) {
            *(p + 1) = '\0';
            return std::string(path) + "MotionTracking_model\\";
        }
    }
    return {};
    }

bool is_high_dpi_mode(HINSTANCE hInst) {
    wchar_t path[MAX_PATH * 2];
    if (!GetModuleFileNameW(hInst, path, static_cast<DWORD>(sizeof(path) / sizeof(path[0])))) {
        return false;
    }
    wchar_t* p = wcsrchr(path, L'\\');
    if (!p) {
        return false;
    }
    *(p + 1) = L'\0';
    wcscat_s(path, L"..\\aviutl2.ini");
    return GetPrivateProfileIntW(L"Direct3D", L"HighDpiMode", 0, path) != 0;
}

int FromDIP(int dip, UINT dpi) {
    return MulDiv(dip, dpi, 96);
}

bool ResizeWindow(HWND hwnd, EDIT_HANDLE* edit_handle) {
    EDIT_INFO info{};
    edit_handle->get_edit_info(&info, sizeof(info));

    int imgw = info.width;
    int imgh = info.height;

    // https://s-kita.hatenablog.com/entry/20130502/1367485535

    // モニタ情報
    MONITORINFOEX MonitorInfoEx;
    HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

    MonitorInfoEx.cbSize = sizeof(MonitorInfoEx);
    GetMonitorInfo(hMonitor, &MonitorInfoEx);

    // モニタの大きさ
    int mh = std::abs(MonitorInfoEx.rcMonitor.top - MonitorInfoEx.rcMonitor.bottom);
    int mw = std::abs(MonitorInfoEx.rcMonitor.right - MonitorInfoEx.rcMonitor.left);

    RECT rect;
    HRESULT hResult;
    BOOL    bResult;
    BOOL    bDwmEnable;

    hResult = DwmIsCompositionEnabled( &bDwmEnable );
    if  (S_OK == hResult ){
        if ( bDwmEnable ){
            // エアロ環境
            hResult = DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(rect));
        }
        else{
            // 非エアロ環境
            bResult = GetWindowRect(hwnd, &rect);
        }
    }

    double wnd_w = rect.right - rect.left;
    double wnd_h = rect.bottom - rect.top;

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    double chrome_w = wnd_w - clientRect.right;
    double chrome_h = wnd_h - clientRect.bottom;

    // いったん0.8倍
    double k = 0.8;
    double scale_w = (mw * k) / imgw;
    double scale_h = (mh * k) / imgh;
    double scale  = std::min(scale_w, scale_h);

    double mw_dash = imgw * scale;
    double mh_dash = imgh * scale;

    mw_dash += chrome_w;
    mh_dash += chrome_h;

    SetWindowPos(hwnd, nullptr, 0, 0, static_cast<int>(mw_dash), static_cast<int>(mh_dash), SWP_NOMOVE | SWP_NOZORDER);

    return  true;
}

}
