// UnrealFinderTool.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "Color.h"
#include "GnamesFinder.h"
#include "PatternScan.h"
#include <iostream>
#include "TArrayFinder.h"

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
	std::cout << yellow << "[?] " << red << "1: " << def << "GNames Finder" << "  -  " << yellow << "Find GNames in ue4 game." << std::endl << def;
	std::cout << yellow << "[?] " << red << "2: " << def << "TArray Finder" << "  -  " << yellow << "Find TArrays in ue4 game." << std::endl << def;

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

	// Use Kernal ???
	char cUseKernal;
	std::cout << green << "[-] " << yellow << "Use Kernal (Y/N): " << dgreen;
	std::cin >> cUseKernal;
	const bool bUseKernal = cUseKernal == 'Y' || cUseKernal == 'y';

	std::cout << def << "===================================" << std::endl;

	// Setup Memory Stuff
	const auto memManager = new Memory(p_id, bUseKernal);
	if (!bUseKernal)
		memManager->GetDebugPrivileges();

	if (tool_id == 1) // GNames Finder
	{
		GnamesFinder gf(memManager);
		gf.Find();
	}
	else if (tool_id == 2) // TArray Finder
	{
		TArrayFinder taf(memManager);
		taf.Find();
	}

	std::cout << def << "===================================" << std::endl;

	delete memManager;
	char cRestart;
	std::cout << yellow << "[?] " << yellow << "RESTART (Y/N): " << dgreen;
	std::cin >> cRestart;

	if (cRestart == 'Y' || cRestart == 'y')
		goto RESTART;

	return 0;
}
