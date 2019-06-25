#include "pch.h"
#include "Debug.h"

#include <DbgHelp.h>
#include <tchar.h>
#include <sstream>

#pragma warning(disable: 4996)
#pragma comment(lib, "dbghelp")

void Debugging::EnterDebugMode(const bool bConsole)
{
	if (bConsole)
	{
		AllocConsole();
		freopen("CONOUT$", "w", stdout);

		bConsoleEnabled = true;
	}

	isX64 = Utils::ProgramIs64();
	expHandle = AddVectoredExceptionHandler(1UL, ExceptionHandler);
}

LONG WINAPI Debugging::ExceptionHandler(EXCEPTION_POINTERS* e)
{
	//UNREFERENCED_PARAMETER(e);
	if (e->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && CreateMiniDump(e))
	{
		MessageBox(HWND_DESKTOP,
			"Program encountered an issue and was terminated. Dump information is located in Dumps folder in program directory.",
			"Error",
			MB_ICONERROR | MB_OK);
	}

	return EXCEPTION_NONCONTINUABLE_EXCEPTION;
}

BOOL Debugging::CreateMiniDump(EXCEPTION_POINTERS* e)
{
	SYSTEMTIME st;
	MINIDUMP_EXCEPTION_INFORMATION md;
	TCHAR szPath[MAX_PATH] = { 0 };

	GetLocalTime(&st);
	CreateDirectory(_T("Dumps"), nullptr);

	sprintf_s(szPath, MAX_PATH, "Dumps\\%04d%02d%02d-%02d%02d%02d-%ld-%ld.mdmp",
	        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
	        GetCurrentProcessId(), GetCurrentThreadId());

	HANDLE hDump = CreateFile(szPath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_WRITE,
							 nullptr,
	                         CREATE_ALWAYS,
							 0,
	                         nullptr);

	md.ThreadId = GetCurrentThreadId();
	md.ExceptionPointers = e;
	md.ClientPointers = TRUE;

	BOOL bSuccess = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDump, MiniDumpWithDataSegs, &md, nullptr, nullptr);
	return bSuccess;
}

void Debugging::Log(const char* format, ...)
{
	if (!bConsoleEnabled)
		return;

	va_list arg;
	va_start(arg, format);

	vprintf(format, arg);
	va_end(arg);
}

void Debugging::LogFile(const char* format, ...)
{
	if (!fp)
	{
		CreateLogFile();
		return;
	}
		

	va_list arg;
	va_start(arg, format);

	vfprintf(fp, format, arg);
	va_end(arg);
}

void Debugging::CreateLogFile()
{
	const char logfile[] = "Dumps\\Debug.log";

	if (!fp)
		fp = fopen(logfile, "w");

	return;
}

Debugging::~Debugging()
{
	if (expHandle)
		RemoveVectoredExceptionHandler(expHandle);
}
