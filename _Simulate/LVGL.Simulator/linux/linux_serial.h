#ifndef LVGL_SIMULATOR_LINUX_SERIAL_H
#define LVGL_SIMULATOR_LINUX_SERIAL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*LinuxSerialReceiveCallback)(uint8_t byte);

int LinuxSerial_Open(const char *path, unsigned int baud,
                     LinuxSerialReceiveCallback callback);
int LinuxSerial_Write(int handle, const uint8_t *data, size_t length);
int LinuxSerial_Read(int handle, uint8_t *data, size_t length);
int LinuxSerial_IsOpen(int handle);
void LinuxSerial_CloseAll(void);

#ifdef __cplusplus
}
#endif

#endif
