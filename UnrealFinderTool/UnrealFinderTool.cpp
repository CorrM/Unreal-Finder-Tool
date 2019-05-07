#include "pch.h"
#include "Color.h"
#include "GnamesFinder.h"
#include <iostream>
#include "GObjectsFinder.h"
#include "InstanceLogger.h"

bool is64()
{
#if _WIN64
	return true;
#else
	return false;
#endif
}

int main()
{
	const HWND console = GetConsoleWindow();
	RECT r;
	GetWindowRect(console, &r);
	MoveWindow(console, r.left, r.top, 650, 400, TRUE);

	int tool_id, p_id, old_pid = 0;

	RESTART:
	system("cls");
	SetConsoleTitleA("Unreal Engine Finder Tool By CorrM");
	std::cout << red << "[*] " << green << "Unreal Engine Finder Tool By " << yellow << "CorrM" << std::endl << std::endl << def;

	Tools:
	std::cout << yellow << "[?] " << red << "1: " << def << "GNames Finder" << "    -  " << yellow << "Find GNamesArray in ue4 game." << std::endl << def;
	std::cout << yellow << "[?] " << red << "2: " << def << "GObject Finder" << "   -  " << yellow << "Find GObjectArray in ue4 game." << std::endl << def;
	std::cout << yellow << "[?] " << red << "3: " << def << "Instance Logger" << "  -  " << yellow << "Dump all Instance in ue4 game." << std::endl << def;
	std::cout << yellow << "[?] " << red << "4: " << def << "SDK Generator" << "    -  " << yellow << "Generate SDK for ue4 game." << std::endl << def;

	std::cout << std::endl;
	std::cout << green << "[-] " << yellow << "Input tool ID: " << dgreen;
	std::cin >> tool_id;

	if (tool_id == 0 || tool_id > 4)
	{
		std::cout << red << "[*] " << def << "Input valid tool ID." << std::endl << def;
		std::cout << def << "===================================\n" << std::endl;
		goto Tools;
	}

	// Get pID
	pID:
	std::cout << green << "[-] " << yellow << "input process ID: " << dgreen;
	std::cin >> p_id;

	if (p_id == 0 && old_pid != 0)
	{
		p_id = old_pid;
	}

	HANDLE pHandle = OpenProcess(0x0 | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, p_id);
	DWORD exitCode;

	if (p_id == 0 && old_pid == 0 || GetExitCodeProcess(pHandle, &exitCode) == FALSE && exitCode != STILL_ACTIVE)
	{
		std::cout << red << "[*] " << def << "Input valid process ID." << std::endl << def;
		goto pID;
	}
	old_pid = p_id;

	// Use Kernal ???
	char cUseKernal;
	std::cout << green << "[-] " << yellow << "Use Kernal (Y/N): " << dgreen;
	std::cin >> cUseKernal;
	const bool bUseKernal = cUseKernal == 'Y' || cUseKernal == 'y';

	std::cout << def << "===================================" << std::endl;

	// Setup Memory Stuff
	auto memManager = new Memory(pHandle, bUseKernal);
	if (!bUseKernal) memManager->GetDebugPrivileges();

	if (tool_id == 1) // GNames Finder
	{
		GnamesFinder gf(memManager);
		gf.Find();
	}
	else if (tool_id == 2) // GObjects Finder
	{
		std::cout << red << "[?] " << yellow << "First try EASY method. not work.? use HARD method and wait some time :D." << std::endl;
		char cUseEz;
		std::cout << green << "[-] " << yellow << "Use Easy Method (Y/N): " << dgreen;
		std::cin >> cUseEz;
		const bool bUseEz = cUseEz == 'Y' || cUseEz == 'y';

		GObjectsFinder taf(memManager, bUseEz);
		taf.Find();
	}
	else if (tool_id == 3) // Instance Logger
	{
		uintptr_t objObjectsAddress, gNamesAddress;
		std::cout << green << "[-] " << yellow << "Input (GObjects) Address: " << dgreen;
		std::cin >> std::hex >> objObjectsAddress;

		std::cout << green << "[-] " << yellow << "Input (GNamesArray) Address: " << dgreen;
		std::cin >> std::hex >> gNamesAddress;

		InstanceLogger il(memManager, objObjectsAddress, gNamesAddress);
		il.Start();
	}

	std::cout << def << "===================================" << std::endl;

	delete memManager;
	CloseHandle(pHandle);

	char cRestart;
	std::cout << yellow << "[?] " << yellow << "RESTART (Y/N): " << dgreen;
	std::cin >> cRestart;

	if (cRestart == 'Y' || cRestart == 'y')
		goto RESTART;

	return 0;
}
