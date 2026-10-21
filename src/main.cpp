#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

int main() {
    HANDLE readPipe, writePipe;

    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    if (!CreatePipe(&readPipe, &writePipe, &sa, 0))
        return 1;

    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

    std::vector<PROCESS_INFORMATION> processes;

    for (int i = 1; i <= 10; i++) {
        STARTUPINFOW si = {};
        PROCESS_INFORMATION pi = {};
        si.cb = sizeof(si);

        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = writePipe;
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

        wchar_t command[50];
        swprintf_s(command, L"ant.exe %d", i);

        if (CreateProcessW(nullptr, command, nullptr, nullptr,
            TRUE, 0, nullptr, nullptr, &si, &pi)) {
            processes.push_back(pi);
            CloseHandle(pi.hThread);
        }
    }

    CloseHandle(writePipe);

    char buffer[256];
    DWORD bytesRead;
    std::string pending;

    while (ReadFile(readPipe, buffer, sizeof(buffer) - 1,
        &bytesRead, nullptr) && bytesRead > 0) {

        buffer[bytesRead] = '\0';
        pending += buffer;

        size_t pos;
        while ((pos = pending.find('\n')) != std::string::npos) {
            std::string message = pending.substr(0, pos);
            pending.erase(0, pos + 1);
            if (message.rfind("FOOD_FOUND ", 0) == 0) {
                std::cout << message.substr(11)
                    << " znalazl jedzenie!" << std::endl;
            }
            else if (message.rfind("ANT_DEAD ", 0) == 0) {
                std::cout << message.substr(9)
                    << " umarl!" << std::endl;
            }
            else {
                std::cout << message << std::endl;
            }
        }
    }

    CloseHandle(readPipe);

    for (const auto& pi : processes) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
    }

    std::cout << "Wszystkie mrowki zakonczyly dzialanie.\n";
    std::cin.get();
    return 0;
}