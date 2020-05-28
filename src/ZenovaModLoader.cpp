#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <ShObjIdl.h>
#include <fstream>

#include "Utils/AppUtils.h"
#include "Utils/ProcessUtils.h"
#include "Utils/Utils.h"
#include "ModLoader.h"

#define IMPORT extern __declspec(dllimport)

IMPORT int __argc;
IMPORT char** __argv;
//IMPORT wchar_t** __wargv;

void LaunchProcess(const std::string& app, const std::string& arg) {
	// Prepare handles.
	STARTUPINFOA si;
	PROCESS_INFORMATION pi; // The function returns this
	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pi, sizeof(pi) );

	// Start the child process.
	if(!CreateProcessA(app.c_str(), const_cast<char*>(arg.c_str()), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
		printf("CreateProcess failed (%d).\n", GetLastError());
		throw std::exception("Could not create child process");
	}
	
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

//#define CONSOLE_DEBUG

// Turning this into a normal Windows program hides the GUI
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	DWORD dwProcessId = 0;
	bool debug = false;

	for(int i = 1; i < __argc; i += 2) {
		//printf("%s, %s\n", __argv[i], __argv[i+1]);
		std::string arg(__argv[i]);
		if(arg == "-p") {
			dwProcessId = atoi(__argv[i + 1]);
		}
		else if(arg == "-d") {
			debug = atoi(__argv[i + 1]);
		}
	}

//Maybe move MessageRedirection to Common and use it here?
#ifdef CONSOLE_DEBUG
	AllocConsole();
	FILE* console = nullptr;
	//Handle std::cout, std::clog, std::cerr, std::cin
	freopen_s(&console, "CONOUT$", "w", stdout);
	freopen_s(&console, "CONOUT$", "w", stderr);
	freopen_s(&console, "CONIN$", "r", stdin);

	HANDLE hConOut = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	HANDLE hConIn = CreateFileA("CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
	SetStdHandle(STD_ERROR_HANDLE, hConOut);
	SetStdHandle(STD_INPUT_HANDLE, hConIn);

	std::cout.clear();
	std::clog.clear();
	std::cerr.clear();
	std::cin.clear();
	std::wcout.clear();
	std::wclog.clear();
	std::wcerr.clear();
	std::wcin.clear();
#endif // CONSOLE_DEBUG

	if(debug) {
		std::cout << "Started with debug mode\n";
		LaunchProcess("C:\\Windows\\System32\\vsjitdebugger.exe", " -p " + std::to_string(dwProcessId));
	}

	if(dwProcessId != 0 && SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) {
		std::cout << "CoInitialize succeeded\n";
		AppUtils::AppDebugger app(AppUtils::GetMinecraftPackageId());

		if(app.GetPackageExecutionState() == PES_UNKNOWN) {
			CoUninitialize();
			return E_FAIL;
		}

		// Assume the game is suspended and inject mods
		std::wstring zenovaPath = Util::StrToWstr(getenv("ZENOVA_DATA")) + L"\\ZenovaAPI.dll";
		std::cout << "Path: " << Util::WstrToStr(zenovaPath) << "\n";
		
		ModLoader::AdjustGroupPolicy(zenovaPath.c_str());
		ModLoader::InjectDLL(dwProcessId, zenovaPath.c_str());
		
		// Resume the game
		ProcessUtils::ResumeProcess(dwProcessId);

		CoUninitialize();
	}

	std::cout << "Gracefully exit\n";

#ifdef CONSOLE_DEBUG
	fclose(console);
#endif // CONSOLE_DEBUG

	return S_OK;
}