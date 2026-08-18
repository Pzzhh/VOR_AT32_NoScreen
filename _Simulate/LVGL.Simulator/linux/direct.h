#ifndef LVGL_LINUX_DIRECT_COMPAT_H
#define LVGL_LINUX_DIRECT_COMPAT_H
#include <unistd.h>
#define _getcwd getcwd
#define _chdir chdir
#endif
