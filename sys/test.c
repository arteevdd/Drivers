#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include "GpIoctl.h"

int main() {
    HANDLE hDevice;

    while (true) {
        hDevice = CreateFile(
            "\\\\.\\SerialPortDriver",
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (hDevice != INVALID_HANDLE_VALUE) {
            printf("Device opened successfully. Starting transmitter loop...\n");
            break;
        } else {
            DWORD error = GetLastError();
            printf("Failed to open device. Error: %lu. Retrying...\n", error);
            Sleep(1000);
        }
    }

    while (true) {
        BOOL isReady = FALSE;
        DWORD bytesReturned;

        // Опрос готовности передатчика
        if (DeviceIoControl(
                hDevice,
                IOCTL_CHECK_TRANSMITTER_READY,
                NULL,
                0,
                &isReady,
                sizeof(isReady),
                &bytesReturned,
                NULL
            )) {
            if (isReady) {
                CHAR byteToWrite = 'A';
                if (DeviceIoControl(
                        hDevice,
                        IOCTL_WRITE_BYTE_TO_PORT,
                        &byteToWrite,
                        sizeof(byteToWrite),
                        NULL,
                        0,
                        &bytesReturned,
                        NULL
                    )) {
                    printf("Byte '%c' written to the port successfully.\n", byteToWrite);
                } else {
                    printf("Failed to write byte to the port. Error: %lu\n", GetLastError());
                }
            } else {
                printf("Transmitter is not ready. Waiting...\n");
            }
        } else {
            printf("Failed to check transmitter readiness. Error: %lu\n", GetLastError());
        }

        Sleep(500);
    }

    CloseHandle(hDevice);
    return 0;
}
