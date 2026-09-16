#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include "opencv2/core.hpp"
#include "aviutl2_sdk/plugin2.h"

struct FRMFIX {
    int    frame;
    double cx;
    double cy;
    double width;
    double height;
    double scale;
    bool   found;
};

struct FRMGROUP {
    int start;
    int end;
    int vi_start;
    int vi_end;
};

class InsertObject {
public:
    static std::string make_alias(const std::vector<FRMFIX>& fixedFrm, int vi_start, int vi_end, bool ignoreAspectRatio);
    static std::string make_alias_as_sub(const std::vector<FRMFIX>& fixedFrm, int vi_start, int vi_end, bool ignoreAspectRatio);
    static bool Insert(
        const std::vector<cv::Rect2d>& results,
        const std::vector<bool>& found,
        int rangeStart,
        EDIT_HANDLE* edit,
        bool ignoreAspectRatio = true,
        bool invertPosition = false,
        bool asSubFilter = false,
        bool smoothEnable = false,
        int  smoothWindow = 5,
        int  smoothPolyorder = 2
    );
    // タイムラインに挿入せず、生成したaliasテキストをファイルに書き出す(確認用)
    static bool ExportToFile(
        const std::vector<cv::Rect2d>& results,
        const std::vector<bool>& found,
        int rangeStart,
        EDIT_HANDLE* edit,
        const std::wstring& filepath,
        bool ignoreAspectRatio = true,
        bool invertPosition = false,
        bool asSubFilter = false,
        bool smoothEnable = false,
        int  smoothWindow = 5,
        int  smoothPolyorder = 2
    );
    // Numeric Data 表示用: 平滑化前後の中心座標(画像ピクセル座標)を計算する
    static void ComputeSmoothPreview(
        const std::vector<cv::Rect2d>& results,
        const std::vector<bool>& found,
        bool smoothEnable,
        int  smoothWindow,
        int  smoothPolyorder,
        std::vector<double>& outX,
        std::vector<double>& outY,
        std::vector<double>& outWidth,
        std::vector<double>& outHeight,
        std::vector<double>& outSmoothX,
        std::vector<double>& outSmoothY
    );
private:
    InsertObject() = delete;
    static cv::Point2d getCenter(const cv::Rect2d& box);
    static int  find_inter_frame(std::vector<bool> &err_list, std::vector<UINT32> &out_list);
    static void interpolate(std::vector<cv::Rect2d> &rect_list, std::vector<bool> &err_list, const std::vector<UINT32> &inter_list);
    static void fix_frame(std::vector<cv::Rect2d> &rect_list, std::vector<bool> &err_list, std::vector<UINT32> &inter_list, std::vector<FRMFIX> &out, int frm_w, int frm_h, int rangeStart, bool ignoreAspectRatio, bool invertPosition, bool smoothEnable, int smoothWindow, int smoothPolyorder);
    static void groupObject(std::vector<FRMFIX> &fixedframes, std::vector<FRMGROUP> &out, int rangeStart);
    static void smoothSegment(std::vector<double>& data, int segStart, int segEnd, int window, int polyorder);

};
