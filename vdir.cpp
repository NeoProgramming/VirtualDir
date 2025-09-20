#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <regex>
#include "framework.h"
extern "C" {
#include "wcxhead.h"   // заголовок из SDK Total Commander
}

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
	size_t proc_idx;
};

namespace fs = std::filesystem;

void ScanDir(PluginContext *ctx, const std::wstring &path, const std::wstring &mask)
{
	WIN32_FIND_DATAW fd;
	HANDLE hFind = FindFirstFileW((path + L"\\*.*").c_str(), &fd);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				std::wregex pattern(MaskToRegex(mask));
				if (std::regex_match(fd.cFileName, pattern)) {
					// Файл подходит
					DirEntry e;
					e.name = fd.cFileName;
					e.originalPath = path + L"\\" + fd.cFileName;
					e.size = (static_cast<uint64_t>(fd.nFileSizeHigh) << 32) | fd.nFileSizeLow;
					e.time = fd.ftLastWriteTime;
					ctx->entries.push_back(e);
				}				
			}
			else if (wcscmp(fd.cFileName, L".") && wcscmp(fd.cFileName, L"..")) {
				ScanDir(ctx, path + L"\\" + fd.cFileName, mask);
			}
		} while (FindNextFileW(hFind, &fd));
		FindClose(hFind);
	}
}

// load .dir file
bool LoadDirFile(PluginContext *ctx, const wchar_t* fileName)
{
	ctx->entries.clear();

	std::wifstream in(fileName);
	if (!in.is_open())
		return false;

	std::wstring line;
	while (std::getline(in, line)) {
		// wait string like: C:\MyPhotos\beach*.jpg
		size_t pos = line.find_last_of(L"\\/");
		if (pos == std::wstring::npos) continue;

		std::wstring path = line.substr(0, pos);
		std::wstring mask = line.substr(pos + 1);

		ScanDir(ctx, path, mask);
	}
	return true;
}

HANDLE OpenDirFile(const std::wstring &arcname, int &rOpenResult)
{
	PluginContext *ctx = new PluginContext;
	ctx->entries.clear();
	ctx->idx = 0;
	ctx->proc_idx = 0;

	if (!LoadDirFile(ctx, arcname.c_str())) {
		rOpenResult = E_EOPEN;
		delete ctx;
		return nullptr;
	}
	rOpenResult = 0;
	return (HANDLE)ctx;
}

int ReadDirEntry(HANDLE hArcData, std::wstring &rFileName, unsigned int &rUnpSize, int &rFileTime, int &rFileAttr)
{
	PluginContext *ctx = (PluginContext*)hArcData;
	if (ctx->idx >= ctx->entries.size()) {
		ctx->idx = 0;
		return E_END_ARCHIVE;
	}
	const auto &e = ctx->entries[ctx->idx++];
	rFileName = e.name;

	rUnpSize = (long)e.size;

	FileTimeToDosDateTime(&e.time, ((LPWORD)&rFileTime) + 1, (LPWORD)&rFileTime);
	rFileAttr = FILE_ATTRIBUTE_NORMAL;

	return 0;
}

int ProcDirEntry(HANDLE hArcData, int Operation, const std::wstring &DestPath, const std::wstring &DestName)
{
	//const wchar_t *DestPath,  // Путь, КУДА распаковать (папка назначения)
	//const wchar_t *DestName   // Имя, под которым сохранить файл В DestPath
	// if (DestPath == NULL):   // Это специальный сигнал!  Цель: Дать TC временный файл для просмотра/запуска
	// else: ПОЛЬЗОВАТЕЛЬ НАЖАЛ F5 (Распаковать); Цель: Скопировать файл в указанную пользователем папку

	PluginContext *ctx = (PluginContext*)hArcData;
	if (Operation == PK_SKIP)
		return 0;
	if (ctx->entries.size() == 0)
		return E_NO_FILES;

	if (ctx->proc_idx >= ctx->entries.size()) {
		ctx->proc_idx = 0; // <- Это отдельный индекс для ProcessFile!
	}

	// берем информацию о файле
	const auto &e = ctx->entries[ctx->proc_idx];
	ctx->proc_idx++; // Готовимся к следующему вызову

	if (Operation == PK_TEST) {
		// можем просто вернуть успех (нет проверки)
		return 0;
	}

	if (Operation == PK_EXTRACT) {
		if (DestPath == L"") {
			// Это запрос на просмотр/запуск для current_entry
			// ... создаем симлинк/копию для current_entry->real_path ...
			wchar_t temp_path[MAX_PATH];
			GetTempPathW(MAX_PATH, temp_path);
			std::wstring temp_file = CreateUniqueName(temp_path, DestName);

			if (!CreateSymbolicLinkW(temp_file.c_str(), e.originalPath.c_str(), 0)) {
				return E_EWRITE;
			}
		}
		else {
			// Это распаковка для current_entry
			// ... копируем current_entry->real_path в DestPath\DestName ...
			std::wstring dest = DestPath;
			if (!dest.empty() && dest.back() != L'\\')
				dest += L'\\';
			dest += DestName;
			if (!CopyFileW(e.originalPath.c_str(), dest.c_str(), FALSE)) {
				return E_EWRITE;
			}
		}
	}
	return 0;
}

void CloseDirFile(HANDLE hArcData)
{
	PluginContext *ctx = (PluginContext*)hArcData;
	ctx->entries.clear();
	delete ctx;
}

