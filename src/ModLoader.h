#pragma once

#include <string>

namespace ModLoader {
	/* Sets the group policy of a given file to "ALL APPLICATION PACKAGES" */
	DWORD AdjustGroupPolicy(std::wstring wstrFilePath);

	/* Inject a DLL into Minecraft */
	BOOL InjectDLL(DWORD dwProcessId, std::wstring dllPath);

	/* Loads mods from AppData and injects them */
	HRESULT InjectMods(DWORD dwProcessId);
};