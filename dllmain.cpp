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
	std::wstring name;
	uint64_t size;
	FILETIME time;
};

struct PluginContext {
	std::vector<DirEntry> entries;	// список файлов
	size_t idx;
};


// --- Чтение .dir файла ---
bool LoadDirFile(const wchar_t* fileName, PluginContext *ctx)
{
	ctx->entries.clear();

	std::wifstream in(fileName);
	if (!in.is_open()) 
		return false;

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
					e.name = fd.cFileName;
					e.originalPath = path + L"\\" + fd.cFileName;
					e.size = (static_cast<uint64_t>(fd.nFileSizeHigh) << 32) | fd.nFileSizeLow;
					e.time = fd.ftLastWriteTime;
					ctx->entries.push_back(e);
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

	// OpenArchive (Обязательная)
	// Открывает *.dir файл.
	__declspec(dllexport) HANDLE __stdcall OpenArchiveW(tOpenArchiveDataW *ArchiveData)
	{
		PluginContext *ctx = new PluginContext;
		if (!LoadDirFile(ArchiveData->ArcName, ctx)) {
			ArchiveData->OpenResult = E_EOPEN;
			delete ctx;
			return nullptr;
		}
		ArchiveData->OpenResult = 0;
		
		ctx->entries.clear();
		ctx->idx = 0;
		return (HANDLE)ctx;
	}

	// ReadHeader (Обязательная)
	// Читает заголовок очередного файла в "архиве". TC будет вызывать эту функцию в цикле, пока вы не вернете E_END_ARCHIVE.
	__declspec(dllexport) int __stdcall ReadHeaderW(HANDLE hArcData, tHeaderDataExW *HeaderData)
	{
		PluginContext *ctx = (PluginContext*)hArcData;
		if (ctx->idx >= ctx->entries.size()) {
			ctx->idx = 0;
			return E_END_ARCHIVE;
		}
		const auto &e = ctx->entries[ctx->idx++];
		wcscpy_s(HeaderData->FileName, e.name.c_str());
		
		HeaderData->UnpSize = (long)e.size;
		FileTimeToDosDateTime(&e.time, ((LPWORD)&HeaderData->FileTime) + 1, (LPWORD)&HeaderData->FileTime);
		HeaderData->FileAttr = FILE_ATTRIBUTE_NORMAL;

		return 0;
	}

	// ProcessFile(Обязательная)
	// Вызывается, когда пользователь хочет выполнить действие с файлом(распаковать, скопировать, пропустить).
	__declspec(dllexport) int __stdcall ProcessFileW(HANDLE hArcData, int Operation, wchar_t *DestPath, wchar_t *DestName)
	{
		PluginContext *ctx = (PluginContext*)hArcData;
		if (Operation == PK_SKIP) 
			return 0;

		// ищем файл по имени...
		int fileIndex = 0;
		
	//	if (g_currentIndex == 0) 
	//		return E_NO_FILES; // safety

		const auto &e = ctx->entries[fileIndex]; // последний выданный файл

		if (Operation == PK_TEST) {
			// можем просто вернуть успех (нет проверки)
			return 0;
		}

		if (Operation == PK_EXTRACT) {
			std::wstring dest = DestPath;
			if (!dest.empty() && dest.back() != L'\\') dest += L'\\';
			dest += DestName;

			if (!CopyFileW(e.originalPath.c_str(), dest.c_str(), FALSE)) {
				return E_EWRITE;
			}
		}
		return 0;
	}

	// CloseArchive (Обязательная)
	// Закрывает "архив" и освобождает ресурсы.
	__declspec(dllexport) int __stdcall CloseArchive(HANDLE hArcData)
	{
		PluginContext *ctx = (PluginContext*)hArcData;
		ctx->entries.clear();
		delete ctx;
		return 0;
	}

}

// Экспортируемые символы
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) 
{
	return TRUE;
}
