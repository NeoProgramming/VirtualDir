// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>


std::wstring AnsiToWide(const char* str);
std::string WideToAnsi(const std::wstring& wstr);

