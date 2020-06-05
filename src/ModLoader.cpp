#include <Windows.h>
#include <ShlObj.h>
#include <AclAPI.h>
#include <Shlwapi.h>
#include <Sddl.h>
#include <stdio.h>

#include "ModLoader.h"

template<typename T>
class unique_hlocal_ptr {
	T ptr = NULL;

public:
	~unique_hlocal_ptr() {
		if(ptr != NULL)
			LocalFree((HLOCAL)ptr);
	}

	T& get() {
		return ptr;
	}
};

DWORD ModLoader::AdjustGroupPolicy(const wchar_t* wstrFilePath) {
	PACL pOldDACL;
	unique_hlocal_ptr<PACL> pNewDACL;
	unique_hlocal_ptr<PSECURITY_DESCRIPTOR> pSD;
	PSID pSID;
	EXPLICIT_ACCESS_W eaAccess;
	SECURITY_INFORMATION siInfo = DACL_SECURITY_INFORMATION;
	DWORD dwResult = ERROR_SUCCESS;

	// Get a pointer to the existing DACL (Conditionaly).
	dwResult = GetNamedSecurityInfoW(wstrFilePath, SE_FILE_OBJECT, siInfo, NULL, NULL, &pOldDACL, NULL, &pSD.get());
	if(dwResult != ERROR_SUCCESS) {
		printf("GetNamedSecurityInfo Error %u\n", dwResult);
		return dwResult;
	}

	ConvertStringSidToSidW(L"S-1-15-2-1", &pSID);
	if(pSID == NULL) {
		DWORD error = GetLastError();
		printf("ConvertStringSidToSidW Error %u\n", error);
		return error;
	}

	ZeroMemory(&eaAccess, sizeof(EXPLICIT_ACCESS_W));
	eaAccess.grfAccessPermissions = GENERIC_READ | GENERIC_EXECUTE;
	eaAccess.grfAccessMode = SET_ACCESS;
	eaAccess.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	eaAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	eaAccess.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	eaAccess.Trustee.ptstrName = (LPWSTR)pSID;

	// Create a new ACL that merges the new ACE into the existing DACL.
	dwResult = SetEntriesInAclW(1, &eaAccess, pOldDACL, &pNewDACL.get());
	if(dwResult != ERROR_SUCCESS) {
		printf("SetEntriesInAcl Error %u\n", dwResult);
		return dwResult;
	}

	// Attach the new ACL as the object's DACL.
	dwResult = SetNamedSecurityInfoW((LPWSTR)wstrFilePath, SE_FILE_OBJECT, siInfo, NULL, NULL, pNewDACL.get(), NULL);
	if(dwResult != ERROR_SUCCESS) {
		printf("SetNamedSecurityInfo Error %u\n", dwResult);
		return dwResult;
	}

	return dwResult;
}

class unique_handle {
	HANDLE mHandle = NULL;

public:
	unique_handle(HANDLE handle) : mHandle(handle) {}

	~unique_handle() {
		CloseHandle(mHandle);
	}

	HANDLE& get() {
		return mHandle;
	}
};

BOOL ModLoader::InjectDLL(DWORD dwProcessId, const wchar_t* dllPath) {
	/* Open the process with all access */
	unique_handle hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
	if(hProc.get() == NULL) {
		printf("Could not open the process (%u) HRESULT: %u", dwProcessId, GetLastError());
		return FALSE;
	}

	/* Allocate memory to hold the path to the DLL File in the process's memory */
	SIZE_T dllPathSize = wcslen(dllPath) * sizeof(wchar_t);
	LPVOID hRemoteMem = VirtualAllocEx(hProc.get(), NULL, dllPathSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if(hRemoteMem == NULL) {
		printf("Could not allocate memory in the process (%u) HRESULT: %u", dwProcessId, GetLastError());
		return FALSE;
	}

	/* Write the path to the DLL File in the memory just allocated */
	if(!WriteProcessMemory(hProc.get(), hRemoteMem, dllPath, dllPathSize, NULL)) {
		printf("Could not write memory in the process (%u) HRESULT: %u", dwProcessId, GetLastError());
		return FALSE;
	}

	/* Find the address of the LoadLibrary API */
	HMODULE hLocKernel32 = GetModuleHandleW(L"Kernel32");
	if(hLocKernel32 == NULL) {
		printf("Could not get a handle on Kernel32 in the process (%u) HRESULT: %u", dwProcessId, GetLastError());
		return FALSE;
	}

	FARPROC hLocLoadLibrary = GetProcAddress(hLocKernel32, "LoadLibraryW");
	if(hLocLoadLibrary == NULL) {
		printf("Could not find the locatin of LoadLibraryW in the process (%u) HRESULT: %u", dwProcessId, GetLastError());
		return FALSE;
	}

	/* Create a remote thread that invokes LoadLibrary for our DLL */
	unique_handle hRemoteThread = CreateRemoteThread(hProc.get(), NULL, 0, (LPTHREAD_START_ROUTINE)hLocLoadLibrary, hRemoteMem, 0, NULL);
	if(hRemoteThread.get() == NULL) {
		printf("Could not create a remote thread in the process (%u) HRESULT: %u", dwProcessId, GetLastError());
		return FALSE;
	}

	return TRUE;
}