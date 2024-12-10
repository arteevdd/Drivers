#include <ntddk.h>
#include "GpIoctl.h"

#define COM1_PORT_BASE_ADDRESS 0x2F8

typedef struct _DEVICE_EXTENSION {
    BOOLEAN TransmitterReady;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

BOOLEAN IsTransmitterReady(USHORT PortBase) {
    UCHAR LSR = READ_PORT_UCHAR((PUCHAR)(PortBase + 5));
    DbgPrint("Checking transmitter readiness: LSR = 0x%x\n", LSR);
    return (LSR & 0x20) != 0;
}

NTSTATUS DeviceIoControlHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;
    ULONG bytesReturned = 0;
    PCHAR buffer = NULL;

    switch (ioStack->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_CHECK_TRANSMITTER_READY:
            DbgPrint("IOCTL_CHECK_TRANSMITTER_READY received.\n");
            deviceExtension->TransmitterReady = IsTransmitterReady(COM1_PORT_BASE_ADDRESS);
            bytesReturned = sizeof(deviceExtension->TransmitterReady);
            RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, &deviceExtension->TransmitterReady, bytesReturned);
            break;

        case IOCTL_WRITE_BYTE_TO_PORT:
            DbgPrint("IOCTL_WRITE_BYTE_TO_PORT received.\n");
            buffer = (PCHAR)Irp->AssociatedIrp.SystemBuffer;

            if (IsTransmitterReady(COM1_PORT_BASE_ADDRESS)) {
                DbgPrint("Writing byte '%c' (0x%x) to port.\n", *buffer, *buffer);
                WRITE_PORT_UCHAR((PUCHAR)COM1_PORT_BASE_ADDRESS, *buffer);
            } else {
                DbgPrint("Transmitter not ready.\n");
                status = STATUS_DEVICE_NOT_READY;
            }
            bytesReturned = sizeof(*buffer);
            break;

        default:
            DbgPrint("Invalid IOCTL code received: 0x%x\n", ioStack->Parameters.DeviceIoControl.IoControlCode);
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    Irp->IoStatus.Information = bytesReturned;
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS CreateCloseHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    DbgPrint("Create or Close request received.\n");
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS ConfigureSerialPort(USHORT PortBase) {
    DbgPrint("Configuring serial port at base address 0x%x.\n", PortBase);
    WRITE_PORT_UCHAR((PUCHAR)(PortBase + 4), 0x00);
    WRITE_PORT_UCHAR((PUCHAR)(PortBase + 3), 0x80);
    WRITE_PORT_UCHAR((PUCHAR)(PortBase + 0), 0x01);
    WRITE_PORT_UCHAR((PUCHAR)(PortBase + 1), 0x00);
    WRITE_PORT_UCHAR((PUCHAR)(PortBase + 3), 0x03);
    WRITE_PORT_UCHAR((PUCHAR)(PortBase + 2), 0xC7);
    WRITE_PORT_UCHAR((PUCHAR)(PortBase + 4), 0x0B);
    DbgPrint("Serial port configured successfully.\n");
    return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath) {
    UNICODE_STRING deviceName;
    UNICODE_STRING symbolicLinkName;
    PDEVICE_OBJECT deviceObject = NULL;
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS status;

    DbgPrint("DriverEntry: Initializing driver...\n");
    RtlInitUnicodeString(&deviceName, L"\\Device\\SerialPortDriver");
    RtlInitUnicodeString(&symbolicLinkName, L"\\DosDevices\\SerialPortDriver");

    status = IoCreateDevice(
        DriverObject,
        sizeof(DEVICE_EXTENSION),
        &deviceName,
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE,
        &deviceObject
    );
    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed to create device: 0x%x\n", status);
        return status;
    }

    deviceExtension = (PDEVICE_EXTENSION)deviceObject->DeviceExtension;
    deviceExtension->TransmitterReady = IsTransmitterReady(COM1_PORT_BASE_ADDRESS);

    if (!deviceExtension->TransmitterReady) {
        DbgPrint("Transmitter not ready at startup. Failing driver load.\n");
        IoDeleteDevice(deviceObject);
        return STATUS_DEVICE_NOT_READY;
    }

    status = ConfigureSerialPort(COM1_PORT_BASE_ADDRESS);
    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed to configure serial port: 0x%x\n", status);
        IoDeleteDevice(deviceObject);
        return status;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateCloseHandler;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateCloseHandler;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceIoControlHandler;

    status = IoCreateSymbolicLink(&symbolicLinkName, &deviceName);
    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed to create symbolic link: 0x%x\n", status);
        IoDeleteDevice(deviceObject);
        return status;
    }

    DbgPrint("DriverEntry: Driver initialized successfully.\n");
    return STATUS_SUCCESS;
}
