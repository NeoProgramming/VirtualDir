#include <string>
#include <Windows.h>

// ����������� �� ANSI (Total Commander) -> wide (Windows API)
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

// ����������� �������: wide -> ANSI (��� HeaderData.FileName)
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
	// ��������� ������ ���� � ������������� �����
	std::wstring fullPath = directory + L"\\" + originalName;

	// ���������, ���������� �� ������������ ����
	DWORD attributes = GetFileAttributesW(fullPath.c_str());
	if (attributes == INVALID_FILE_ATTRIBUTES) {
		// ���� �� ���������� - ���������� ������������ ���
		return originalName;
	}

	// ��������� ��� � ���������� �����
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

	// �������� ����� ��������� ��� � �������� ���������
	for (int i = 1; i < 10000; ++i) {
		std::wstring testName = nameWithoutExt + L" (" + std::to_wstring(i) + L")" + extension;
		std::wstring testPath = directory + L"\\" + testName;

		// ���������, ���������� �� ���� � ����� ������
		attributes = GetFileAttributesW(testPath.c_str());
		if (attributes == INVALID_FILE_ATTRIBUTES) {
			// ���� �� ���������� - ����� ��������� ���
			return testName;
		}
	}

	// ���� �� ����� ��������� ��� �� 10000 �������, ���������� ������ ������
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
