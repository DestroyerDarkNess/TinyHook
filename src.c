#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

void* DetourFunc(BYTE* src, const BYTE* dest, const DWORD length);
BYTE* pOriginalCode = NULL;

// Function to hook a function
void* DetourFunc(BYTE* src, const BYTE* dest, const DWORD length) {
    BYTE* jump = (BYTE*)malloc(length + 12);  // 12 bytes of space for jump and restore
    DWORD dwVirtualProtectBackup;

    // Backup and change memory protection to allow writing
    VirtualProtect(src, length + 12, PAGE_EXECUTE_READWRITE, &dwVirtualProtectBackup);

    // Save the original code and prepare the springboard code
    pOriginalCode = (BYTE*)malloc(length + 5);
    memcpy(pOriginalCode, src, length);
    BYTE* trampoline = jump;
    trampoline += length;

    // Create a jump statement to the hook function
    trampoline[0] = 0xE9;
    *(DWORD*)(trampoline + 1) = (DWORD)(src - trampoline - 5);

    // Prepare the jump statement to the original function from the springboard
    src[0] = 0xE9;
    *(DWORD*)(src + 1) = (DWORD)(dest - src - 5);

    // Restores original memory protection
    VirtualProtect(src, length + 12, dwVirtualProtectBackup, &dwVirtualProtectBackup);

    return jump;
}


typedef int (WINAPI *MessageBox_t)(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
MessageBox_t pMessageBoxOrig = NULL;

// Hook function para MessageBox
int WINAPI hookMessageBox(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
    printf("[+]\tHooked MessageBox: %s\n", lpText);

    // Restore the original code
    DWORD dwVirtualProtectBackup;
    VirtualProtect((BYTE*)pMessageBoxOrig, 5, PAGE_EXECUTE_READWRITE, &dwVirtualProtectBackup);
    memcpy((BYTE*)pMessageBoxOrig, pOriginalCode, 5);  // Restore the original code

    // Call the original MessageBox function
    int result = pMessageBoxOrig(hWnd, lpText, lpCaption, uType);

    // Restore the hook code
    BYTE* src = (BYTE*)pMessageBoxOrig;
    BYTE* dest = (BYTE*)hookMessageBox;
    DetourFunc(src, dest, 5);  // Reaplica el gancho

    // Call the original MessageBox function
    return result;
}

int main() {

   printf("[+]\tInitHook function called\n");

    // Get the address of MessageBoxA
    HMODULE user32 = GetModuleHandleA("user32.dll");
    pMessageBoxOrig = (MessageBox_t)GetProcAddress(user32, "MessageBoxA");

    if (pMessageBoxOrig != NULL) {
        // Detour the MessageBoxA function
        DetourFunc((BYTE*)pMessageBoxOrig, (BYTE*)hookMessageBox, 5); // 5 is the length of the instruction
    } else {
        printf("[-]\tFailed to get MessageBoxA address\n");
    }

    // Try the hook
    MessageBoxA(NULL, "Hello, World!", "Hook Test", MB_OK);
  
    return 0;
}
