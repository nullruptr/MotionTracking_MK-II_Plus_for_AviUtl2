#pragma once
#include <string>
#include <windows.h>
#include "aviutl2_sdk/filter2.h"
#include "constants.hpp"

static std::wstring verstr_hsv = std::wstring(constants::hsv) + L" " + constants::version + L" by nullru";
static const std::wstring hsv_label = std::wstring(constants::WindowName) + L"\\Pre-track";
extern FILTER_PLUGIN_TABLE filter_hsv;
