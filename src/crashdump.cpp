#include "crashdump.h"
#include "main.h"
#include "serverconfig.h"

// Ends all game matches if possible
void ForceEndServer()
{

}

#ifdef WIN32
typedef BOOL(WINAPI* MINIDUMPWRITEDUMP)
(
	HANDLE hProcess,
	DWORD dwPid,
	HANDLE hFile,
	MINIDUMP_TYPE DumpType,
	CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
	);

BOOL WriteMiniDump(EXCEPTION_POINTERS* pep, MINIDUMP_TYPE dumpType, const char* mdmpName)
{
	HMODULE hDbgHelpDll = ::LoadLibrary("DbgHelp.dll");
	if (!hDbgHelpDll)
		return false;

	BOOL bResult = false;
	MINIDUMPWRITEDUMP pfnMiniDumpWrite = (MINIDUMPWRITEDUMP) ::GetProcAddress(hDbgHelpDll, "MiniDumpWriteDump");
	if (pfnMiniDumpWrite)
	{
		char rgchModuleName[MAX_PATH];
		::GetModuleFileName(NULL, rgchModuleName, sizeof(rgchModuleName) / sizeof(char));
		char* pch = strrchr(rgchModuleName, '.');
		if (pch)
		{
			*pch = 0;
		}
		pch = strrchr(rgchModuleName, '\\');
		if (pch)
		{
			// move past the last slash
			pch++;
		}
		else
		{
			strcpy(pch, "unknown");
		}

		char name[MAX_PATH];
		snprintf(name, sizeof(name) / sizeof(char), "%s_%s.mdmp", pch, mdmpName);

		HANDLE hFile = ::CreateFile(name, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (!hFile || hFile == INVALID_HANDLE_VALUE)
			return false;

		MINIDUMP_EXCEPTION_INFORMATION exInfo;
		exInfo.ThreadId = GetCurrentThreadId();
		exInfo.ExceptionPointers = pep;
		exInfo.ClientPointers = FALSE;

		bResult = (*pfnMiniDumpWrite)(::GetCurrentProcess(), GetCurrentProcessId(), hFile, dumpType, (pep != NULL) ? &exInfo : NULL, NULL, NULL);
		CloseHandle(hFile);
	}

	::FreeLibrary(hDbgHelpDll);

	return bResult;
}

typedef HINSTANCE(WINAPI* SHELLEXECUTE)
(
	HWND hwnd,
	LPCSTR lpOperation,
	LPCSTR lpFile,
	LPCSTR lpParameters,
	LPCSTR lpDirectory,
	INT nShowCmd
);

LONG __stdcall ExceptionFilter(EXCEPTION_POINTERS* pep)
{
	printf("ExceptionFilter called\n");

	time_t currTime = time(NULL);
	struct tm* pTime = localtime(&currTime);

	char name[MAX_PATH];
	snprintf(name, sizeof(name) / sizeof(char),
		"%d-%.2d-%.2d %.2d-%.2d-%.2d",
		pTime->tm_year + 1900,	/* Year less 2000 */
		pTime->tm_mon + 1,		/* month (0 - 11 : 0 = January) */
		pTime->tm_mday,			/* day of month (1 - 31) */
		pTime->tm_hour,			/* hour (0 - 23) */
		pTime->tm_min,		    /* minutes (0 - 59) */
		pTime->tm_sec		    /* seconds (0 - 59) */
	);

#ifndef PROTECTION
	bool isMdmpGenerated = true;
	if (!WriteMiniDump(pep, MiniDumpNormal, name))
	{
		snprintf(name, sizeof(name) / sizeof(char),
			"%d-%.2d-%.2d %.2d-%.2d-%.2d_FULL",
			pTime->tm_year + 1900,	/* Year less 2000 */
			pTime->tm_mon + 1,		/* month (0 - 11 : 0 = January) */
			pTime->tm_mday,			/* day of month (1 - 31) */
			pTime->tm_hour,			/* hour (0 - 23) */
			pTime->tm_min,		    /* minutes (0 - 59) */
			pTime->tm_sec		    /* seconds (0 - 59) */
		);

		if (!WriteMiniDump(pep, MiniDumpWithFullMemory, name))
		{
			Logger().Info(OBFUSCATE("WriteMiniDump(MiniDumpWithFullMemory) failed, GetLastError: %d\n"), GetLastError());

			isMdmpGenerated = false;
		}

		// it is not safe to do that
		Logger().Info(OBFUSCATE("WriteMiniDump(MiniDumpNormal) failed, GetLastError: %d\n"), GetLastError());
	}
#endif

	//if (g_pServerInstance)
		//Logger().Info("Last packet function: %s()\n", g_pServerInstance->GetLastPacket());

	Logger().Info("Trying to shutdown the server\n");

	ForceEndServer();

	// make zip (crashing)
	//auto hZip = CreateZip(va("Server_%s.zip", name), 0, ZIP_FILENAME);
	//if (hZip != 0)
	//{
	//	ZipAdd(hZip, "Server.log", "Server.log", 0, ZIP_FILENAME);
	//	ZipAdd(hZip, "Users.log", "Users.log", 0, ZIP_FILENAME);
	//	ZipAdd(hZip, "Internal.log", "Internal.log", 0, ZIP_FILENAME);
	//	if (!name[0] == '\0' && isMdmpGenerated)
	//	{
	//		ZipAdd(hZip, name, name, 0, ZIP_FILENAME);
	//	}

	//	CloseZip(hZip);
	//}
	//else
	//{
	//	MoveFile(OBFUSCATE("Server.log"), va(OBFUSCATE("Server_CRASH_%s.log"), name));
	//	MoveFile(OBFUSCATE("Users.log"), va(OBFUSCATE("Users_CRASH_%s.log"), name));
	//	MoveFile(OBFUSCATE("Internal.log"), va(OBFUSCATE("Internal_CRASH_%s.log"), name));
	//}

	// TODO: проверить, что если сервер будет крашить в цикле
	if (g_pServerConfig && g_pServerConfig->restartOnCrash)
	{
		// open new instance
		TCHAR szPath[_MAX_PATH];
		GetModuleFileName(NULL, szPath, _MAX_PATH);
		//ShellExecute(NULL, "open", szPath, NULL, NULL, 1);

		// fuck defender
		HMODULE hShell32 = ::LoadLibrary("Shell32.dll");
		if (hShell32)
		{
			SHELLEXECUTE pfnShellExecute = (SHELLEXECUTE) ::GetProcAddress(hShell32, "ShellExecuteA");
			pfnShellExecute(NULL, "open", szPath, NULL, NULL, 1);
		}
	}

	return EXCEPTION_EXECUTE_HANDLER;
}
#endif