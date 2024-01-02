#pragma once
#include <string>

typedef void (*printFuncP)(const WCHAR* str);

void connectAndFetchFromDB(HWND hWnd, printFuncP consolePrintFunc, std::wstring *dateStr = nullptr);

void closeSQLEnvironment();