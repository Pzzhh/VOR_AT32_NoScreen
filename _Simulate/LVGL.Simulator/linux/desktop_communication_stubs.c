#include <stdarg.h>
#include <stddef.h>

void Rk3588_Init(void) {}

void Rk3588_R_Printf(const char * format, ...)
{
    (void)format;
}

void Rk3588_L_Printf(const char * format, ...)
{
    (void)format;
}

int Rk3588_ReadLine(const char * format, ...)
{
    (void)format;
    return 0;
}

int Rk3588_ReadLineArg(const char * format, va_list * args)
{
    (void)format;
    (void)args;
    return 0;
}

int Rk3588_R_ReadLineArg(const char * format, va_list * args)
{
    return Rk3588_ReadLineArg(format, args);
}

int Rk3588_L_ReadLineArg(const char * format, va_list * args)
{
    return Rk3588_ReadLineArg(format, args);
}

int Rk3588_ReadLinearray(char * bytes)
{
    (void)bytes;
    return 0;
}

int Rk3588_R_ReadLinearray(char * bytes)
{
    return Rk3588_ReadLinearray(bytes);
}

int Rk3588_L_ReadLinearray(char * bytes)
{
    return Rk3588_ReadLinearray(bytes);
}
