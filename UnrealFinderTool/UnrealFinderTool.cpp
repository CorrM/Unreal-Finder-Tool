// UnrealFinderTool.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "Color.h"
#include "GnamesFinder.h"
#include "PatternScan.h"
#include <iostream>
#include "TArrayFinder.h"

Memory* memManager;

int main()
{
	const HWND console = GetConsoleWindow();
	RECT r;
	GetWindowRect(console, &r);
	MoveWindow(console, r.left, r.top, 650, 400, TRUE);

	int tool_id, p_id;

	Color green(LightGreen, Black);
	Color def(White, Black);
	Color yellow(LightYellow, Black);
	Color purple(LightPurple, Black);
	Color red(LightRed, Black);
	Color dgreen(Green, Black);

	RESTART:
	system("cls");
	SetConsoleTitleA("Unreal Engine Finder Tool By CorrM");
	std::cout << red << "[*] " << green << "Unreal Engine Finder Tool By " << yellow << "CorrM" << std::endl << std::endl << def;

	Tools:
	std::cout << yellow << "[?] " << red << "1: " << def << "GNames Finder" << "   -  " << yellow << "Find GNamesArray in ue4 game." << std::endl << def;
	std::cout << yellow << "[?] " << red << "2: " << def << "GObject Finder" << "  -  " << yellow << "Find GObjectArray in ue4 game." << std::endl << def;

	std::cout << std::endl;
	std::cout << green << "[-] " << yellow << "Input tool ID: " << dgreen;
	std::cin >> tool_id;

	if (tool_id == 0 || tool_id > 2)
	{
		std::cout << red << "[*] " << def << "Input valid tool ID." << std::endl << def;
		std::cout << def << "===================================\n" << std::endl;
		goto Tools;
	}

	// Get pID
	pID:
	std::cout << green << "[-] " << yellow << "input process ID: " << dgreen;
	std::cin >> p_id;

	if (p_id == 0)
	{
		std::cout << red << "[*] " << def << "Input valid process ID." << std::endl << def;
		goto pID;
	}

	const HANDLE pHandle = OpenProcess(0x0 | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, p_id);
	DWORD exitCode;
	if (GetExitCodeProcess(pHandle, &exitCode) == FALSE && exitCode != STILL_ACTIVE) goto pID;

	// Use Kernal ???
	char cUseKernal;
	std::cout << green << "[-] " << yellow << "Use Kernal (Y/N): " << dgreen;
	std::cin >> cUseKernal;
	const bool bUseKernal = cUseKernal == 'Y' || cUseKernal == 'y';

	std::cout << def << "===================================" << std::endl;

	// Setup Memory Stuff
	memManager = memManager == nullptr ? new Memory(pHandle, bUseKernal) : memManager;
	memManager->UpdateHandle(pHandle);
	if (!bUseKernal) memManager->GetDebugPrivileges();

	if (tool_id == 1) // GNames Finder
	{
		GnamesFinder gf(memManager);
		gf.Find();
	}
	else if (tool_id == 2) // GObjects Finder
	{
		TArrayFinder taf(memManager);
		taf.Find();
	}

	std::cout << def << "===================================" << std::endl;

	CloseHandle(pHandle);

	char cRestart;
	std::cout << yellow << "[?] " << yellow << "RESTART (Y/N): " << dgreen;
	std::cin >> cRestart;

	if (cRestart == 'Y' || cRestart == 'y')
		goto RESTART;

	delete memManager;
	CloseHandle(pHandle);
	return 0;
}
