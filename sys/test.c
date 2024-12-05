#include <windows.h>
#include <stdio.h>

#define IOCTL_CHECK_TRANSMITTER_READY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_WRITE_BYTE_TO_PORT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// Удобная функция для вывода информации об ошибках
void PrintLastError(const char* message) {
    DWORD errorCode = GetLastError();
    LPVOID errorMessage;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorCode,
        0,
        (LPSTR)&errorMessage,
        0,
        NULL
    );
    printf("%s: Error %lu - %s\n", message, errorCode, (char*)errorMessage);
    LocalFree(errorMessage);
}

int main() {
    HANDLE hDevice;
    DWORD bytesReturned;
    BOOLEAN transmitterReady;
    char byteToSend = 'A'; // Байт, который будем отправлять
    BOOL result;

    // Открытие устройства через символическую ссылку
    hDevice = CreateFile(
        "\\\\.\\SerialPortDriver", // Символическая ссылка, заданная в драйвере
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        PrintLastError("Failed to open device");
        return 1;
    }

    printf("Device opened successfully.\n");

    // Проверка готовности передатчика
    printf("Checking transmitter readiness...\n");
    result = DeviceIoControl(
        hDevice,
        IOCTL_CHECK_TRANSMITTER_READY,
        NULL,
        0,
        &transmitterReady,
        sizeof(transmitterReady),
        &bytesReturned,
        NULL
    );

    if (!result) {
        PrintLastError("DeviceIoControl for IOCTL_CHECK_TRANSMITTER_READY failed");
        CloseHandle(hDevice);
        return 1;
    }

    if (bytesReturned != sizeof(transmitterReady)) {
        printf("Unexpected response size: expected %zu, got %lu\n", sizeof(transmitterReady), bytesReturned);
    }

    if (transmitterReady) {
        printf("Transmitter is ready.\n");
    } else {
        printf("Transmitter is not ready.\n");
    }

    // Отправка байта в порт
    printf("Sending byte '%c' (0x%x) to port...\n", byteToSend, byteToSend);
    result = DeviceIoControl(
        hDevice,
        IOCTL_WRITE_BYTE_TO_PORT,
        &byteToSend,
        sizeof(byteToSend),
        NULL,
        0,
        &bytesReturned,
        NULL
    );

    if (!result) {
        PrintLastError("DeviceIoControl for IOCTL_WRITE_BYTE_TO_PORT failed");
        CloseHandle(hDevice);
        return 1;
    }

    printf("Byte sent successfully.\n");

    // Закрытие дескриптора устройства
    CloseHandle(hDevice);
    printf("Device closed.\n");

    return 0;
}
