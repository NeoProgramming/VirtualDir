#include <string>
#include <Windows.h>

// Конвертация из ANSI (Total Commander) -> wide (Windows API)
std::wstring AnsiToWide(const char* str) 
{
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
