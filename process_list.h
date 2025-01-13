#ifndef PROCESS_LIST_H
#define PROCESS_LIST_H
#include <windows.h>
#include <psapi.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <tuple>
#include "application_controller.h"
class process_list
{
public:
    process_list();
    bool EnableDebugPrivilege();
    static std::vector<std::tuple<std::string, DWORD>> GetRunningProcesses();
private:
    static std::string ConvertToString(const TCHAR* tcharString);
    application_controller* controller;
};
#endif
