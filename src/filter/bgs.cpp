#include <atomic>
#include "bgs.hpp"
#include "aviutl2_sdk/filter2.h"
#include "aviutl2_sdk/plugin2.h"

extern EDIT_HANDLE* edit_handle; // main.cpp

static std::atomic<bool> g_in_scene_render{false};

enum class bgs_id : int {
    mog2 = 0,
    knn = 1,
    mog2_mask = 2,
    knn_mask = 3
};

static FILTER_ITEM_SELECT::ITEM bgs_check_name[] {
    { L"MOG2", static_cast<int>(bgs_id::mog2) },
    { L"KNN", static_cast<int>(bgs_id::knn) },
    { L"MOG2(Mask)", static_cast<int>(bgs_id::mog2_mask) },
    { L"KNN(Mask)", static_cast<int>(bgs_id::knn_mask) },
    { nullptr, {} }
};

static FILTER_ITEM_TRACK bgs_range(L"Range", 30, 1, 250);
static FILTER_ITEM_CHECK bgs_shadow(L"Shadow", false);
static FILTER_ITEM_TRACK bgs_nmix(L"NMix", 5, 1, 30);
static FILTER_ITEM_TRACK bgs_bgratio(L"BG%", 70, 1, 99);
static FILTER_ITEM_TRACK bgs_d2t(L"d2T", 400, 100, 1000);

static FILTER_ITEM_SELECT bgs_select(L"Mode", static_cast<int>(bgs_id::mog2), bgs_check_name);
static void* bgs_filter_items[] = {
    &bgs_range, &bgs_shadow, &bgs_nmix, &bgs_bgratio, &bgs_d2t, &bgs_select, nullptr
};

bool bgs_func_proc_video(FILTER_PROC_VIDEO* video);

//---------------------------------------------------------------------
//	フィルタプラグイン構造体定義
//---------------------------------------------------------------------
FILTER_PLUGIN_TABLE filter_bgs = {
	FILTER_PLUGIN_TABLE::FLAG_VIDEO | FILTER_PLUGIN_TABLE::FLAG_FILTER, //	フラグ
	constants::bgs,					// プラグインの名前
	bgs_label.c_str(),									// ラベルの初期値 (nullptrならデフォルトのラベルになります)
	verstr_bgs.c_str(),	// プラグインの情報
	bgs_filter_items,											// 設定項目の定義 (FILTER_ITEM_XXXポインタを列挙してnull終端したリストへのポインタ)
	bgs_func_proc_video,								// 画像フィルタ処理関数へのポインタ (FLAG_VIDEOが有効の時のみ呼ばれます)
	nullptr									// 音声フィルタ処理関数へのポインタ (FLAG_AUDIOが有効の時のみ呼ばれます)
};


// plugin2.h より、任意フレーム画像を取得する処理
cv::Mat get_scene_frame(int f) {
    cv::Mat img;
    g_in_scene_render = true;
    bool render = edit_handle->rendering_scene_video (
        f,
        &img,
        [](void* param, int frame, const void* buffer, int width, int height, int pitch) {
            cv::Mat* out = static_cast<cv::Mat*>(param);
            cv::Mat rgba(height, width, CV_8UC4, const_cast<void*>(buffer), static_cast<size_t>(pitch));
            cv::cvtColor(rgba, *out, cv::COLOR_RGBA2BGR);
        });

    if (!render) {
        g_in_scene_render = false;
        return cv::Mat();
    }

    edit_handle->wait_rendering_task();
    g_in_scene_render = false;
    return img;
}

//MOG2 and KNN Background subtraction
bool bgs_func_proc_video(FILTER_PROC_VIDEO* video) {
    if (g_in_scene_render) return true; // 描画中は何もしない

    if (edit_handle->get_edit_state() == EDIT_HANDLE::EDIT_STATE_SAVE) {
        return true;
    }

    bool isShadow = false;
    if (bgs_shadow.value) {
        isShadow = true;
    }
    //boundary correction
    int frame_s, frame_e;

    // 描画フレーム値
    // 現在描画しているフレーム = オブジェクトの始点フレーム値 + 始点からの進行したフレーム数
    int cur_frame = video->object->frame_s + video->object->frame; // fpip->frame

    // オブジェクト始点と終点
    int obj_frame_s = video->object->frame_s;
    int obj_frame_e = video->object->frame_e;

    // パラメータ の int
    int range_value   = static_cast<int>(bgs_range.value); // track[0]
    int nmix_value    = static_cast<int>(bgs_nmix.value); // track[2]
    int bgratio_value = static_cast<int>(bgs_bgratio.value); // track[3]
    int d2t_value     = static_cast<int>(bgs_d2t.value); // track[4]

    // Range 切り詰め処理
    // ex. オブジェクト範囲 100~200 で、cur_frame が 120 なら、手前10切り詰める。
    // 後ろも同様にrange超過分は切り詰める
    frame_s = cur_frame - range_value;
    if (frame_s < obj_frame_s) frame_s = obj_frame_s;
    frame_e = cur_frame + range_value;
    if (frame_e > obj_frame_e) frame_e = obj_frame_e;

    //Setup OCV subtracttor
    cv::Mat mask;
    const auto check = static_cast<bgs_id>(bgs_select.value);
    cv::Ptr<cv::BackgroundSubtractor> bsub;
    if (check == bgs_id::mog2 || check == bgs_id::mog2_mask) {
        auto mog2 = cv::createBackgroundSubtractorMOG2();
        mog2->setDetectShadows(isShadow);
        mog2->setHistory(range_value);
        mog2->setNMixtures(nmix_value);
        mog2->setBackgroundRatio((double)bgratio_value / 100.0);
        bsub = mog2;
    } else {
        auto knn = cv::createBackgroundSubtractorKNN();
        knn->setDetectShadows(isShadow);
        knn->setHistory(range_value*2);
        knn->setDist2Threshold(d2t_value);
        bsub = knn;
    }
    for (int f = frame_s; f <= cur_frame - 1; f++) {// Scan all frames in range
        cv::Mat img = get_scene_frame(f);
        if (img.empty()) return true; // 画像がないとき抜ける
        bsub->apply(img, mask);
    }
    cv::Mat cur_img = get_scene_frame(cur_frame);
    bsub->apply(cur_img, mask);
    cv::Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::erode(mask, mask, element);
    cv::dilate(mask, mask, element);
    cv::Mat rgba;
    if (check == bgs_id::mog2_mask || check == bgs_id::knn_mask) {
        cv::cvtColor(mask, rgba, cv::COLOR_GRAY2RGBA);
    } else {
        cv::Mat tempbuf;
        cur_img.copyTo(tempbuf, mask);
        cv::cvtColor(tempbuf, rgba, cv::COLOR_BGR2RGBA);
    }
    if (rgba.cols != video->object->width || rgba.rows != video->object->height) return true;
    video->set_image_data(reinterpret_cast<const PIXEL_RGBA*>(rgba.data), rgba.cols, rgba.rows);

    return true;
}

