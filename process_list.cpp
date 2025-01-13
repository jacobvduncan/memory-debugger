#include "process_list.h"
#pragma comment(lib, "psapi.lib")
process_list::process_list() {
}
bool process_list::EnableDebugPrivilege() {
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return false;
    }
    tkp.PrivilegeCount = 1;
    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tkp.Privileges[0].Luid)) {
        CloseHandle(hToken);
        return false;
    }
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    if (GetLastError() != ERROR_SUCCESS) {
        CloseHandle(hToken);
        return false;
    }
    CloseHandle(hToken);
    return true;
}
std::string process_list::ConvertToString(const TCHAR* tcharString) {
#ifdef UNICODE
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, tcharString, -1, NULL, 0, NULL, NULL);
    if (sizeNeeded == 0) {
        return std::string();
    }
    std::string result(sizeNeeded - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, tcharString, -1, &result[0], sizeNeeded, NULL, NULL);
    return result;
#else
    return std::string(tcharString);
#endif
}
std::vector<std::tuple<std::string, DWORD>> process_list::GetRunningProcesses() {
    std::vector<std::tuple<std::string, DWORD>> processes;
    DWORD processIDs[1024], bytesReturned;
    unsigned int processCount;
    if (!EnumProcesses(processIDs, sizeof(processIDs), &bytesReturned)) {
        return processes;
    }
    processCount = bytesReturned / sizeof(DWORD);
    for (unsigned int i = 0; i < processCount; i++) {
        if (processIDs[i] != 0) {
            DWORD processID = processIDs[i];
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processID);
            if (hProcess) {
                TCHAR processName[MAX_PATH] = TEXT("<unknown>");
                DWORD size = MAX_PATH;
                if (GetProcessImageFileName(hProcess, processName, size) > 0) {
                    std::string procName = ConvertToString(processName);
                    size_t pos = procName.find_last_of("\\/");
                    if (pos != std::string::npos) {
                        procName = procName.substr(pos + 1);
                    }
                    if (procName != "<unknown>") {
                        processes.emplace_back(procName, processID);
                    }
                }
                CloseHandle(hProcess);
            }
        }
    }
    return processes;
}
