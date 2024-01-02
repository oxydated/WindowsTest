#include "HTTPTest.h"
#include <cstdlib>


void connectAndExecuteRequest(printFuncP consolePrintFunc, std::wstring URL) {

	HINTERNET hSession = WinHttpOpen(
		L"HTTP Test",
		WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
		WINHTTP_NO_PROXY_NAME,
		WINHTTP_NO_PROXY_BYPASS,
		0
	);

	if (hSession == NULL) {
		std::runtime_error("Failed to open HTTP session!\n");
		return;
	}

	HINTERNET hConn = WinHttpConnect(
		hSession,
		(LPCWSTR)URL.c_str(),
		INTERNET_DEFAULT_HTTP_PORT,
		0
	);

	if (hConn == NULL) {
		DWORD error = GetLastError();
		std::runtime_error("Failed to make a HTTP connection!\n");
		WinHttpCloseHandle(hSession);
		return;
	}

	HINTERNET hRequest = WinHttpOpenRequest(
		hConn,
		NULL,
		NULL,
		NULL,
		WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES,
		WINHTTP_FLAG_REFRESH
	);

	if (hRequest == NULL) {
		std::runtime_error("Failed to open a HTTP request!\n");
		WinHttpCloseHandle(hConn);
		WinHttpCloseHandle(hSession);
		return;
	}

	BOOL bResult = WinHttpSendRequest(
		hRequest,
		WINHTTP_NO_ADDITIONAL_HEADERS,
		0,
		WINHTTP_NO_REQUEST_DATA,
		0,
		0,
		NULL
	);

	if (!bResult) {
		std::runtime_error("Failed to send a HTTP request!\n");
	}

	bResult = WinHttpReceiveResponse(hRequest, NULL);

	if (!bResult) {
		DWORD error = GetLastError();
		std::runtime_error("Failed to receive HTTP response!\n");
	}
	else {
		BOOL keepReading = true;
		while (keepReading) {
			DWORD bytesAvailable = 0;
			bResult = WinHttpQueryDataAvailable(hRequest, &bytesAvailable);
			keepReading = (bResult) && (bytesAvailable > 0);

			if (!bResult) {
				DWORD error = GetLastError();
				std::runtime_error("Failed to get HTTP response bytes available!\n");
			}
			else {
				char* data = new char[bytesAvailable + 1];
				if (data) {
					ZeroMemory(data, bytesAvailable + 1);
					DWORD totalDownloaded = 0;

					BOOL readResult = WinHttpReadData(hRequest, (LPVOID)data, bytesAvailable, &totalDownloaded);

					if (!readResult) {
						DWORD error = GetLastError();
						std::runtime_error("Failed to read HTTP response data!\n");
					}
					else {
						char* context = nullptr;
						char step[] = "\n";
						char* tok = strtok_s(data, step, &context);
						while (tok) {
							WCHAR toPrint[400];
							size_t retval;
							errno_t error = mbstowcs_s(&retval, toPrint, tok, strlen(tok));
							consolePrintFunc(toPrint);
							consolePrintFunc(L"\n");
							tok = strtok_s(NULL, step, &context);
						}
					}
				}
			}
		}
		consolePrintFunc(L"\n\n");

	}

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConn);
	WinHttpCloseHandle(hSession);
}