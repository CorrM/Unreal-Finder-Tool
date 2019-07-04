#pragma once

class Debugging
{
	bool bConsoleEnabled = false;
	bool isX64 = false;
	FILE* fp = nullptr;
	PVOID expHandle = nullptr;

public:
	void EnterDebugMode(bool bConsole = false);
	void Log(const char *format, ...) const;
	void LogFile(const char *format, ...);

	~Debugging();
private:
	void CreateLogFile();
	static BOOL CreateMiniDump(EXCEPTION_POINTERS* e);
	static LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* e);
};