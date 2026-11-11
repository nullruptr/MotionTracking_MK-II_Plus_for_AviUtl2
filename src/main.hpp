#pragma once
#include <windows.h>
#include "constants.hpp"
#include "aviutl2_sdk/plugin2.h"
#include "ui/mainframe.hpp"

extern HINSTANCE hModuleDLL;
extern MainFrame* g_frame;

static std::wstring verstr = std::wstring(constants::WindowName) + L" " + constants::version + L" by nullru";
