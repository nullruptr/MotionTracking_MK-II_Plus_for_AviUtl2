#pragma once
#include <string>
#include <windows.h>
#include <opencv2/opencv.hpp>
#include "aviutl2_sdk/filter2.h"
#include "aviutl2_sdk/plugin2.h"

#include "constants.hpp"

static std::wstring verstr_bgs = std::wstring(constants::bgs) + L" " + constants::version + L" by nullru";
static const std::wstring bgs_label = std::wstring(constants::WindowName);
extern FILTER_PLUGIN_TABLE filter_bgs;


