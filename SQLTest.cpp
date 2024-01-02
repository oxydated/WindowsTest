#include <windows.h>
#include <windowsx.h>
#include <sql.h>
#include <sqltypes.h>
#include <sqlext.h>
#include <stdexcept>
#include <string>
#include "resource.h"
#include "SQLTest.h"


#define OUTCONNECTIONSTRINGMINLEN 1024

SQLHENV henv = NULL;
BOOL driveSuccesfullySelected = FALSE;
SQLWCHAR DriverDescription[200];


void fetchDataFromStatement(SQLHSTMT hstmt, printFuncP consolePrintFunc);

void executeQuery(SQLHSTMT hstmt, printFuncP consolePrintFunc);

void executeStoredProcedure(SQLHSTMT hstmt, printFuncP consolePrintFunc, std::wstring* dateStr);

void printToConsole(const WCHAR* str) {
	HANDLE stdOut;
	stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD written = 0;
	WriteConsole(stdOut, str, wcslen(str), &written, NULL);

	HWND consoleWindow = GetConsoleWindow();

	RECT rect;

	if (GetClientRect(consoleWindow, &rect) != 0) {
		InvalidateRect(consoleWindow, &rect, TRUE);
		UpdateWindow(consoleWindow);
	}
}

void printErrorToConsole(const WCHAR* str) {
	printToConsole(str);
	printToConsole(L"\n");

	char mbstr[400];
	size_t retval;
	errno_t error = wcstombs_s(&retval, mbstr, str, wcslen(str));

	throw std::runtime_error(mbstr);
}

void getSQLError(SQLSMALLINT handleType, SQLHANDLE handle) {
	SQLWCHAR SQLState[6];
	SQLSMALLINT RecNumber = 1;
	SQLINTEGER NativeErrorPtr = 0;
	SQLWCHAR MessageText[400];
	SQLSMALLINT TextLengthPtr = 0;

	SQLRETURN checkRet = SQL_SUCCESS;

	while (checkRet == SQL_SUCCESS) {
		checkRet = SQLGetDiagRec(
			handleType,
			handle,
			RecNumber++,
			SQLState,
			&NativeErrorPtr,
			MessageText,
			400,
			&TextLengthPtr
		);
		printToConsole(MessageText);
		printToConsole(L"\n");
	}
}

SQLHENV getSQLEnvironmentHandle() {

	if (henv == NULL) {
		SQLRETURN ret;

		ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			getSQLError(SQL_HANDLE_ENV, SQL_NULL_HANDLE);
			printErrorToConsole(L"Failed to alloc environment handle\n");
		}

		ret = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);

		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			getSQLError(SQL_HANDLE_ENV, henv);
			printErrorToConsole(L"Failed to set environment attribute\n");
		}
		
	}

	return henv;
}

INT_PTR CALLBACK DriverPicker(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	HWND hComb = NULL;
	SQLHENV hEnv = getSQLEnvironmentHandle();
	SQLRETURN ret;

	SQLSMALLINT DescriptionLegth = 0;
	SQLSMALLINT AttributesLength = 0;

	int selectedIndex = 0;

	switch (message) {

	case WM_INITDIALOG:
		hComb = GetDlgItem(hDlg, IDC_DRIVERCOMBO);
		ret = SQLDrivers(hEnv, SQL_FETCH_FIRST, DriverDescription, 200, &DescriptionLegth, NULL, 0, &AttributesLength);
		ComboBox_AddString(hComb, DriverDescription);
		while (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
			ret = SQLDrivers(hEnv, SQL_FETCH_NEXT, DriverDescription, 200, &DescriptionLegth, NULL, 0, &AttributesLength);
			ComboBox_AddString(hComb, DriverDescription);
		}
		ComboBox_SetCurSel(hComb, 0);
		return (INT_PTR)TRUE;
		break;

	case WM_COMMAND:
		if (wParam == IDOK || wParam == IDCANCEL) {
			if (wParam == IDOK) {
				hComb = GetDlgItem(hDlg, IDC_DRIVERCOMBO);
				selectedIndex = ComboBox_GetCurSel(hComb);
				ComboBox_GetLBText(hComb, selectedIndex, DriverDescription);
			}
			EndDialog(hDlg, TRUE);
		}
		return (INT_PTR)TRUE;
		break;

	}
	return (INT_PTR)FALSE;
}

void chooseSQLDriver(HWND hWnd)
{
	HMODULE hInst = GetModuleHandle(NULL);

	DialogBox(hInst, MAKEINTRESOURCE(IDD_DRIVERDIALOG), hWnd, DriverPicker);

}

void closeSQLEnvironment() {

	if (henv) {
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
}

void connectAndFetchFromDB(HWND hWnd, printFuncP consolePrintFunc, std::wstring* dateStr) {
	// STEP 1: CONNECT

	SQLRETURN ret;
	SQLHDBC hcon;

	if (henv == NULL || !driveSuccesfullySelected) {
		chooseSQLDriver(hWnd);
	}

	ret = SQLAllocHandle(SQL_HANDLE_DBC, getSQLEnvironmentHandle(), &hcon);

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		getSQLError(SQL_HANDLE_ENV, getSQLEnvironmentHandle());
		printErrorToConsole(L"Failed to alloc connection handle\n");
	}

	std::wstring connectionString = L"";

	SQLWCHAR outConnectionString[OUTCONNECTIONSTRINGMINLEN];

#define __USE_CONNECTION_STRING__

#ifdef __USE_CONNECTION_STRING__
	connectionString += L"DRIVER={";
	connectionString += DriverDescription;
	connectionString += L"}; ";
	connectionString += L"Database=bludb;";
	connectionString += L"Hostname=6667d8e9-9d4d-4ccb-ba32-21da3bb5aafc.c1ogj3sd0tgtu0lqde00.databases.appdomain.cloud;";
	connectionString += L"PORT=30376;";
	connectionString += L"PROTOCOL=TCPIP;";
	connectionString += L"UID=ztj17127;";
	connectionString += L"PWD=g1gy3vct6IkXAhLE;";
	connectionString += L"Security=SSL;";
	connectionString += L"CurrentSchema=ZTJ17127;";

#else

	connectionString += L"DSN=blu;";

#endif

	connectionString.shrink_to_fit();

	printToConsole(L"Connection string: \n");
	printToConsole(connectionString.c_str());
	printToConsole(L"\n\n");

	SQLSMALLINT outConnStringLen = 0;


	ret = SQLDriverConnect(
		hcon,
		NULL,
		(SQLWCHAR*)connectionString.c_str(),
		(SQLSMALLINT)connectionString.length(),
		outConnectionString,
		OUTCONNECTIONSTRINGMINLEN,
		&outConnStringLen,
		SQL_DRIVER_COMPLETE
	);

	if (ret != SQL_SUCCESS) {
		getSQLError(SQL_HANDLE_DBC, hcon);

		if (ret != SQL_SUCCESS_WITH_INFO) {
			printErrorToConsole(L"Failed to connect\n");
		}
	}
	else {
		driveSuccesfullySelected = TRUE;
	}

	// STEP 2: INITIALIZE

	SQLHSTMT hstmt;

	ret = SQLAllocHandle(SQL_HANDLE_STMT, hcon, &hstmt);

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		getSQLError(SQL_HANDLE_DBC, hcon);
		printErrorToConsole(L"Failed to alloc statement handle\n");
	}

	if (dateStr) {
		executeStoredProcedure(hstmt, consolePrintFunc, dateStr);
	}
	else {
		executeQuery(hstmt, consolePrintFunc);
	}

	// STEP 5: CLOSE TRANSACTION
	
	ret = SQLEndTran(SQL_HANDLE_DBC, hcon, SQL_COMMIT);

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		getSQLError(SQL_HANDLE_DBC, hcon);
		printErrorToConsole(L"Failed to commit transaction");
	}

	// STEP 6: DISCONNECT
	if (hstmt) {
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	}

	if (hcon) {
		SQLDisconnect(hcon);
		SQLFreeHandle(SQL_HANDLE_DBC, hcon);
	}
}

void executeQuery(SQLHSTMT hstmt, printFuncP consolePrintFunc) {

	// STEP 3: EXECUTE

	SQLRETURN ret;

	std::wstring query = L"select * from FERIADO";

	ret = SQLPrepare(hstmt, (SQLWCHAR*)query.c_str(), SQL_NTS);

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		getSQLError(SQL_HANDLE_STMT, hstmt);
		printErrorToConsole(L"Failed to prepare statement");
	}
	else {
		ret = SQLExecute(hstmt);

		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {

			if (ret != SQL_NO_DATA) {
				getSQLError(SQL_HANDLE_STMT, hstmt);
				printErrorToConsole(L"Failed to prepare statement");
			}
		}
		else {
			fetchDataFromStatement(hstmt, consolePrintFunc);

			SQLCloseCursor(hstmt);
		}
	}
}

void executeStoredProcedure(SQLHSTMT hstmt, printFuncP consolePrintFunc, std::wstring* dateStr) {
	
	SQLRETURN ret;

	/*SQLWCHAR ParameterValue[255] = L"2023-01-02";*/
	SQLSMALLINT ParamNum = 1;
	SQLSMALLINT InputOutputType = SQL_PARAM_INPUT;
	SQLSMALLINT ValueType = SQL_C_WCHAR;
	SQLSMALLINT ParameterType = SQL_VARCHAR;
	SQLSMALLINT DecimalDigits = 0;

	ret = SQLBindParameter(hstmt, ParamNum, InputOutputType, ValueType, ParameterType, 255, 0, (SQLPOINTER)dateStr->c_str(), dateStr->length(), NULL);

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		getSQLError(SQL_HANDLE_STMT, hstmt);
		printErrorToConsole(L"Failed to bind parameter");
	}	

	SQLWCHAR ParameterValueOut[255];
	ParamNum = 2;
	InputOutputType = SQL_PARAM_OUTPUT;
	ValueType = SQL_C_WCHAR;
	ParameterType = SQL_VARCHAR;
	DecimalDigits = 0;

	ret = SQLBindParameter(hstmt, ParamNum, InputOutputType, ValueType, ParameterType, 255, 0, (SQLPOINTER)ParameterValueOut, 255, NULL);

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		getSQLError(SQL_HANDLE_STMT, hstmt);
		printErrorToConsole(L"Failed to bind parameter");
	}
	else {

		std::wstring callProcedureStatement = L"CALL VERIFICARFERIADO(?, ?);";

		ret = SQLExecDirect(hstmt, (SQLWCHAR*)callProcedureStatement.c_str(), callProcedureStatement.length());

		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			getSQLError(SQL_HANDLE_STMT, hstmt);
			printErrorToConsole(L"Failed to execute stored procedure");
		}
		else {
			std::wstring consoleLine = L"VERIFICANDO FERIADO EM: " + (*dateStr) + L" : " + std::wstring(ParameterValueOut) + L"\n";
			consolePrintFunc(consoleLine.c_str());
			consolePrintFunc(L"\n\n");

			SQLCloseCursor(hstmt);
		}
	}

}

void fetchDataFromStatement(SQLHSTMT hstmt, printFuncP consolePrintFunc) {
	
	// STEP 4: FETCH RESULTS

	SQLRETURN ret;

	SQLSMALLINT numCols = 0;

	ret = SQLNumResultCols(hstmt, &numCols);

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		getSQLError(SQL_HANDLE_STMT, hstmt);
		printErrorToConsole(L"Failed to get number of columns");
	}

	if (numCols != 0) {
		SQLUSMALLINT   ColumnNumber = 0;
		SQLWCHAR columnName[100];
		SQLSMALLINT    BufferLength = 100;
		SQLSMALLINT NameLengthPtr = 0;
		SQLSMALLINT DataTypePtr = 0;
		SQLULEN ColumnSizePtr = 0;
		SQLSMALLINT DecimalDigitsPtr = 0;
		SQLSMALLINT NullablePtr = 0;

		for (SQLUSMALLINT i = 1; i <= numCols; i++) {
			ret = SQLDescribeCol(
				hstmt,
				i,
				columnName,
				BufferLength,
				&NameLengthPtr,
				&DataTypePtr,
				&ColumnSizePtr,
				&DecimalDigitsPtr,
				&NullablePtr
			);

			if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
				getSQLError(SQL_HANDLE_STMT, hstmt);
				printErrorToConsole(L"Failed to get column description");
			}
		}

		SQLWCHAR date[11];

		SQLLEN Strlen_or_IndPtr = 0;
		ret = SQLBindCol(hstmt, 2, SQL_C_WCHAR, &date, sizeof(date), &Strlen_or_IndPtr);

		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			getSQLError(SQL_HANDLE_STMT, hstmt);
			printErrorToConsole(L"Failed to bind column");
		}
		else {
			ret = SQL_SUCCESS;
			while (ret == SQL_SUCCESS) {
				ret = SQLFetch(hstmt);

				if (ret == SQL_ERROR) {
					getSQLError(SQL_HANDLE_STMT, hstmt);
					printErrorToConsole(L"Failed to fetch row");
				}

				if (ret == SQL_SUCCESS) {
					std::wstring consoleLine = L"FERIADO: " + std::wstring(date) + L"\n";
					consolePrintFunc(consoleLine.c_str());
				}
			}
			consolePrintFunc(L"\n\n");
		}
	}
}