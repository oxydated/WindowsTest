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
SQLWCHAR DriverDescription[200];


void fetchDataFromStatement(SQLHSTMT hstmt, printFuncP consolePrintFunc);

void executeQuery(SQLHSTMT hstmt, printFuncP consolePrintFunc);

void executeStoredProcedure(SQLHSTMT hstmt, printFuncP consolePrintFunc, std::wstring* dateStr);

SQLHENV getSQLEnvironmentHandle() {

	if (henv == NULL) {
		SQLRETURN ret;

		ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			std::runtime_error("Failed to alloc environment handle\n");
		}

		ret = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);

		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			std::runtime_error("Failed to set environment attribute\n");
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

	switch (message) {

	case WM_INITDIALOG:
		hComb = GetDlgItem(hDlg, IDC_DRIVERCOMBO);
		ret = SQLDrivers(hEnv, SQL_FETCH_FIRST, DriverDescription, 200, &DescriptionLegth, NULL, 0, &AttributesLength);
		ComboBox_AddString(hComb, DriverDescription);
		while (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
			ret = SQLDrivers(hEnv, SQL_FETCH_NEXT, DriverDescription, 200, &DescriptionLegth, NULL, 0, &AttributesLength);
			ComboBox_AddString(hComb, DriverDescription);
		}
		
		break;

	//case WM_NOTIFY:


	case WM_COMMAND:
		//if (wParam == IDOK || wParam == IDCANCEL) {
		//	if (wParam == IDOK) {
		//		connectAndFetchFromDB(hDlg, printToConsole, &dateString);
		//	}
		//	EndDialog(hDlg, TRUE);
		//}
		return (INT_PTR)TRUE;
		break;

	}
	return (INT_PTR)FALSE;
}

void chooseSQLDriver(HWND hWnd)
{
	SQLHENV hEnv = getSQLEnvironmentHandle();

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

	chooseSQLDriver(hWnd);

	ret = SQLAllocHandle(SQL_HANDLE_DBC, getSQLEnvironmentHandle(), &hcon);

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		std::runtime_error("Failed to alloc connection handle\n");
	}

	std::wstring connectionString = L"";

	SQLWCHAR outConnectionString[OUTCONNECTIONSTRINGMINLEN];

#define __USE_CONNECTION_STRING__

#ifdef __USE_CONNECTION_STRING__
	connectionString += L"DRIVER={IBM DB2 ODBC DRIVER - IBMDBCL1};";
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

		SQLWCHAR SQLState[6];
		SQLSMALLINT RecNumber = 1;
		SQLINTEGER NativeErrorPtr = 0;
		SQLWCHAR MessageText[400];
		SQLSMALLINT TextLengthPtr = 0;

		SQLRETURN checkRet = SQL_SUCCESS;

		while (checkRet == SQL_SUCCESS) {
			checkRet = SQLGetDiagRec(
				SQL_HANDLE_DBC,
				hcon,
				RecNumber++,
				SQLState,
				&NativeErrorPtr,
				MessageText,
				400,
				&TextLengthPtr
			);
		}

		if (ret != SQL_SUCCESS_WITH_INFO) {
			std::runtime_error("Failed to connect\n");
		}
	}

	// STEP 2: INITIALIZE

	SQLHSTMT hstmt;

	ret = SQLAllocHandle(SQL_HANDLE_STMT, hcon, &hstmt);

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		std::runtime_error("Failed to alloc statement handle\n");
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
		std::runtime_error("Failed to commit transaction\n");
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
		std::runtime_error("Failed to prepare statement\n");
	}
	else {
		ret = SQLExecute(hstmt);

		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {

			if (ret != SQL_NO_DATA) {
				std::runtime_error("Failed to prepare statement\n");
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
		std::runtime_error("Failed to bind parameter\n");
	}	

	SQLWCHAR ParameterValueOut[255];
	ParamNum = 2;
	InputOutputType = SQL_PARAM_OUTPUT;
	ValueType = SQL_C_WCHAR;
	ParameterType = SQL_VARCHAR;
	DecimalDigits = 0;

	ret = SQLBindParameter(hstmt, ParamNum, InputOutputType, ValueType, ParameterType, 255, 0, (SQLPOINTER)ParameterValueOut, 255, NULL);

	if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
		std::runtime_error("Failed to bind parameter\n");
	}
	else {

		std::wstring callProcedureStatement = L"CALL VERIFICARFERIADO(?, ?);";

		ret = SQLExecDirect(hstmt, (SQLWCHAR*)callProcedureStatement.c_str(), callProcedureStatement.length());

		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {

			SQLWCHAR SQLState[6];
			SQLSMALLINT RecNumber = 1;
			SQLINTEGER NativeErrorPtr = 0;
			SQLWCHAR MessageText[400];
			SQLSMALLINT TextLengthPtr = 0;

			SQLRETURN checkRet = SQL_SUCCESS;

			while (checkRet == SQL_SUCCESS) {
				checkRet = SQLGetDiagRec(
					SQL_HANDLE_STMT,
					hstmt,
					RecNumber++,
					SQLState,
					&NativeErrorPtr,
					MessageText,
					400,
					&TextLengthPtr
				);
			}
			std::runtime_error("Failed to execute stored procedure\n");
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
		std::runtime_error("Failed to get number of columns\n");
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
				std::runtime_error("Failed to get column description\n");
			}
		}

		SQLWCHAR date[11];

		SQLLEN Strlen_or_IndPtr = 0;
		ret = SQLBindCol(hstmt, 2, SQL_C_WCHAR, &date, sizeof(date), &Strlen_or_IndPtr);

		if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
			std::runtime_error("Failed to bind column\n");
		}
		else {
			ret = SQL_SUCCESS;
			while (ret == SQL_SUCCESS) {
				ret = SQLFetch(hstmt);

				if (ret != SQL_ERROR) {
					std::runtime_error("Failed to fetch row\n");
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