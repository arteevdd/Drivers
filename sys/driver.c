#include <ntddk.h>
#include "GpIoctl.h"

// Адрес базового порта COM2, который будем использовать
#define COM1_PORT_BASE_ADDRESS 0x2F8

// Структура расширения устройства для хранения состояния передатчика
typedef struct _DEVICE_EXTENSION {
    BOOLEAN TransmitterReady; // Флаг готовности передатчика
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

// Проверяет готовность передатчика по значению регистра Line Status Register (LSR)
BOOLEAN IsTransmitterReady(USHORT PortBase) {
    UCHAR LSR = READ_PORT_UCHAR((PUCHAR)(PortBase + 5)); // Читаем LSR
    DbgPrint("Checking transmitter readiness: LSR = 0x%x\n", LSR);
    return (LSR & 0x20) != 0; // Проверяем бит "Transmitter Holding Register Empty"
}

// Ожидание готовности передатчика с периодическим опросом
NTSTATUS WaitForTransmitterReady(USHORT PortBase) {
    LARGE_INTEGER delayTime;
    delayTime.QuadPart = -10000 * 100; // Задержка 100 мс (100-наносекундные единицы)
    DbgPrint("Waiting for transmitter to become ready...\n");

    while (!IsTransmitterReady(PortBase)) { // Пока передатчик не готов
        DbgPrint("Transmitter not ready, waiting...\n");
        KeDelayExecutionThread(KernelMode, FALSE, &delayTime); // Ожидание
    }

    DbgPrint("Transmitter is ready.\n");
    return STATUS_SUCCESS;
}

// Обработчик запросов управления устройством (IOCTL)
NTSTATUS DeviceIoControlHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    PDEVICE_EXTENSION deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension; // Получаем расширение устройства
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp); // Стек ввода/вывода
    NTSTATUS status = STATUS_SUCCESS;
    ULONG bytesReturned = 0; // Количество возвращаемых данных
    PCHAR buffer = NULL; // Буфер для записи данных

    switch (ioStack->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_CHECK_TRANSMITTER_READY:
            // Проверка готовности передатчика
            DbgPrint("IOCTL_CHECK_TRANSMITTER_READY received.\n");
            deviceExtension->TransmitterReady = IsTransmitterReady(COM1_PORT_BASE_ADDRESS);
            bytesReturned = sizeof(deviceExtension->TransmitterReady);
            RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, &deviceExtension->TransmitterReady, bytesReturned); // Возвращаем результат
            break;

        case IOCTL_WRITE_BYTE_TO_PORT:
            // Запись байта в порт
            DbgPrint("IOCTL_WRITE_BYTE_TO_PORT received.\n");
            buffer = (PCHAR)Irp->AssociatedIrp.SystemBuffer; // Получаем байт из пользовательского запроса
            status = WaitForTransmitterReady(COM1_PORT_BASE_ADDRESS);

            if (NT_SUCCESS(status)) {
                DbgPrint("Writing byte '%c' (0x%x) to port.\n", *buffer, *buffer);
                WRITE_PORT_UCHAR((PUCHAR)COM1_PORT_BASE_ADDRESS, *buffer); // Пишем байт в порт
            } else {
                DbgPrint("Transmitter not ready, timeout occurred.\n");
                status = STATUS_TIMEOUT;
            }
            bytesReturned = sizeof(*buffer); // Возвращаем размер записанного байта
            break;

        default:
            // Неизвестный IOCTL-код
            DbgPrint("Invalid IOCTL code received: 0x%x\n", ioStack->Parameters.DeviceIoControl.IoControlCode);
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    Irp->IoStatus.Information = bytesReturned; // Устанавливаем количество данных в запросе
    Irp->IoStatus.Status = status; // Устанавливаем статус выполнения
    IoCompleteRequest(Irp, IO_NO_INCREMENT); // Завершаем обработку IRP

    return status;
}

// Обработчик запросов на создание или закрытие устройства
NTSTATUS CreateCloseHandler(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    DbgPrint("Create or Close request received.\n");
    Irp->IoStatus.Status = STATUS_SUCCESS; // Успешное выполнение
    IoCompleteRequest(Irp, IO_NO_INCREMENT); // Завершаем обработку IRP
    return STATUS_SUCCESS;
}

// Конфигурация последовательного порта
NTSTATUS ConfigureSerialPort(USHORT PortBase) {
    DbgPrint("Configuring serial port at base address 0x%x.\n", PortBase);
    WRITE_PORT_UCHAR((PUCHAR)(PortBase + 4), 0x00); // Управление
    WRITE_PORT_UCHAR((PUCHAR)(PortBase + 3), 0x80); // Включение доступа к делителю
    WRITE_PORT_UCHAR((PUCHAR)(PortBase + 0), 0x01); // Скорость (делитель, младший байт)
    WRITE_PORT_UCHAR((PUCHAR)(PortBase + 1), 0x00); // Скорость (делитель, старший байт)
    WRITE_PORT_UCHAR((PUCHAR)(PortBase + 3), 0x03); // Конфигурация (8N1)
    WRITE_PORT_UCHAR((PUCHAR)(PortBase + 2), 0xC7); // Включение FIFO
    WRITE_PORT_UCHAR((PUCHAR)(PortBase + 4), 0x0B); // Установка линий DTR и RTS
    DbgPrint("Serial port configured successfully.\n");
    return STATUS_SUCCESS;
}

// Точка входа драйвера
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath) {
    UNICODE_STRING deviceName;
    UNICODE_STRING symbolicLinkName;
    PDEVICE_OBJECT deviceObject = NULL;
    PDEVICE_EXTENSION deviceExtension;
    NTSTATUS status;

    DbgPrint("DriverEntry: Initializing driver...\n");
    RtlInitUnicodeString(&deviceName, L"\\Device\\SerialPortDriver"); // Имя устройства
    RtlInitUnicodeString(&symbolicLinkName, L"\\DosDevices\\SerialPortDriver"); // Символическая ссылка

    // Создание устройства
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

    deviceExtension = (PDEVICE_EXTENSION)deviceObject->DeviceExtension; // Инициализируем расширение устройства
    deviceExtension->TransmitterReady = FALSE;

    status = ConfigureSerialPort(COM1_PORT_BASE_ADDRESS); // Настраиваем порт
    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed to configure serial port: 0x%x\n", status);
        IoDeleteDevice(deviceObject); // Удаляем устройство при ошибке
        return status;
    }

    // Назначаем обработчики запросов
    DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateCloseHandler;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateCloseHandler;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceIoControlHandler;

    // Создаем символическую ссылку
    status = IoCreateSymbolicLink(&symbolicLinkName, &deviceName);
    if (!NT_SUCCESS(status)) {
        DbgPrint("Failed to create symbolic link: 0x%x\n", status);
        IoDeleteDevice(deviceObject);
        return status;
    }

    DbgPrint("DriverEntry: Driver initialized successfully.\n");
    return STATUS_SUCCESS;
}


