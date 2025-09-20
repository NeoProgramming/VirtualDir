#include <string>
#include <Windows.h>

// Конвертация из ANSI (Total Commander) -> wide (Windows API)
std::wstring AnsiToWide(const char* str) 
{
	if (str == NULL)
		return L"";
	int len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	std::wstring result(len, L'\0');
	MultiByteToWideChar(CP_ACP, 0, str, -1, &result[0], len);
	if (!result.empty() && result.back() == L'\0') result.pop_back();
	return result;
}

// Конвертация обратно: wide -> ANSI (для HeaderData.FileName)
std::string WideToAnsi(const std::wstring& wstr) 
{
	int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
	std::string result(len, '\0');
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &result[0], len, NULL, NULL);
	if (!result.empty() && result.back() == '\0') result.pop_back();
	return result;
}

std::wstring CreateUniqueName(const std::wstring& directory, const std::wstring& originalName)
{
	// Формируем полный путь к оригинальному файлу
	std::wstring fullPath = directory + L"\\" + originalName;

	// Проверяем, существует ли оригинальный файл
	DWORD attributes = GetFileAttributesW(fullPath.c_str());
	if (attributes == INVALID_FILE_ATTRIBUTES) {
		// Файл не существует - возвращаем оригинальное имя
		return originalName;
	}

	// Извлекаем имя и расширение файла
	size_t dotPos = originalName.find_last_of(L'.');
	std::wstring nameWithoutExt;
	std::wstring extension;

	if (dotPos != std::wstring::npos) {
		nameWithoutExt = originalName.substr(0, dotPos);
		extension = originalName.substr(dotPos);
	}
	else {
		nameWithoutExt = originalName;
		extension = L"";
	}

	// Пытаемся найти свободное имя с числовым суффиксом
	for (int i = 1; i < 10000; ++i) {
		std::wstring testName = nameWithoutExt + L" (" + std::to_wstring(i) + L")" + extension;
		std::wstring testPath = directory + L"\\" + testName;

		// Проверяем, существует ли файл с таким именем
		attributes = GetFileAttributesW(testPath.c_str());
		if (attributes == INVALID_FILE_ATTRIBUTES) {
			// Файл не существует - нашли свободное имя
			return testName;
		}
	}

	// Если не нашли свободное имя за 10000 попыток, возвращаем пустую строку
	return L"";
}

std::wstring MaskToRegex(const std::wstring& mask) 
{
	std::wstring regex_pattern;
	for (wchar_t c : mask) {
		switch (c) {
		case L'*': regex_pattern += L".*"; break;
		case L'?': regex_pattern += L"."; break;
		case L'.': regex_pattern += L"\\."; break;
		case L'\\': regex_pattern += L"\\\\"; break;
		default: regex_pattern += c; break;
		}
	}
	return regex_pattern;
}
