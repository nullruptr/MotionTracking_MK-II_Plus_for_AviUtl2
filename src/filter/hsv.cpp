#include "hsv.hpp"

bool hsv_func_proc_video(FILTER_PROC_VIDEO* video);
static void* hsv_filter_items[] = { nullptr };

//---------------------------------------------------------------------
//	フィルタプラグイン構造体定義
//---------------------------------------------------------------------
FILTER_PLUGIN_TABLE filter_hsv = {
	FILTER_PLUGIN_TABLE::FLAG_VIDEO | FILTER_PLUGIN_TABLE::FLAG_AUDIO, //	フラグ
	constants::hsv,					// プラグインの名前
	nullptr,									// ラベルの初期値 (nullptrならデフォルトのラベルになります)
	verstr_hsv.c_str(),	// プラグインの情報
	hsv_filter_items,											// 設定項目の定義 (FILTER_ITEM_XXXポインタを列挙してnull終端したリストへのポインタ)
	hsv_func_proc_video,								// 画像フィルタ処理関数へのポインタ (FLAG_VIDEOが有効の時のみ呼ばれます)
	nullptr									// 音声フィルタ処理関数へのポインタ (FLAG_AUDIOが有効の時のみ呼ばれます)
};

bool hsv_func_proc_video(FILTER_PROC_VIDEO* video) { return true; }
