#ifndef APPLICATION_CONTROLLER_H
#define APPLICATION_CONTROLLER_H
#include <vector>
#include <string>
#include <tuple>
#include <iostream>
#include <windows.h>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <thread>
#include <mutex>
#include <psapi.h>
class process_list;
class MainWindow;

struct ScanResult {
    DWORD pid;
    std::vector<uintptr_t> addresses;
};

class application_controller
{
public:
    application_controller();

    void debug_test();

    inline void setWindow(MainWindow* w){window = w;};

    std::vector<std::tuple<std::string, DWORD>> getScannedProcesses();

    void setPid(DWORD pid);

    ScanResult scan(const std::string& value, const std::string& dataType);

    void refresh();

    bool writeToMemory(DWORD pid, const std::string& dataType, const std::string& value, uintptr_t address);

    inline int getTotalScans(){return totalScans;};

    std::string readValueFromAddress(DWORD pid, const std::string& dataType, uintptr_t address);

private:
    MainWindow* window;

    std::vector<std::tuple<std::string, DWORD>> processes;

    bool isReadable(DWORD protect);

    std::vector<uintptr_t> foundAddresses;
    DWORD selectedPid;

    int totalScans;
};

#endif // APPLICATION_CONTROLLER_H
