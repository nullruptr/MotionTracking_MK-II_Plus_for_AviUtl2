#include "hsv.hpp"
#include "constants.hpp"
#include <memory>
#include <vector>
#include <opencv2/opencv.hpp>

enum class hsv_id : int {
    hsv = 0,
    hue_only = 1,
    saturation_only = 2,
    value_only = 3
};
static FILTER_ITEM_SELECT::ITEM hsv_check_name[] {
    { L"HSV", static_cast<int>(hsv_id::hsv) },
    { L"Hue only", static_cast<int>(hsv_id::hue_only) },
    { L"Saturation only", static_cast<int>(hsv_id::saturation_only) },
    { L"Value only", static_cast<int>(hsv_id::value_only) },
    { nullptr, {} },
};

bool hsv_func_proc_video(FILTER_PROC_VIDEO* video);
static FILTER_ITEM_SELECT hsv_select(L"Mode", static_cast<int>(hsv_id::hsv), hsv_check_name);
static void* hsv_filter_items[] = { &hsv_select, nullptr };


//---------------------------------------------------------------------
//	フィルタプラグイン構造体定義
//---------------------------------------------------------------------

FILTER_PLUGIN_TABLE filter_hsv = {
	FILTER_PLUGIN_TABLE::FLAG_VIDEO, //	フラグ
	constants::hsv,					// プラグインの名前
	hsv_label.c_str(), 									// ラベルの初期値 (nullptrならデフォルトのラベルになります)
	verstr_hsv.c_str(),	// プラグインの情報
	hsv_filter_items,											// 設定項目の定義 (FILTER_ITEM_XXXポインタを列挙してnull終端したリストへのポインタ)
	hsv_func_proc_video,								// 画像フィルタ処理関数へのポインタ (FLAG_VIDEOが有効の時のみ呼ばれます)
	nullptr									// 音声フィルタ処理関数へのポインタ (FLAG_AUDIOが有効の時のみ呼ばれます)
};

//HSV Conversion filter
bool hsv_func_proc_video(FILTER_PROC_VIDEO* video) {

    int w, h;
    w = video->object->width;
    h = video->object->height;
    auto buffer = std::make_unique<PIXEL_RGBA[]>(w * h);

    const auto check = static_cast<hsv_id>(hsv_select.value);
    video->get_image_data(buffer.get());

    cv::Mat ocvImg_alpha(h, w, CV_8UC4, buffer.get());
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
    cv::cvtColor(outImg, outImg_alpha, cv::COLOR_RGB2RGBA); // alpha 追加
    video->set_image_data(reinterpret_cast<const PIXEL_RGBA*>(outImg_alpha.data), w, h);
    return true;
}
