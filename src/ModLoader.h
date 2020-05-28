#pragma once

namespace ModLoader {
	/* Sets the group policy of a given file to "ALL APPLICATION PACKAGES" */
	DWORD AdjustGroupPolicy(const wchar_t* wstrFilePath);

	/* Inject a DLL into Minecraft */
	BOOL InjectDLL(DWORD dwProcessId, const wchar_t* dllPath);
};