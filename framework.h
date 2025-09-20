// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

// tools
std::wstring AnsiToWide(const char* str);
std::string WideToAnsi(const std::wstring& wstr);
std::wstring CreateUniqueName(const std::wstring& directory, const std::wstring& originalName);
std::wstring MaskToRegex(const std::wstring& mask);

// vdir
HANDLE OpenDirFile(const std::wstring &arcname, int &rOpenResult);
int    ReadDirEntry(HANDLE hArcData, std::wstring &rFileName, unsigned int &rUnpSize, int &rFileTime, int &rFileAttr);
int    ProcDirEntry(HANDLE hArcData, int Operation, const std::wstring &DestPath, const std::wstring &DestName);
void   CloseDirFile(HANDLE hArcData);


