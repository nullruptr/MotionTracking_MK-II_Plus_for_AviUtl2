#include "bgs.hpp"

bool bgs_func_proc_video(FILTER_PROC_VIDEO* video);
static void* bgs_filter_items[] = { nullptr };

//---------------------------------------------------------------------
//	フィルタプラグイン構造体定義
//---------------------------------------------------------------------
FILTER_PLUGIN_TABLE filter_bgs = {
	FILTER_PLUGIN_TABLE::FLAG_VIDEO | FILTER_PLUGIN_TABLE::FLAG_AUDIO, //	フラグ
	constants::bgs,					// プラグインの名前
	nullptr,									// ラベルの初期値 (nullptrならデフォルトのラベルになります)
	verstr_bgs.c_str(),	// プラグインの情報
	bgs_filter_items,											// 設定項目の定義 (FILTER_ITEM_XXXポインタを列挙してnull終端したリストへのポインタ)
	bgs_func_proc_video,								// 画像フィルタ処理関数へのポインタ (FLAG_VIDEOが有効の時のみ呼ばれます)
	nullptr									// 音声フィルタ処理関数へのポインタ (FLAG_AUDIOが有効の時のみ呼ばれます)
};

bool bgs_func_proc_video(FILTER_PROC_VIDEO* video) { return true; }
