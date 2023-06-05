#include <winternl.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#define CRT_ALLOW_UNSAFE
/* 1 KB */
#define KB 1024
/* 1 MB */
#define MB (KB*KB)

typedef struct _THREAD_BASIC_INFORMATION
{
	NTSTATUS ExitStatus;
	PVOID TebBaseAddress;
	CLIENT_ID ClientId;
	KAFFINITY AffinityMask;
	KPRIORITY Priority;
	KPRIORITY BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

typedef NTSTATUS(WINAPI *f_NtOpenThread)
    (PHANDLE ThreadHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes, CLIENT_ID *ClientId);

f_NtOpenThread NtOpenThread;

void findPointerStrings(uintptr_t *stack, size_t stackSize, HANDLE hProcess) {
    FILE *fp2;
    fp2 = fopen("ptrstringDump.txt", "w");
	for (size_t i = 0; i < (stackSize/sizeof(&stack[0])); i++) {
		if (stack[i] == '\0') 
			continue;

		char *stackValue = (char *)malloc(KB);

		if (ReadProcessMemory(hProcess, (void *)stack[i], &stackValue[0],
            KB, NULL))
			fprintf(fp2, "readmemory: pointer string: %s\n", stackValue);

        free(stackValue);
	}
    fclose(fp2);
}

void findLocalStrings(uintptr_t *stack, size_t stackSize) {
    FILE *fp1;
    fp1 = fopen("localstringDump.txt", "w");
	for (size_t i = 0; i < stackSize; i++) {
        if (stack[i] == '\0')
            continue;

		size_t copyLen = stackSize - i;

		if (copyLen > KB)
			copyLen = KB;

		char *stackValue = (char *)malloc(copyLen);
		memcpy(&stackValue[0], &stack[i], copyLen);
        fprintf(fp1, "readmemory: local string: %s\n", stackValue);

        free(stackValue);
	}
    fclose(fp1);
}

void getHeapStrings(HANDLE hProcess)
{
	MEMORY_BASIC_INFORMATION mbi;
    FILE *fp0;
    fp0 = fopen("heapstringDump.txt", "w"); 
	// Loop all the memory pages and search contents for strings
	for (char *pAddr = NULL; VirtualQueryEx(hProcess, pAddr, &mbi,
        sizeof(mbi)); pAddr += mbi.RegionSize) {
		if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS |
            PAGE_GUARD | PAGE_EXECUTE)))
			continue;

		char *page = (char *)malloc(mbi.RegionSize);

		if (!ReadProcessMemory(hProcess, mbi.BaseAddress, &page[0],
            mbi.RegionSize, NULL))
			continue;

		for (size_t i = 0; i < mbi.RegionSize; i++) {
			if (page[i] == '\0') continue;
			

			size_t copyLen = mbi.RegionSize - i;

			if (copyLen > KB)
                copyLen = KB;
			
			char *heapValue = (char *)malloc(copyLen);
			memcpy(&heapValue[0], &page[i], copyLen);
            fprintf(fp0, "readmemory: heap string: %s\n", heapValue);
		}
	}
    fclose(fp0);
}

void getStackStrings(HANDLE hProcess, HANDLE hThread)
{
    THREAD_BASIC_INFORMATION threadInfo;

    memset(&threadInfo, 0, sizeof(threadInfo));
    if (!NT_SUCCESS(NtQueryInformationThread(hThread, (THREADINFOCLASS)
    ThreadBasicInformation, &threadInfo, sizeof(threadInfo), NULL))) {
        return;
    }

    NT_TIB teb;
    memset(&teb, 0, sizeof(teb));

    if (!ReadProcessMemory(hProcess, threadInfo.TebBaseAddress, &teb,
    sizeof(teb), 0)) {
		return;
	}

    size_t stackSize = (uintptr_t)(teb.StackBase)-
    (uintptr_t)(teb.StackLimit);

    uintptr_t *stack = (uintptr_t *)malloc(stackSize);

    if(!ReadProcessMemory(hProcess, teb.StackLimit, stack, stackSize, NULL)) {
        return;
    }
    findPointerStrings(stack, stackSize, hProcess);
    findLocalStrings(stack, stackSize/sizeof(uintptr_t));
}

DWORD findThread(SYSTEM_THREAD_INFORMATION *pThread, 
    SYSTEM_PROCESS_INFORMATION *pProcess, HANDLE hProcess)
{
    for (DWORD32 i = 0; i < pProcess->NumberOfThreads; i++) {
        HANDLE hThread = NULL;
        OBJECT_ATTRIBUTES objAttr;
        memset(&objAttr, 0, sizeof(objAttr));
        objAttr.Length = sizeof(objAttr);

        if(NT_SUCCESS(NtOpenThread(&hThread, THREAD_QUERY_INFORMATION,
            &objAttr, &pThread->ClientId))) {
                getHeapStrings(hProcess);
                getStackStrings(hProcess, hThread);
                CloseHandle(hThread);
            }
        pThread++;
    }
    return EXIT_SUCCESS;
}

SYSTEM_PROCESS_INFORMATION *getpProcess(void)
{
    ULONG retLen = 0;
    SYSTEM_PROCESS_INFORMATION *pProcess = NULL;
    
    NtQuerySystemInformation(SystemProcessInformation, NULL, 0, &retLen);

    if ((pProcess = (SYSTEM_PROCESS_INFORMATION *)malloc(retLen))
        == NULL) {
        fprintf(stderr, "readmemory: failed to allocate %lu of memory :^(",
        retLen);
        return NULL;
    }
    /* recurse if failed */
    if(!NT_SUCCESS(NtQuerySystemInformation(SystemProcessInformation,
        pProcess, retLen, NULL))) {
            free(pProcess);
            return getpProcess();
        }
    return pProcess;
}

DWORD findProcess(DWORD procID, HANDLE hProcess)
{
    SYSTEM_PROCESS_INFORMATION *pProcess = getpProcess();

    while((intptr_t)pProcess->UniqueProcessId != procID) {
        if(!pProcess->NextEntryOffset) {
            fputs("readmemory: no matching process found :^(\n", stdout);
            return EXIT_FAILURE;
        }
        pProcess = (SYSTEM_PROCESS_INFORMATION *)
        ((unsigned char *)pProcess+pProcess->NextEntryOffset);
    }

    SYSTEM_THREAD_INFORMATION *pThread =
        (SYSTEM_THREAD_INFORMATION *)((unsigned char *)pProcess+
        sizeof(SYSTEM_PROCESS_INFORMATION));

    findThread(pThread, pProcess, hProcess);
    return EXIT_SUCCESS;
}


static inline void help(void)
{
    char *helpmsg = "Usage: readmemory [procID]\n"
    "Note that the readmemory will dump heap, local and pointer strings as .tx"
    "t files.\n\n";
    fputs(helpmsg, stdout);
}

int main(int argc, char *argv[])
{
    NtOpenThread = (f_NtOpenThread)GetProcAddress
    (GetModuleHandle("ntdll.dll"), "NtOpenThread");
    
    HANDLE hProcess = NULL;
    DWORD procID = 0;

    /* Help */

    for (int i = argc-1; i >= 0; i--) {
        if (strstr(argv[i], "-h") != NULL) {
            help();
            return EXIT_SUCCESS;
        }
    }

    if (argc == 1) {
        fputs("readmemory: have you considered writing one argument? :^)\n",
            stdout);
        return EXIT_SUCCESS;
    } else if (argc == 2) {
        procID = atoi(argv[1]);
    } else if (argc > 2) {
        fputs("readmemory: one argument only!", stdout);
        return EXIT_SUCCESS;
    }

    if ((hProcess = OpenProcess(PROCESS_VM_READ, 0, procID)) == NULL) {
        fprintf(stderr, "readmemory: failed to open process [%lu]"
        ", aborting\n", procID);
        abort();
    }

    fprintf(stdout, "readmemory: process opened [%lu]\n", procID);
    findProcess(procID, hProcess);
    CloseHandle(hProcess);
    fputs("readmemory: The text files have been successfully dumped\n",
    stdout);

    return EXIT_SUCCESS;
}
