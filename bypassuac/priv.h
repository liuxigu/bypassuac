#pragma once

#ifndef PRIV_AAAAAAAAAAAAA
#define PRIV_AAAAAAAAAAAAA


#include "windows.h"


VOID fn_start_service();
VOID fn_delete_service();
INT fn_create_service();
INT fn_create_process(LPWSTR lpFileName, LPCWSTR lpParam);
HANDLE fn_backup_token();
VOID fn_adjust_token_privilege(HANDLE& hNewThreadToken);
INT fn_impersonate_via_namepipe();


#endif