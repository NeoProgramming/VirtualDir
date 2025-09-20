// dllmain.cpp : Defines the entry point for the DLL application.
#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include "framework.h"
extern "C" {
#include "wcxhead.h"   // заголовок из SDK Total Commander
}

/*
#ifndef _WIN64 
#pragma comment(linker, "/EXPORT:OpenArchive=_OpenArchive@4")
#pragma comment(linker, "/EXPORT:ReadHeader=_ReadHeader@8")
#pragma comment(linker, "/EXPORT:ProcessFile=_ProcessFile@16")
#pragma comment(linker, "/EXPORT:CloseArchive=_CloseArchive@4")
#pragma comment(linker, "/EXPORT:GetPackerCaps=_GetPackerCaps@0")

#pragma comment(linker, "/EXPORT:OpenArchiveW=_OpenArchiveW@4")
#pragma comment(linker, "/EXPORT:ReadHeaderExW=_ReadHeaderExW@8")
#pragma comment(linker, "/EXPORT:ProcessFileW=_ProcessFileW@16")
#pragma comment(linker, "/EXPORT:CloseArchive=_CloseArchive@4")
#pragma comment(linker, "/EXPORT:GetPackerCaps=_GetPackerCaps@0")
#endif
*/

// WCX API functions

extern "C" {
	__declspec(dllexport) void __stdcall SetChangeVolProc(HANDLE hArcData, tChangeVolProc pChangeVolProc1) {}
	__declspec(dllexport) void __stdcall SetProcessDataProc(HANDLE hArcData, tProcessDataProc pProcessDataProc) {}

	// OpenArchive (Обязательная); Открывает *.dir файл.
//	__declspec(dllexport) HANDLE __stdcall OpenArchiveW(tOpenArchiveDataW *ArchiveData)
//	{
//		return OpenDirFile(ArchiveData->ArcName, ArchiveData->OpenResult);
//	}
	__declspec(dllexport) HANDLE __stdcall OpenArchive(tOpenArchiveData *ArchiveData)
	{
		return OpenDirFile(AnsiToWide(ArchiveData->ArcName), ArchiveData->OpenResult);
	}

	// ReadHeader (Обязательная)
	// Читает заголовок очередного файла в "архиве". TC будет вызывать эту функцию в цикле, пока вы не вернете E_END_ARCHIVE.
//	__declspec(dllexport) int __stdcall ReadHeaderExW(HANDLE hArcData, tHeaderDataExW *HeaderData)
//	{
//		std::wstring fname;
//		int r = ReadDirEntry(hArcData, fname, HeaderData->UnpSize, HeaderData->FileTime, HeaderData->FileAttr);
//		wcscpy_s(HeaderData->FileName, fname.c_str());
//		return r;
//	}
	__declspec(dllexport) int __stdcall ReadHeader(HANDLE hArcData, tHeaderData *HeaderData)
	{
		std::wstring fname;
		unsigned int fsize;
		int r = ReadDirEntry(hArcData, fname, fsize, HeaderData->FileTime, HeaderData->FileAttr);
		strcpy_s(HeaderData->FileName, WideToAnsi(fname).c_str());
		HeaderData->UnpSize = fsize;
		return r;
	}

	// ProcessFile(Обязательная)
	// Вызывается, когда пользователь хочет выполнить действие с файлом(распаковать, скопировать, пропустить).
//	__declspec(dllexport) int __stdcall ProcessFileW(HANDLE hArcData, int Operation, wchar_t *DestPath, wchar_t *DestName)
//	{
//		return ProcDirEntry(hArcData, Operation, DestPath, DestName);
//	}
	__declspec(dllexport) int __stdcall ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName)
	{
		return ProcDirEntry(hArcData, Operation, AnsiToWide(DestPath), AnsiToWide(DestName));
	}

	__declspec(dllexport) int __stdcall GetPackerCaps(void) 
	{
		// Ваш плагин определяет архив по содержимому (расширению .dir)
		// Он не создает новые архивы и не модифицирует существующие
		return PK_CAPS_BY_CONTENT;
	}

	// CloseArchive (Обязательная)
	// Закрывает "архив" и освобождает ресурсы.
	__declspec(dllexport) int __stdcall CloseArchive(HANDLE hArcData)
	{
		CloseDirFile(hArcData);
		return 0;
	}
}

// Экспортируемые символы
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) 
{
	return TRUE;
}
