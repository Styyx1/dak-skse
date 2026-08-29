#pragma once

#define NOMINMAX
#include "SKSEMenuFramework.h"
#undef ERROR

namespace DAK
{

namespace UI
{
void Reg();
namespace DAKMenu
{
inline std::string Hotkey = "Dynamic Activation Key";
void __stdcall Render();
void DrawHotkey();
} // namespace DAKMenu
} // namespace UI

} // namespace DAK