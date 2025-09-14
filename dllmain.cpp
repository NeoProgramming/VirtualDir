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

namespace fs = std::filesystem;

// --- Структура для хранения найденных файлов ---
struct DirEntry {
	std::wstring originalPath;  // полный путь к реальному файл
	std::string name;
	uint64_t size;
	FILETIME time;
};

// --- Глобальный список файлов ---
static std::vector<DirEntry> g_entries;

// --- Чтение .dir файла ---
bool LoadDirFile(const char* fileName) 
{
	g_entries.clear();

	std::wifstream in(fileName);
	if (!in.is_open()) return false;

	std::wstring line;
	while (std::getline(in, line)) {
		// ожидаем строку вида: C:\MyPhotos\beach*.jpg
		size_t pos = line.find_last_of(L"\\/");
		if (pos == std::wstring::npos) continue;

		std::wstring path = line.substr(0, pos);
		std::wstring mask = line.substr(pos + 1);

		WIN32_FIND_DATAW fd;
		HANDLE hFind = FindFirstFileW((path + L"\\" + mask).c_str(), &fd);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
					DirEntry e;
					e.name = WideToAnsi(fd.cFileName);
					e.originalPath = path + L"\\" + fd.cFileName;
					e.size = (static_cast<uint64_t>(fd.nFileSizeHigh) << 32) | fd.nFileSizeLow;
					e.time = fd.ftLastWriteTime;
					g_entries.push_back(e);
				}
			} while (FindNextFileW(hFind, &fd));
			FindClose(hFind);
		}
	}
	return true;
}

// --- Основные функции WCX API ---

// Инициализация
extern "C" {
	__declspec(dllexport) void __stdcall SetChangeVolProc(HANDLE hArcData, tChangeVolProc pChangeVolProc1) {}
	__declspec(dllexport) void __stdcall SetProcessDataProc(HANDLE hArcData, tProcessDataProc pProcessDataProc) {}

	__declspec(dllexport) HANDLE __stdcall OpenArchive(tOpenArchiveData *ArchiveData)
	{
		if (!LoadDirFile(ArchiveData->ArcName)) {
			ArchiveData->OpenResult = E_EOPEN;
			return nullptr;
		}
		ArchiveData->OpenResult = 0;
		return (HANDLE)1; // фиктивный handle
	}

	static size_t g_currentIndex = 0;

	__declspec(dllexport) int __stdcall ReadHeader(HANDLE hArcData, tHeaderData *HeaderData)
	{
		static size_t idx = 0;
		if (idx >= g_entries.size()) {
			idx = 0;
			return E_END_ARCHIVE;
		}
		const auto &e = g_entries[idx++];
		strcpy_s(HeaderData->FileName, e.name.c_str());
		HeaderData->UnpSize = (long)e.size;
		FileTimeToDosDateTime(&e.time, ((LPWORD)&HeaderData->FileTime) + 1, (LPWORD)&HeaderData->FileTime);
		HeaderData->FileAttr = FILE_ATTRIBUTE_NORMAL;
		g_currentIndex++;
		return 0;
	}

	__declspec(dllexport) int __stdcall ProcessFile(HANDLE hArcData, int Operation, char *DestPath, char *DestName)
	{
		if (Operation == PK_SKIP) return 0;
		if (g_currentIndex == 0) return E_NO_FILES; // safety

		const auto &e = g_entries[g_currentIndex - 1]; // последний выданный файл

		if (Operation == PK_TEST) {
			// можем просто вернуть успех (нет проверки)
			return 0;
		}

		if (Operation == PK_EXTRACT) {
			std::wstring dest = AnsiToWide(DestPath);
			if (!dest.empty() && dest.back() != L'\\') dest += L'\\';
			dest += AnsiToWide(DestName);

			if (!CopyFileW(e.originalPath.c_str(), dest.c_str(), FALSE)) {
				return E_EWRITE;
			}
		}
		return 0;
	}

	__declspec(dllexport) int __stdcall CloseArchive(HANDLE hArcData)
	{
		g_entries.clear();
		return 0;
	}

}

// Экспортируемые символы
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) 
{
	return TRUE;
}
