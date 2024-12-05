// GpIoctl.h
#ifndef GP_IOCTL_H
#define GP_IOCTL_H

// Определяем IOCTL-код для проверки готовности передатчика
#define IOCTL_CHECK_TRANSMITTER_READY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS)
// Определяем IOCTL-код для записи байта в порт
#define IOCTL_WRITE_BYTE_TO_PORT CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#endif // GP_IOCTL_H
