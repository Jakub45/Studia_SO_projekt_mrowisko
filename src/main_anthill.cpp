#include <windows.h>
#include <iostream>
#include <string>

enum class status {
    ANT_BORN = 0,
    ANT_DEAD = 1,
    FOOD_FOUND = 2,
};


int main()
{
    HANDLE readPipe, writePipe;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    if (!CreatePipe(&readPipe, &writePipe, &sa, 0))
        return 1;
    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);
    PROCESS_INFORMATION processes[10];
    int processCount = 0;



    for (int i = 1; i <= 10; i++)
    {
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        ZeroMemory(&pi, sizeof(pi));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = writePipe;
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

        wchar_t command[50];
        swprintf_s(command, 50, L"ant.exe %d", i);

        if (CreateProcessW(NULL, command, NULL, NULL,
            TRUE, 0, NULL, NULL, &si, &pi))
        {
            processes[processCount] = pi;
            processCount++;

            CloseHandle(pi.hThread);
        }
    }

    CloseHandle(writePipe);

    char buffer[256];
    DWORD bytesRead;
    std::string pending;

    while (ReadFile(readPipe, buffer, sizeof(buffer) - 1,
        &bytesRead, NULL) && bytesRead > 0)
    {
        buffer[bytesRead] = '\0';
        pending += buffer;

        size_t pos;

        while ((pos = pending.find('\n')) != std::string::npos)
        {
            std::string message = pending.substr(0, pos);
            pending.erase(0, pos + 1);

            if (message.find("FOOD_FOUND ") == 0)
            {
                std::cout << message.substr(11)
                    << " znalazl jedzenie!" << std::endl;
            }
            else if (message.find("ANT_DEAD ") == 0)
            {
                std::cout << message.substr(9)
                    << " umarl!" << std::endl;
            }
            else
            {
                std::cout << message << std::endl;
            }
        }
    }

    CloseHandle(readPipe);

    for (int i = 0; i < processCount; i++)
    {
        WaitForSingleObject(processes[i].hProcess, INFINITE);
        CloseHandle(processes[i].hProcess);
    }

    std::cout << "Wszystkie mrowki zakonczyly dzialanie.\n";

    std::cin.get();

    return 0;
}
