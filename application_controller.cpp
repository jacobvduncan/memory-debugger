#include "application_controller.h"
#include "process_list.h"
#include "mainwindow.h"
application_controller::application_controller() {}

void application_controller::debug_test(){




}

std::vector<std::tuple<std::string, DWORD>> application_controller::getScannedProcesses(){
    process_list p;
    processes.clear();
    processes = p.GetRunningProcesses();
    return processes;
}

void application_controller::refresh() {
    foundAddresses.clear();
    totalScans = 0;
}

bool application_controller::isReadable(DWORD protect) {
    if (protect & PAGE_GUARD) return false;
    if (protect & PAGE_NOACCESS) return false;
    if (protect & PAGE_READONLY) return true;
    if (protect & PAGE_READWRITE) return true;
    if (protect & PAGE_WRITECOPY) return true;
    if (protect & PAGE_EXECUTE_READ) return true;
    if (protect & PAGE_EXECUTE_READWRITE) return true;
    if (protect & PAGE_EXECUTE_WRITECOPY) return true;
    return false;
}
void application_controller::setPid(DWORD pid) {
    selectedPid = pid;
    foundAddresses.clear();
    totalScans = 0;
}

ScanResult application_controller::scan(const std::string& value, const std::string& dataType) {
    HANDLE processHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, selectedPid);
    if (processHandle == NULL) {
        std::cerr << "Failed to open process with PID: " << selectedPid << std::endl;
        return { selectedPid, {} };
    }
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    LPVOID startAddress = sysInfo.lpMinimumApplicationAddress;
    LPVOID endAddress = sysInfo.lpMaximumApplicationAddress;
    MEMORY_BASIC_INFORMATION mbi;
    SIZE_T bytesRead;
    std::vector<BYTE> searchValue;
    if (dataType == "Integer") {
        int intValue = std::stoi(value);
        BYTE* p = reinterpret_cast<BYTE*>(&intValue);
        searchValue.assign(p, p + sizeof(int));
    } else if (dataType == "Float") {
        float floatValue = std::stof(value);
        BYTE* p = reinterpret_cast<BYTE*>(&floatValue);
        searchValue.assign(p, p + sizeof(float));
    } else if (dataType == "String") {
        searchValue.assign(value.begin(), value.end());
    } else {
        CloseHandle(processHandle);
        return { selectedPid, {} };
    }
    const SIZE_T chunkSize = 64 * 1024; // 64 KB
    if (foundAddresses.empty()) {
        // Initial scan
        totalScans = 1;
        for (LPBYTE addr = (LPBYTE)startAddress; addr < (LPBYTE)endAddress; addr += mbi.RegionSize) {
            if (VirtualQueryEx(processHandle, addr, &mbi, sizeof(mbi)) == sizeof(mbi)) {
                if (mbi.State == MEM_COMMIT && isReadable(mbi.Protect)) {
                    SIZE_T regionSize = mbi.RegionSize;
                    LPBYTE baseAddress = (LPBYTE)mbi.BaseAddress;

                    for (SIZE_T offset = 0; offset < regionSize; offset += chunkSize) {
                        SIZE_T bytesToRead = std::min(chunkSize, regionSize - offset);
                        std::vector<BYTE> buffer(bytesToRead);

                        if (ReadProcessMemory(processHandle, baseAddress + offset, buffer.data(), bytesToRead, &bytesRead)) {
                            for (SIZE_T i = 0; i <= bytesRead - searchValue.size(); ++i) {
                                if (memcmp(&buffer[i], searchValue.data(), searchValue.size()) == 0) {
                                    foundAddresses.push_back((uintptr_t)(baseAddress + offset + i));
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        // Refined scan
        totalScans++;
        std::vector<uintptr_t> newAddresses;
        for (uintptr_t addr : foundAddresses) {
            std::vector<BYTE> buffer(searchValue.size());
            if (ReadProcessMemory(processHandle, (LPCVOID)addr, buffer.data(), buffer.size(), &bytesRead)) {
                if (memcmp(buffer.data(), searchValue.data(), searchValue.size()) == 0) {
                    newAddresses.push_back(addr);
                }
            }
        }
        foundAddresses = newAddresses; // Update found addresses
    }
    CloseHandle(processHandle);
    return { selectedPid, foundAddresses };
}



bool application_controller::writeToMemory(DWORD pid, const std::string& dataType, const std::string& value, uintptr_t address) {
    HANDLE processHandle = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid);
    if (processHandle == NULL) {
        std::cerr << "Failed to open process with PID: " << pid << std::endl;
        return false;
    }

    std::vector<BYTE> newValueBytes;

    // Convert the value based on the data type
    if (dataType == "Integer") {
        try {
            int newValue = std::stoi(value);
            BYTE* p = reinterpret_cast<BYTE*>(&newValue);
            newValueBytes.assign(p, p + sizeof(int));
        } catch (const std::exception& e) {
            std::cerr << "Error converting value to Integer: " << e.what() << std::endl;
            CloseHandle(processHandle);
            return false;
        }
    } else if (dataType == "Float") {
        try {
            float newValue = std::stof(value);
            BYTE* p = reinterpret_cast<BYTE*>(&newValue);
            newValueBytes.assign(p, p + sizeof(float));
        } catch (const std::exception& e) {
            std::cerr << "Error converting value to Float: " << e.what() << std::endl;
            CloseHandle(processHandle);
            return false;
        }
    } else if (dataType == "String") {
        newValueBytes.assign(value.begin(), value.end());
        // Add null terminator for strings, if needed
        newValueBytes.push_back('\0');
    } else {
        std::cerr << "Unsupported data type: " << dataType << std::endl;
        CloseHandle(processHandle);
        return false;
    }

    // Write the new value to the specified memory address
    SIZE_T bytesWritten;
    BOOL result = WriteProcessMemory(
        processHandle,
        reinterpret_cast<LPVOID>(address),
        newValueBytes.data(),
        newValueBytes.size(),
        &bytesWritten
        );

    CloseHandle(processHandle);

    if (result && bytesWritten == newValueBytes.size()) {
        std::cout << "Successfully wrote value to memory address 0x" << std::hex << address << std::endl;
        return true;
    } else {
        std::cerr << "Failed to write to memory address 0x" << std::hex << address << std::endl;
        return false;
    }
}


std::string application_controller::readValueFromAddress(DWORD pid, const std::string& dataType, uintptr_t address) {
    // Open the process with read permissions
    HANDLE processHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!processHandle) {
        return "Error: Failed to open process with PID " + std::to_string(pid);
    }

    SIZE_T bytesRead;
    char buffer[256] = {0}; // Allocate buffer for reading data

    // Determine the type and read the corresponding value
    if (dataType == "Integer") {
        int value = 0;
        if (ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), &value, sizeof(value), &bytesRead)) {
            CloseHandle(processHandle);
            return std::to_string(value);
        }
    } else if (dataType == "Float") {
        float value = 0.0f;
        if (ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), &value, sizeof(value), &bytesRead)) {
            CloseHandle(processHandle);
            return std::to_string(value);
        }
    } else if (dataType == "Double") {
        double value = 0.0;
        if (ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), &value, sizeof(value), &bytesRead)) {
            CloseHandle(processHandle);
            return std::to_string(value);
        }
    } else if (dataType == "String") {
        // Assuming a null-terminated string with a maximum length of 256
        if (ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), buffer, sizeof(buffer) - 1, &bytesRead)) {
            CloseHandle(processHandle);
            return std::string(buffer); // Convert to std::string
        }
    } else if (dataType == "Char") {
        char value = 0;
        if (ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), &value, sizeof(value), &bytesRead)) {
            CloseHandle(processHandle);
            return std::string(1, value); // Single character as a string
        }
    } else {
        CloseHandle(processHandle);
        return "Error: Unsupported data type.";
    }

    // If ReadProcessMemory fails, return an error message
    CloseHandle(processHandle);
    return "ERROR: FAILED AT 0x" + std::to_string(address);
}
