#include <windows.h>
#include <stdio.h>

#define IOCTL_SEND_DATA_TO_PORT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_WRITE_ACCESS)

int main() {
    HANDLE hDevice;
    DWORD bytesReturned;
    char buffer[256];


    hDevice = CreateFileW(
        L"\\\\.\\SerialPortDriver",  
        GENERIC_READ | GENERIC_WRITE, 
        0,                           
        NULL,                         
        OPEN_EXISTING,                
        FILE_ATTRIBUTE_NORMAL,        
        NULL                         
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("Failed to open the device. Error: %lu\n", GetLastError());
        return 1;
    }

    snprintf(buffer, sizeof(buffer), "Hello, Serial Port!\n");

    if (!DeviceIoControl(
        hDevice,                    
        IOCTL_SEND_DATA_TO_PORT,    
        buffer,                     
        strlen(buffer) + 1,         
        NULL,                       
        0,                          
        &bytesReturned,             
        NULL                        
    )) {
        printf("DeviceIoControl failed. Error: %lu\n", GetLastError());
        CloseHandle(hDevice);
        return 1;
    }

    printf("Data sent to the driver: %s\n", buffer);

    CloseHandle(hDevice);

    return 0;
}
