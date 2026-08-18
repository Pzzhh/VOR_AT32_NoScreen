#include <windows.h>
#include <stdio.h>
#include "stdint.h"
#include "serialPort.h"

#include <windows.h>
#include <stdio.h>

extern void OutputDebugPrintf(const char* strOutputString, ...);
static   HANDLE hSerial = 0;
static void (*Handle_cb)(uint8_t dr);

void SerialPort_SendBuf(uint8_t* buf, uint16_t len)
{
    DWORD bytesWritten;
    if (hSerial == 0)
        return;
    OutputDebugPrintf("\r\n [BL]<- ");
    for (int i = 0;i < len;i++)
        OutputDebugPrintf(".%02X", buf[i]);
    OutputDebugPrintf("\r\n");

    if (!WriteFile(hSerial, buf, len, &bytesWritten, NULL)) {
        CloseHandle(hSerial);
        return;
    }
}

static DWORD WINAPI Serial_Thread(LPVOID parameter)
{
    LPCWSTR comname = (LPCWSTR)parameter;
    if (hSerial != 0)
        return 0;

    LPCWSTR portName = comname;
    hSerial = CreateFile(portName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        0,
        OPEN_EXISTING,
        0,
        0);
    if (hSerial == INVALID_HANDLE_VALUE) {
        OutputDebugPrintf("Error opening serial port\n");
        hSerial = 0;
        return 0;
    }

    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams)) {
        OutputDebugPrintf("Error getting state\n");
        CloseHandle(hSerial);
        hSerial = 0;
        return 0;
    }

    dcbSerialParams.BaudRate = CBR_9600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        OutputDebugPrintf("Error setting serial port state\n");
        CloseHandle(hSerial);
        hSerial = 0;
        return 0;
    }

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hSerial, &timeouts)) {
        OutputDebugPrintf("Error setting timeouts\n");
        CloseHandle(hSerial);
        hSerial = 0;
        return 0;
    }

    uint8_t dataReceived[20];
    DWORD bytesRead;
    int count = 0;
    while (1)
    {
        if (!ReadFile(hSerial, dataReceived, sizeof(dataReceived), &bytesRead, NULL)) {
            OutputDebugPrintf("Error reading from serial port\n");
            CloseHandle(hSerial);
            hSerial = 0;
            return 0;
        }
        if (bytesRead)
        {
            OutputDebugPrintf("\r\n [BL]--> ");
            count = 0;
            while (bytesRead--)
            {
                uint32_t hex = dataReceived[count];
                OutputDebugPrintf("%c", hex);
                if (Handle_cb)
                    Handle_cb(dataReceived[count]);
                count++;
                Sleep(1);
            }
            OutputDebugPrintf("\r\n");
        }
    }

    CloseHandle(hSerial);
    hSerial = 0;
    return 0;
}
void SerialPort_Init(void (*cb)(uint8_t dr), LPCWSTR comname)
{
    HANDLE hThread;
    DWORD dwThreadId;
    Handle_cb = cb;
    hThread = CreateThread(NULL,		 // 默认安全属性
        0,			 // 默认堆栈大小
        Serial_Thread,	 // 线程函数
        (LPVOID)comname,	 // 传递给线程函数的参数
        0,			 // 默认创建标志
        &dwThreadId); // 线程ID)
    if (hThread != NULL)
        CloseHandle(hThread);
}

