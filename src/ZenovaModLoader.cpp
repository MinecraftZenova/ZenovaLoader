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
		throw std::runtime_error("Could not create child process");
	}
	
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

//#define CONSOLE_DEBUG

#include <conio.h>

class MessageManager {
	FILE* console = nullptr;

public:

	MessageManager() {
		AllocConsole();
		
		if(!console) {
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
		}
	}

	~MessageManager() {
		FreeConsole();
		if(console) {
			fclose(console);
		}
	}
};


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

#ifdef CONSOLE_DEBUG
	MessageManager console;
#endif

	if(debug) {
		std::cout << "Started with debug mode\n";
		try {
			LaunchProcess("C:\\Windows\\System32\\vsjitdebugger.exe", " -p " + std::to_string(dwProcessId));
		}
		catch(const std::exception& e) {
			std::cout << "Exception: " << e.what() << "\n";
		}
	}

	if(dwProcessId != 0 && SUCCEEDED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) {
		std::cout << "CoInitialize succeeded\n";

		std::wstring minecraftPID = AppUtils::GetMinecraftPackageId();
		std::cout << Util::WstrToStr(minecraftPID) << "\n";

		AppUtils::AppDebugger app(minecraftPID);

		if(app.GetPackageExecutionState() == PES_UNKNOWN) {
			std::cout << "App PES is unknown\n";
			CoUninitialize();
			return E_FAIL;
		}

		std::wstring zenovaPath = Util::StrToWstr(getenv("ZENOVA_DATA")) + L"\\ZenovaAPI.dll";
		std::cout << "Path: " << Util::WstrToStr(zenovaPath) << "\n";

		//Assume the game is suspended and inject ZenovaAPI
		//ModLoader::AdjustGroupPolicy(zenovaPath.c_str()); //not really needed seeing how ZenovaLauncher adjusts the group policy
		ModLoader::InjectDLL(dwProcessId, zenovaPath.c_str());
		
		// Resume the game
		ProcessUtils::ResumeProcess(dwProcessId);

		CoUninitialize();
	}

	std::cout << "Gracefully exit" << std::flush;

	return S_OK;
}