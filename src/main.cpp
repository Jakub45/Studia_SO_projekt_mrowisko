#include <stdio.h>
#include <windows.h>
#include <iostream>

int main() {

	STARTUPINFOW si = {};
	PROCESS_INFORMATION pi = {};
	si.cb = sizeof(si);

	wchar_t command[] = L"ant.exe 10";

	BOOL success = CreateProcessW(
		nullptr,
		command,
		nullptr, nullptr,
		FALSE, 0,
		nullptr, nullptr,
		&si, &pi
	);

	if (success) {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}


	std::cin.get();
	return 0;
}