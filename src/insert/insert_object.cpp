#include <cmath>
#include <cstdio>
#include <algorithm>
#include "insert_object.hpp"
#include "savgol/savgol.hpp"

std::string InsertObject::make_alias(const std::vector<FRMFIX>& fixedFrm, int vi_start, int vi_end, bool ignoreAspectRatio) {
    std::string s;
    s += "[Object]\n";
    s += "frame=";
    for (int i = vi_start; i <= vi_end; i++) {
        s += std::to_string(fixedFrm[i].frame);
        if (i < vi_end) s += ",";
    }
    s += "\n";
    s += "[Object.0]\n";
    s += "effect.name=図形\n";
    s += "図形の種類=四角形\n";
    s += "サイズ=100\n";
    s += "縦横比=0.00\n";
    s += "ライン幅=4\n";
    s += "色=00ff00\n";
    s += "角を丸くする=0\n";
    s += "[Object.1]\n";
    s += "effect.name=標準描画\n";
    // X=x1,x2,...
    s += "X=";
    for (int i = vi_start; i <= vi_end; i++) {
        s += std::to_string(fixedFrm[i].cx) + ".00";
        if (i < vi_end) s += ",";
    }
    s += ",直線移動,0\n";

    // Y=y1,y2,...
    s += "Y=";
    for (int i = vi_start; i <= vi_end; i++) {
        s += std::to_string(fixedFrm[i].cy) + ".00";
        if (i < vi_end) s += ",";
    }

    s += ",直線移動,0\n";
    s += "Z=0.00\n";
    s += "Group=1\n";
    s += "中心X=0.00\n";
    s += "中心Y=0.00\n";
    s += "中心Z=0.00\n";
    s += "X軸回転=0.00\n";
    s += "Y軸回転=0.00\n";
    s += "Z軸回転=0.00\n";
    s += "Group2=1\n";
    s += "拡大率=100.000\n";
    s += "縦横比=0.000\n";
    s += "透明度=0.00\n";
    s += "合成モード=通常\n";
    s += "[Object.2]\n";
    s += "effect.name=リサイズ\n";
    if (ignoreAspectRatio) {
        // 拡大率=s1,s2,...,直線移動,0
        s += "拡大率=";
        for (int i = vi_start; i <= vi_end; i++) {
            s += std::to_string((int)fixedFrm[i].scale);
            s += ".000";
            if (i < vi_end) s += ",";
        }
        s += ",直線移動,0\n";
        s += "X=100.000\n";
        s += "Y=100.000\n";
        s += "補間なし=0\n";
        s += "ピクセル数でサイズ指定=0\n";
    } else {
        s += "拡大率=100.000\n";
        // X=w1,w2,...,直線移動,0 (幅をピクセル指定)
        s += "X=";
        for (int i = vi_start; i <= vi_end; i++) {
            s += std::to_string(fixedFrm[i].width) + ".000";
            if (i < vi_end) s += ",";
        }
        s += ",直線移動,0\n";
        // Y=h1,h2,...,直線移動,0 (高さをピクセル指定)
        s += "Y=";
        for (int i = vi_start; i <= vi_end; i++) {
            s += std::to_string(fixedFrm[i].height) + ".000";
            if (i < vi_end) s += ",";
        }
        s += ",直線移動,0\n";
        s += "補間なし=0\n";
        s += "ピクセル数でサイズ指定=0\n";
    }
    return s;
}

std::string InsertObject::make_alias_as_sub(const std::vector<FRMFIX>& fixedFrm, int vi_start, int vi_end, bool ignoreAspectRatio) {
    std::string s;

    s += "[Object]\n";
    s += "frame=";
    for (int i = vi_start; i <= vi_end; i++) {
        s += std::to_string(fixedFrm[i].frame);
        if (i < vi_end) s += ",";
    }
    s += "\n";
    s += "[Object.0]\n";
    s += "effect.name=部分フィルタ\n";
    // X=x1,x2,...
    s += "X=";
    for (int i = vi_start; i <= vi_end; i++) {
        s += std::to_string(fixedFrm[i].cx);
        if (i < vi_end) s += ",";
    }
    s += ",直線移動,0\n";

    // Y=y1,y2,...
    s += "Y=";
    for (int i = vi_start; i <= vi_end; i++) {
        s += std::to_string(fixedFrm[i].cy);
        if (i < vi_end) s += ",";
    }

    s += ",直線移動,0\n";
    s += "Group=1\n";
    s += "回転=0.00\n";
    // サイズ=s1,s2,...,直線移動,0
    s += "サイズ=";
    for (int i = vi_start; i <= vi_end; i++) {
        s += std::to_string((int)fixedFrm[i].scale);
        if (i < vi_end) s += ",";
    }
    s += ",直線移動,0\n";
    s += "縦横比=";
    for (int i = vi_start; i <= vi_end; i++) {
        double rAsp = 0.0;
        if (!ignoreAspectRatio) {
            if (fixedFrm[i].width > fixedFrm[i].height) {
                rAsp = -100.0 * (1.0 - ((double)fixedFrm[i].height / (double)fixedFrm[i].width));
            } else if (fixedFrm[i].width < fixedFrm[i].height) {
                rAsp = 100.0 * (1.0 - ((double)fixedFrm[i].width / (double)fixedFrm[i].height));
            }
        }
        // 小数第2位までにフォーマット(四捨五入込み)
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f", rAsp);
        s += buf;
        if (i < vi_end) s += ",";
    }
    s += ",直線移動,0\n";
    s += "ぼかし=0\n";
    s += "マスクの種類=四角形\n";
    s += "シーンの長さを合わせる=0\n";
    s += "マスクの反転=0\n";
    s += "[Object.1]\n";
    s += "effect.name=単色化\n";
    s += "強さ=100.0\n";
    s += "色=ff0000\n";
    s += "輝度を保持する=1\n";

    return s;
}

bool InsertObject::Insert(
    const std::vector<cv::Rect2d>& results,
    const std::vector<bool>& found,
    int rangeStart,
    EDIT_HANDLE* edit,
    bool ignoreAspectRatio,
    bool invertPosition,
    bool asSubFilter,
    bool smoothEnable,
    int  smoothWindow,
    int  smoothPolyorder)
{
    if (results.empty()) return false;

    auto rect_list = results;
    auto err_list  = found;

    std::vector<UINT32>   inter_list;
    std::vector<FRMFIX>   fixedFrm;
    std::vector<FRMGROUP> groups;

    find_inter_frame(err_list, inter_list);

    struct Param {
        std::vector<cv::Rect2d>* rect_list;
        std::vector<bool>*       err_list;
        std::vector<UINT32>*     inter_list;
        std::vector<FRMFIX>*     fixedFrm;
        std::vector<FRMGROUP>*   groups;
        int  rangeStart;
        bool ignoreAspectRatio;
        bool invertPosition;
        bool asSubFilter;
        bool smoothEnable;
        int  smoothWindow;
        int  smoothPolyorder;
        bool ok;
    } p { &rect_list, &err_list, &inter_list, &fixedFrm, &groups, rangeStart, ignoreAspectRatio, invertPosition, asSubFilter, smoothEnable, smoothWindow, smoothPolyorder, false };

    edit->call_edit_section_param(&p, [](void* v, EDIT_SECTION* edit) {
        auto* p = static_cast<Param*>(v);

        fix_frame(*p->rect_list, *p->err_list, *p->inter_list,
                  *p->fixedFrm, edit->info->width, edit->info->height, p->rangeStart, p->ignoreAspectRatio, p->invertPosition,
                  p->smoothEnable, p->smoothWindow, p->smoothPolyorder);
        groupObject(*p->fixedFrm, *p->groups, p->rangeStart);

        int layer = edit->info->layer;
        for (const auto& g : *p->groups) {
            std::string alias = p->asSubFilter
                ? make_alias_as_sub(*p->fixedFrm, g.vi_start, g.vi_end, p->ignoreAspectRatio)
                : make_alias(*p->fixedFrm, g.vi_start, g.vi_end, p->ignoreAspectRatio);
                bool inserted = false;
                for (int i = 0; i < 100; i++) {
                    OBJECT_HANDLE handle = edit->create_object_from_alias(
                        alias.c_str(), layer, g.start, g.end - g.start + 1);
                    if (handle) {
                        inserted = true;
                        break;
                    }
                    layer += 1;
                }
            if (!inserted) { // insert 失敗時
                p->ok = false;
                return;
            }
        }
        p->ok = true;
    });

    return p.ok;
}

bool InsertObject::ExportToFile(
    const std::vector<cv::Rect2d>& results,
    const std::vector<bool>& found,
    int rangeStart,
    EDIT_HANDLE* edit,
    const std::wstring& filepath,
    bool ignoreAspectRatio,
    bool invertPosition,
    bool asSubFilter,
    bool smoothEnable,
    int  smoothWindow,
    int  smoothPolyorder)
{
    if (results.empty()) return false;

    auto rect_list = results;
    auto err_list  = found;

    std::vector<UINT32>   inter_list;
    std::vector<FRMFIX>   fixedFrm;
    std::vector<FRMGROUP> groups;

    find_inter_frame(err_list, inter_list);

    struct Param {
        std::vector<cv::Rect2d>* rect_list;
        std::vector<bool>*       err_list;
        std::vector<UINT32>*     inter_list;
        std::vector<FRMFIX>*     fixedFrm;
        std::vector<FRMGROUP>*   groups;
        int  rangeStart;
        bool ignoreAspectRatio;
        bool invertPosition;
        bool asSubFilter;
        bool smoothEnable;
        int  smoothWindow;
        int  smoothPolyorder;
        std::string text;
        bool ok;
    } p { &rect_list, &err_list, &inter_list, &fixedFrm, &groups, rangeStart, ignoreAspectRatio, invertPosition, asSubFilter, smoothEnable, smoothWindow, smoothPolyorder, "", false };

    edit->call_edit_section_param(&p, [](void* v, EDIT_SECTION* edit) {
        auto* p = static_cast<Param*>(v);

        fix_frame(*p->rect_list, *p->err_list, *p->inter_list,
                  *p->fixedFrm, edit->info->width, edit->info->height, p->rangeStart, p->ignoreAspectRatio, p->invertPosition,
                  p->smoothEnable, p->smoothWindow, p->smoothPolyorder);
        groupObject(*p->fixedFrm, *p->groups, p->rangeStart);

        for (const auto& g : *p->groups) {
            p->text += p->asSubFilter
                ? make_alias_as_sub(*p->fixedFrm, g.vi_start, g.vi_end, p->ignoreAspectRatio)
                : make_alias(*p->fixedFrm, g.vi_start, g.vi_end, p->ignoreAspectRatio);
        }
        p->ok = true;
    });

    if (!p.ok) return false;

    // 既存ロジックはそのまま、書き込みだけCreateFile/WriteFileで行う
    HANDLE hFile = CreateFileW(
        filepath.c_str(), GENERIC_WRITE, 0, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD written = 0;
    BOOL wrote = WriteFile(hFile, p.text.data(), (DWORD)p.text.size(), &written, nullptr);
    CloseHandle(hFile);

    return wrote && written == p.text.size();
}

cv::Point InsertObject::getCenter(const cv::Rect2d& box) {
    return cv::Point(
        (int)((box.tl().x + box.br().x) / 2),
        (int)((box.tl().y + box.br().y) / 2)
    );
}

//Find single-frame error to be interpolate
//RETURN: a std::vector<UINT32> containing relevant index -> out_list
//RETURN: no. of inter-frame ->func return int
int InsertObject::find_inter_frame(std::vector<bool> &err_list, std::vector<UINT32> &out_list)
{
    //TODO
    int loop_last_index = err_list.size() - 3;
    int interfrm_count = 0;
    if (err_list.size() < 3)
    {
        return FALSE;
    }
    out_list.clear();
    for (int i = 0; i <= loop_last_index; i++)
    {
        bool S, M, E;
        S = err_list[i];
        M = err_list[i + 1];
        E = err_list[i + 2];
        if ((S && E) && !M)
        {
            interfrm_count++;
            out_list.push_back((UINT32)i + 1);
        }
    }
    return interfrm_count;
}

void InsertObject::interpolate(std::vector<cv::Rect2d> &rect_list, std::vector<bool> &err_list, const std::vector<UINT32> &inter_list)
{
    for (size_t f = 0; f < inter_list.size(); f++)
    {
        int v_idx = inter_list[f];

        cv::Point prevC(getCenter(rect_list[v_idx - 1]));
        int prevW = (int)rect_list[v_idx - 1].width;
        int prevH = (int)rect_list[v_idx - 1].height;

        cv::Point nextC(getCenter(rect_list[v_idx + 1]));
        int nextW = (int)rect_list[v_idx + 1].width;
        int nextH = (int)rect_list[v_idx + 1].height;

        int nowW = (prevW + nextW) / 2;
        int nowH = (prevH + nextH) / 2;
        int now_cx = (prevC.x + nextC.x) / 2;
        int now_cy = (prevC.y + nextC.y) / 2;

        rect_list[v_idx].x = now_cx - (nowW / 2);
        rect_list[v_idx].y = now_cy - (nowH / 2);
        rect_list[v_idx].width = nowW;
        rect_list[v_idx].height = nowH;
        err_list[v_idx] = true;
    }
}

void InsertObject::fix_frame(std::vector<cv::Rect2d> &rect_list, std::vector<bool> &err_list, std::vector<UINT32> &inter_list, std::vector<FRMFIX> &out, int frm_w, int frm_h, int rangeStart, bool ignoreAspectRatio, bool invertPosition, bool smoothEnable, int smoothWindow, int smoothPolyorder)
{
    //TODO
    //Interpolation phase
    interpolate(rect_list, err_list, inter_list);

    // SGフィルタで平滑化 (連続してトラッキングできている区間ごとに実施し、ロスト区間はまたがない)
    if (smoothEnable) {
        std::vector<double> cxArr(rect_list.size()), cyArr(rect_list.size());
        std::vector<double> wArr(rect_list.size()), hArr(rect_list.size());
        for (size_t i = 0; i < rect_list.size(); i++) {
            cv::Point c = getCenter(rect_list[i]);
            cxArr[i] = c.x;
            cyArr[i] = c.y;
            wArr[i]  = rect_list[i].width;
            hArr[i]  = rect_list[i].height;
        }

        size_t segStart = 0;
        bool inSeg = false;
        for (size_t i = 0; i <= err_list.size(); i++) {
            bool cur = (i < err_list.size()) && err_list[i];
            if (cur && !inSeg) { segStart = i; inSeg = true; }
            if (!cur && inSeg) {
                size_t segEnd = i - 1;
                smoothSegment(cxArr, (int)segStart, (int)segEnd, smoothWindow, smoothPolyorder);
                smoothSegment(cyArr, (int)segStart, (int)segEnd, smoothWindow, smoothPolyorder);
                smoothSegment(wArr,  (int)segStart, (int)segEnd, smoothWindow, smoothPolyorder);
                smoothSegment(hArr,  (int)segStart, (int)segEnd, smoothWindow, smoothPolyorder);
                inSeg = false;
            }
        }

        for (size_t i = 0; i < rect_list.size(); i++) {
            rect_list[i].width  = wArr[i];
            rect_list[i].height = hArr[i];
            rect_list[i].x = cxArr[i] - wArr[i] / 2.0;
            rect_list[i].y = cyArr[i] - hArr[i] / 2.0;
        }
    }

    //Transform to AviUtl coordiante
    int dX = frm_w / -2;
    int dY = frm_h / -2;
    for (size_t i = 0; i < rect_list.size(); i++)
    {
        // Ignore Aspect Ratio がOFFのとき、リサイズのX,Y(幅・高さ)が画面をはみ出さないようクランプ
        if (!ignoreAspectRatio) {
            if (rect_list[i].x < 0) {
                rect_list[i].width += rect_list[i].x;
                rect_list[i].x = 0;
            }
            if (rect_list[i].y < 0) {
                rect_list[i].height += rect_list[i].y;
                rect_list[i].y = 0;
            }
            if (rect_list[i].x + rect_list[i].width > frm_w) {
                rect_list[i].width = frm_w - rect_list[i].x;
            }
            if (rect_list[i].y + rect_list[i].height > frm_h) {
                rect_list[i].height = frm_h - rect_list[i].y;
            }
        }

        FRMFIX buf;
        cv::Point center(getCenter(rect_list[i]));
        buf.cx = center.x + dX;
        buf.cy = center.y + dY;
        if (invertPosition) {
            buf.cx = -buf.cx;
            buf.cy = -buf.cy;
        }
        buf.width = (int)rect_list[i].width;
        buf.height = (int)rect_list[i].height;
        buf.scale = std::max(rect_list[i].width, rect_list[i].height);
        buf.frame = (int)i + rangeStart;
        buf.found = err_list[i];
        out.push_back(buf); //store to output vector
    }
}

//Group into objects
void InsertObject::groupObject(std::vector<FRMFIX> &fixedframes, std::vector<FRMGROUP> &out, int rangeStart)
{
    //TODO
    std::vector<int> startpos;
    std::vector<int> endpos;
    bool prevstate = false;
    for (size_t i = 0; i < fixedframes.size(); i++)
    {
        bool currentstate = fixedframes[i].found;
        if (prevstate != currentstate) // a state change marking obj boundary
        {
            if (currentstate) //F->T = start
            {
                startpos.push_back(i);
            }
            else //T->F = end (prev frame)
            {
                endpos.push_back(i - 1);
            }

        }
        prevstate = currentstate;
    }
    //If endpos has 1 less item than startpos, add the last item back
    if (endpos.size() < startpos.size())
    {
        endpos.push_back(fixedframes.size() - 1);
    }
    //set output
    out.clear();
    if (startpos.size() > 0) //if there is at least 1 object
    {
        for (size_t i = 0; i < startpos.size(); i++)
        {
            FRMGROUP buf;
            buf.vi_start = startpos[i];
            buf.vi_end = endpos[i];
            buf.start = buf.vi_start + rangeStart;
            buf.end = buf.vi_end + rangeStart;
            out.push_back(buf);
        }
    }
}

void InsertObject::smoothSegment(std::vector<double>& data, int segStart, int segEnd, int window, int polyorder)
{
    int segLen = segEnd - segStart + 1;
    if (window < 3 || polyorder < 1 || segLen <= polyorder) return; // 平滑化できないほど短い区間はそのまま

    std::vector<double> result(segLen);
    for (int i = 0; i < segLen; i++) {
        // 区間の端では対称に取れる範囲まで窓を縮める
        int maxHalf = std::min(i, segLen - 1 - i);
        int half = std::min(window / 2, maxHalf);
        int w = half * 2 + 1;
        int order = std::min(polyorder, w - 1); // 次数は窓サイズ未満である必要がある

        if (w <= order) {
            result[i] = data[segStart + i];
            continue;
        }

        Eigen::RowVectorXd coeff = CalcSavGolCoeff((size_t)w, (unsigned int)order, 0, 1.0);

        double smoothed = 0.0;
        for (int j = -half; j <= half; j++) {
            smoothed += coeff[j + half] * data[segStart + i + j];
        }
        result[i] = smoothed;
    }

    for (int i = 0; i < segLen; i++) data[segStart + i] = result[i];
}

void InsertObject::ComputeSmoothPreview(
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
    std::vector<double>& outSmoothY)
{
    auto rect_list = results;
    auto err_list  = found;

    std::vector<UINT32> inter_list;
    find_inter_frame(err_list, inter_list);
    interpolate(rect_list, err_list, inter_list);

    outX.resize(rect_list.size());
    outY.resize(rect_list.size());
    outWidth.resize(rect_list.size());
    outHeight.resize(rect_list.size());
    for (size_t i = 0; i < rect_list.size(); i++) {
        cv::Point c = getCenter(rect_list[i]);
        outX[i] = c.x;
        outY[i] = c.y;
        outWidth[i] = rect_list[i].width;
        outHeight[i] = rect_list[i].height;
    }

    if (!smoothEnable) {
        outSmoothX.clear();
        outSmoothY.clear();
        return;
    }

    outSmoothX = outX;
    outSmoothY = outY;

    size_t segStart = 0;
    bool inSeg = false;
    for (size_t i = 0; i <= err_list.size(); i++) {
        bool cur = (i < err_list.size()) && err_list[i];
        if (cur && !inSeg) { segStart = i; inSeg = true; }
        if (!cur && inSeg) {
            size_t segEnd = i - 1;
            smoothSegment(outSmoothX, (int)segStart, (int)segEnd, smoothWindow, smoothPolyorder);
            smoothSegment(outSmoothY, (int)segStart, (int)segEnd, smoothWindow, smoothPolyorder);
            inSeg = false;
        }
    }
}
