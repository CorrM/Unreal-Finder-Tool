#pragma once
#pragma warning(disable:4996)
class Debugging
{
private:
	bool bConsoleEnabled;
	bool IsX64;
	FILE *fp;
	PVOID ExpHandle;

public:
	void EnterDebugMode(bool bConsole=false);
	void Log(const char *format, ...);
	void LogFile(const char *format, ...);

	~Debugging();
private:
	static BOOL CreateMiniDump( EXCEPTION_POINTERS *e);
	void CreateLogFile();
	static LONG WINAPI ExceptionHandler(struct _EXCEPTION_POINTERS * e);
};