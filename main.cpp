#include <QCoreApplication>
#include <Windows.h>
#include <TlHelp32.h>
#include <QDebug>

const QString processName = "Bot.exe";

DWORD GetProcessIdByName( const QString &name )
{
	DWORD id;
	
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	
	if (Process32First(snapshot, &entry) == TRUE)
	{
		while (Process32Next(snapshot, &entry) == TRUE)
		{
			auto i( 0 );
			QString procName;
			while( entry.szExeFile[i] != '\0' )
			{
				procName += entry.szExeFile[i];
				++i;
			}
			
			if ( procName == name )
			{
				id = entry.th32ProcessID;
				break;
			}
		}
	}
	
	CloseHandle(snapshot);
	
	return id;
}

BOOL InjectDll( const WCHAR *modulePath )
{
	auto pid = GetProcessIdByName( processName );
	HANDLE process = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pid );
	
	int cch = 1 + lstrlenW( modulePath );
	int cb  = cch * sizeof(WCHAR);
	
	LPVOID memory = VirtualAllocEx( process, 0, cb,
	                                MEM_COMMIT, PAGE_EXECUTE_READWRITE );
	
	if(!memory)
		return FALSE;
	
	WriteProcessMemory( process, memory, modulePath, cb, 0 );
	
	PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)
	         GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "LoadLibraryW");
	
	if ( pfnThreadRtn == NULL )
		qDebug() << "pfnThreadRtn = NULL";
	
	HANDLE hThread = CreateRemoteThread( process, 0, 0, pfnThreadRtn, memory, 0, 0 );
	
	if( !hThread )
	{
		CloseHandle( process );
		return FALSE;
	}
	
	WaitForSingleObject( hThread, INFINITE );
	
	VirtualFreeEx( process, memory, cb, MEM_RELEASE );
	CloseHandle( hThread );
	CloseHandle( process );
	
	return TRUE;
}

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);
	
	auto result = InjectDll( L"e:/TestBot/build-testLib-Desktop_Qt_5_2_1_MSVC2012_64bit-Debug/debug/testLib.dll" );
	
	qDebug() << "Open result : " << result;
	
	return a.exec();
}

