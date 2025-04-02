#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define IOCTL_TRIGGER_BSOD CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

int main(int argc, char* argv[])
{
    if (argc != 3) {
        printf("Usage: %s <DriverName> <CustomBugCheckCode>\n", argv[0]);
        return 1;
    }

    // Get the driver name and custom bug check code from command line arguments
    wchar_t deviceName[256];
    size_t convertedChars = 0;

    // Use mbstowcs_s for safer conversion
    if (mbstowcs_s(&convertedChars, deviceName, sizeof(deviceName) / sizeof(wchar_t), argv[1], _TRUNCATE) != 0) {
        printf("Failed to convert driver name to wide string.\n");
        return 1;
    }

    unsigned int customBugCheckCode = strtoul(argv[2], NULL, 16); // Convert hex string to unsigned int

    HANDLE hDevice = CreateFile(
        deviceName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("[+] Failed to open device: %d\n", GetLastError());
        return 1;
    }

    // Send the IOCTL to trigger the BSOD
    DWORD bytesReturned;
    BOOL result = DeviceIoControl(
        hDevice,
        IOCTL_TRIGGER_BSOD,
        &customBugCheckCode,
        sizeof(customBugCheckCode),
        NULL,
        0,
        &bytesReturned,
        NULL
    );

    if (!result) {
        printf("[-] Failed to send IOCTL: %d\n", GetLastError());
    }
    else {
        printf("[+] BSOD triggered successfully with code: 0x%X\n", customBugCheckCode);
    }

    CloseHandle(hDevice);
    return 0;
}