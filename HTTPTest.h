#pragma once

#include <windows.h>
#include <winhttp.h>
#include <stdexcept>
#include "SQLTest.h"

void connectAndExecuteRequest(printFuncP consolePrintFunc, std::wstring URL);