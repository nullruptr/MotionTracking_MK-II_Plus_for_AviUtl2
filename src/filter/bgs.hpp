#pragma once
#include <string>
#include <windows.h>
#include "aviutl2_sdk/filter2.h"

#include "constants.hpp"

static std::wstring verstr_bgs = std::wstring(constants::bgs) + L" " + constants::version + L" by nullru";
extern FILTER_PLUGIN_TABLE filter_bgs;


