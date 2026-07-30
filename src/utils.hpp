#pragma once
#include <windows.h>
#include "opencv2/core/utility.hpp"

namespace utils {
    cv::Scalar hue_to_scalar(int hue);
    // モデルのファイルパス処理
    std::string get_model_dir(HINSTANCE hInst);
    // aviutl2.ini から 拡大表示か否かを読み取る
    bool is_high_dpi_mode(HINSTANCE hInst);
    // 96DPI基準のDIP値を、指定DPIに合わせた実ピクセル値に変換する(wxWidgetsのFromDIP相当)
    int FromDIP(int dip, UINT dpi);
}
