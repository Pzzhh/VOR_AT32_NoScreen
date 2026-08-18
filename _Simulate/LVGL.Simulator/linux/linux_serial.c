#include "linux_serial.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define LINUX_SERIAL_PORT_COUNT 3

typedef struct {
    int fd;
    int running;
    pthread_t thread;
    LinuxSerialReceiveCallback callback;
} LinuxSerialPort;

static LinuxSerialPort ports[LINUX_SERIAL_PORT_COUNT] = {
    {.fd = -1}, {.fd = -1}, {.fd = -1}
};

static speed_t baud_to_speed(unsigned int baud)
{
    return baud == 9600 ? B9600 : B115200;
}

static void *reader_thread(void *argument)
{
    LinuxSerialPort *port = (LinuxSerialPort *)argument;
    uint8_t buffer[128];

    while(port->running) {
        ssize_t count = read(port->fd, buffer, sizeof(buffer));
        if(count > 0) {
            for(ssize_t i = 0; i < count; ++i) port->callback(buffer[i]);
        }
        else if(count < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            break;
        }
        usleep(1000);
    }
    return NULL;
}

int LinuxSerial_Open(const char *path, unsigned int baud,
                     LinuxSerialReceiveCallback callback)
{
    int slot = -1;
    for(int i = 0; i < LINUX_SERIAL_PORT_COUNT; ++i) {
        if(ports[i].fd < 0) { slot = i; break; }
    }
    if(slot < 0 || path == NULL) return -1;

    int fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(fd < 0) {
        fprintf(stderr, "Linux serial: cannot open %s: %s\n", path, strerror(errno));
        return -1;
    }

    struct termios options;
    if(tcgetattr(fd, &options) != 0) {
        fprintf(stderr, "Linux serial: tcgetattr(%s): %s\n", path, strerror(errno));
        close(fd);
        return -1;
    }
    cfmakeraw(&options);
    speed_t speed = baud_to_speed(baud);
    cfsetispeed(&options, speed);
    cfsetospeed(&options, speed);
    options.c_cflag |= CLOCAL | CREAD;
    if(tcsetattr(fd, TCSANOW, &options) != 0) {
        fprintf(stderr, "Linux serial: tcsetattr(%s): %s\n", path, strerror(errno));
        close(fd);
        return -1;
    }

    ports[slot].fd = fd;
    ports[slot].callback = callback;
    if(callback != NULL) {
        ports[slot].running = 1;
        if(pthread_create(&ports[slot].thread, NULL, reader_thread, &ports[slot]) != 0) {
            ports[slot].running = 0;
            ports[slot].fd = -1;
            close(fd);
            return -1;
        }
    }
    return slot;
}

int LinuxSerial_Write(int handle, const uint8_t *data, size_t length)
{
    if(!LinuxSerial_IsOpen(handle) || data == NULL) return 0;
    size_t sent = 0;
    while(sent < length) {
        ssize_t count = write(ports[handle].fd, data + sent, length - sent);
        if(count > 0) sent += (size_t)count;
        else if(count < 0 && errno == EINTR) continue;
        else if(count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) { usleep(1000); continue; }
        else return 0;
    }
    return 1;
}

int LinuxSerial_Read(int handle, uint8_t *data, size_t length)
{
    if(!LinuxSerial_IsOpen(handle) || data == NULL || length == 0) return 0;
    ssize_t count = read(ports[handle].fd, data, length);
    if(count > 0) return (int)count;
    return 0;
}

int LinuxSerial_IsOpen(int handle)
{
    return handle >= 0 && handle < LINUX_SERIAL_PORT_COUNT && ports[handle].fd >= 0;
}

void LinuxSerial_CloseAll(void)
{
    for(int i = 0; i < LINUX_SERIAL_PORT_COUNT; ++i) {
        if(ports[i].fd < 0) continue;
        if(ports[i].callback != NULL) {
            ports[i].running = 0;
            pthread_join(ports[i].thread, NULL);
        }
        close(ports[i].fd);
        ports[i].fd = -1;
    }
}
