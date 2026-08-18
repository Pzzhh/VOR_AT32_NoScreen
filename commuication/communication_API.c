#include "communication_API.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"

// FreeRTOS includes for task diagnostics

#ifdef ATM32F43_xx
#include "FreeRTOS.h"
#include "task.h"
#include "control/control.h"
#define USE_USB 1
#define USE_BL 1
#else
#define USE_USB 0
#define USE_BL 1
#ifdef use_linux
#include "linux_serial.h"
static int linux_usb_port = -1;
static uint16_t USB_init_flag = 0;
#else
#include "../serial/serialPort.h"
#endif
#include "../INTERFACE/UI/control/control.h"
#endif

#if !defined(ATM32F43_xx) && !defined(LV_PLATFORM_LINUX)
#include <windows.h>
#endif

#if USE_USB
// #include "usbd_usr.h"
// #include "usbd_desc.h"

// #include "usbd_cdc_core.h"

// extern USB_OTG_CORE_HANDLE USB_OTG_dev;
// extern USBD_Usr_cb_TypeDef USR_cb;
#endif

#if USE_BL

#include "BLUETOOTH/BlueTooth.h"
#include "../HARDWARE/USART/USART3.h"
#include "rk3588_uart.h"
#endif

#define USE_MPU 1
#if USE_MPU
#include "mpu/jy60.h"
#endif



extern Task_control_info control_info;

// 命令窗口 需要线程

// log print

// oriange pie

// ble 要开线程通信

void HAL_communication_Bl_Init(void)
{
	// 串口 init
}

// yj-60 init
void HAL_communication_Mpu_Init(void)
{
#if USE_MPU
	JY60_Init();
#endif
}

void Hal_Rk3588_Printf(const char *strOutputString, ...)
{
	//     va_arg
	// Rk3588_Printf(strOutputString,...);
}

int Hal_Rk3588_R_ReadLine(const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	int res = Rk3588_R_ReadLineArg(format, &arg);
	va_end(arg);
	return res;
	//    Rk3588_Printf(strOutputString,...);
}

int Hal_Rk3588_R_Readarray(char *bytes)
{
	return Rk3588_R_ReadLinearray(bytes);
	//    Rk3588_Printf(strOutputString,...);
}

int Hal_Rk3588_L_Readarray(char *bytes)
{
	return Rk3588_L_ReadLinearray(bytes);
	//    Rk3588_Printf(strOutputString,...);
}

int Hal_Rk3588_L_ReadLine(const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	int res = Rk3588_L_ReadLineArg(format, &arg);
	va_end(arg);
	return res;
	//    Rk3588_Printf(strOutputString,...);
}

uint8_t HAL_MPU_Get_Angle(float *angle)
{
#if USE_MPU
	return JY60_Get_Inc(angle);
#else
	return 0;
#endif
}

uint8_t HAL_BL_USART_INIT(void)
{
	extern void usart_protocol_InterruptHandle(uint8_t dr);
#if USE_BL
#ifdef ATM32F43_xx
	USART3_INIT(9600,
				0);
	usart_protocol_init(USART3_Send_Package);
#else
	// SerialPort_Init(usart_protocol_InterruptHandle, L"COM1");
	// usart_protocol_init(SerialPort_SendBuf);
#endif // ATM32F43_xx

#endif
	return 1;
}

uint8_t HAL_BL_USART_Server(void)
{
#if USE_BL
	usart_protocol_decoding(0);
#endif
	return 1;
}

uint8_t HAL_USB_INIT()
{
#if USE_USB
	// USBD_Init(&USB_OTG_dev,
	// 	USB_OTG_FS_CORE_ID,
	// 	&USR_desc,
	// 	&USBD_CDC_cb,
	// 	&USR_cb);
#endif
	return 1;
}

int HAL_USB_Isconnect()
{
#if USE_USB
// extern	vu8 bDeviceState;
#endif
	// return bDeviceState;
}

uint8_t HAL_USB_TX(uint8_t *str, uint16_t len)
{
#ifdef ATM32F43_xx

	// extern void usbd_cdc_senddata(void* pdev, uint8_t * str, uint16_t len);
	// usbd_cdc_senddata(&USB_OTG_dev, str, len);
#endif // ATM32F43_xx
	return 0;
}

// ========== USB ACM Implementation ==========
#ifdef ATM32F43_xx
extern uint8_t USB_CDC_Send(uint8_t CH, uint8_t *buf, uint16_t len);
extern uint16_t USB_CDC_GET(uint8_t CH, uint8_t *buf);
extern uint16_t USB_init_flag;
#endif

/**
 * @brief Initialize USB ACM virtual serial port
 * @retval 1: success, 0: failed
 */
uint8_t HAL_USB_ACM_Init(void)
{
#ifdef ATM32F43_xx
	extern void USB1_Init(void);
	USB1_Init();
	return 1;
#elif defined(use_linux)
	linux_usb_port = LinuxSerial_Open("/tmp/ttyUSB", 115200, NULL);
	USB_init_flag = LinuxSerial_IsOpen(linux_usb_port) ? 1 : 0;
	return USB_init_flag;
#else
	return 0;
#endif
}

/**
 * @brief Send data through USB ACM
 * @param data: pointer to data buffer
 * @param len: length of data to send
 * @retval 1: success, 0: failed or busy
 */
uint8_t HAL_USB_ACM_Send(uint8_t *data, uint16_t len)
{
#ifdef ATM32F43_xx
	if (!USB_init_flag)
		return 0;
	return (USB_CDC_Send(1, data, len) == 0) ? 1 : 0;
#elif defined(use_linux)
	return LinuxSerial_Write(linux_usb_port, data, len) ? 1 : 0;
#else
	return 0;
#endif
}

/**
 * @brief Read data from USB ACM
 * @param buffer: pointer to receive buffer
 * @param maxlen: maximum length to read
 * @retval number of bytes read
 */
uint16_t HAL_USB_ACM_Read(uint8_t *buffer, uint16_t maxlen)
{
#ifdef ATM32F43_xx
	if (!USB_init_flag || buffer == NULL)
		return 0;
	return USB_CDC_GET(1, buffer);
#elif defined(use_linux)
	if(!USB_init_flag || buffer == NULL) return 0;
	return (uint16_t)LinuxSerial_Read(linux_usb_port, buffer, maxlen);
#else
	return 0;
#endif
}

/**
 * @brief Check if USB ACM is ready
 * @retval 1: ready, 0: not ready
 */
uint8_t HAL_USB_ACM_IsReady(void)
{
#ifdef ATM32F43_xx
	return USB_init_flag;
#elif defined(use_linux)
	return LinuxSerial_IsOpen(linux_usb_port) ? 1 : 0;
#else
	return 0;
#endif
}

// ========== USB ACM Log Functions ==========

/**
 * @brief Print formatted log to USB ACM
 * @param format: printf-style format string
 * @param ...: variable arguments
 */
void HAL_USB_Log_Printf(const char *format, ...)
{
#if defined(ATM32F43_xx) || defined(use_linux)
	if (!USB_init_flag)
		return;

	char log_buffer[256];
	va_list args;
	va_start(args, format);
	int len = vsnprintf(log_buffer, sizeof(log_buffer), format, args);
	va_end(args);

	if (len > 0 && len < sizeof(log_buffer))
	{
		HAL_USB_ACM_Send((uint8_t *)log_buffer, len);
	}
	ControlDelay(1); // hort delay to ensure data is sent
#endif
}
#pragma anon_unions
/**
 * @brief Generate detailed task info string (without timestamp)
 * @param task: pointer to task parameter structure
 * @param buffer: output buffer
 * @param bufsize: buffer size
 */
static void Task_Info_ToString(void *task_ptr, char *buffer, uint16_t bufsize)
{
	if (task_ptr == NULL || buffer == NULL || bufsize == 0)
		return;

	// Include task structure definition
	typedef enum
	{
		Task_VOR = 0,
		Task_Continue,
		Task_OVAR,
		Task_VHIT,
		Task_TC
	} Task_Mode_enum;

	typedef enum
	{
		Ex_VOR = 0,
		Ex_OKR,
		Ex_BOTH,
		Ex_BOTH_R
	} VOR_Exmode;

	typedef struct
	{
		Task_Mode_enum mode;
		union
		{
			struct
			{
				uint32_t Counter;
				float Vel;
				float Freq;
				VOR_Exmode ExMode;
			} VOR;
			struct
			{
				int Sec;
				float Vel;
				float okr_Vel;
			} CONT;
			struct
			{
				int Sec;
				float Vel;
				float Inc_Degree;
			} OVAR;
			struct
			{
				uint32_t Counter;
				uint8_t IsCW;
			} VHIT;
			struct
			{
				uint32_t Counter;
				float Vel;
			} TC;
		};
	} Task_Param;

	Task_Param *task = (Task_Param *)task_ptr;

	static const char vorStrList[][10] = {"VOR", "OKR", "VOR_OKR", "VOR_OKRR"};

	switch (task->mode)
	{
	case Task_VOR:
		snprintf(buffer, bufsize, "VOR: %s T%d V%d F%.1f",
				 vorStrList[task->VOR.ExMode],
				 (int)task->VOR.Counter,
				 (int)task->VOR.Vel,
				 task->VOR.Freq);
		break;
	case Task_Continue:
		snprintf(buffer, bufsize, "CONT: S%d V%d",
				 task->CONT.Sec,
				 (int)task->CONT.Vel);
		break;
	case Task_OVAR:
		snprintf(buffer, bufsize, "OVAR: S%d V%d I%d",
				 task->OVAR.Sec,
				 (int)task->OVAR.Vel,
				 (int)task->OVAR.Inc_Degree);
		break;
	case Task_VHIT:
		snprintf(buffer, bufsize, "VHIT: T%d %s",
				 (int)task->VHIT.Counter,
				 task->VHIT.IsCW ? "CW" : "CCW");
		break;
	case Task_TC:
		snprintf(buffer, bufsize, "TC: S%d V%d",
				 (int)task->TC.Counter,
				 (int)task->TC.Vel);
		break;
	default:
		snprintf(buffer, bufsize, "UNKNOWN: mode=%d", task->mode);
		break;
	}
}

/**
 * @brief Log task operation to USB ACM
 * @param operation: operation name (ADD, DEL, MOVE, SAVE)
 * @param index: task index
 * @param details: additional details (can be NULL)
 */
void HAL_USB_Log_Task_Operation(const char *operation, uint16_t index, const char *details)
{
#ifdef ATM32F43_xx
	if (!USB_init_flag)
		return;

	if (details != NULL && details[0] != '\0')
	{
		HAL_USB_Log_Printf("[TASK] %s index=%d, %s\r\n", operation, index, details);
	}
	else
	{
		HAL_USB_Log_Printf("[TASK] %s index=%d\r\n", operation, index);
	}
#endif
}

/**
 * @brief Log task operation with full task details to USB ACM
 * @param operation: operation name (ADD, DEL, MOVE, SAVE)
 * @param index: task index
 * @param task: pointer to task parameter structure (can be NULL)
 */
void HAL_USB_Log_Task_Detail(const char *operation, uint16_t index, void *task)
{
#ifdef ATM32F43_xx
	if (!USB_init_flag)
		return;

	if (task != NULL)
	{
		char task_info[128];
		Task_Info_ToString(task, task_info, sizeof(task_info));
		HAL_USB_Log_Printf("[TASK] %s index=%d, %s\r\n", operation, index, task_info);
	}
	else
	{
		HAL_USB_Log_Printf("[TASK] %s index=%d\r\n", operation, index);
	}
#endif
}

void HAL_USB_Log_Task_Detail_all()
{
#ifdef ATM32F43_xx
	if (!USB_init_flag)
		return;


		HAL_USB_Log_Printf("\r\n========== task Analysis ==========\r\n");

		char task_info[128];
		uint16_t cout = control_info.taskCount;
		for (int i = 0; i < cout; i++)
		{
			Task_Info_ToString((void *)&control_info.taskArray[i], task_info, sizeof(task_info));
			HAL_USB_Log_Printf("[TASK] %s index=%d, %s\r\n", "show", i, task_info);
			/* code */
		}
		HAL_USB_Log_Printf("\r\n========== task Analysis end ==========\r\n");
#endif
}

// ========== LVGL Memory Analysis Functions ==========

#ifdef ATM32F43_xx
#include "../INTERFACE/LVGL/lvgl/lvgl.h"
#include "FreeRTOS.h"
#include "task.h"

/**
 * @brief Analyze FreeRTOS heap status via USB
 */
void HAL_USB_Heap_Analysis(void)
{
	if (!USB_init_flag)
		return;

	HAL_USB_Log_Printf("\r\n========== FreeRTOS Heap (malloc) Analysis ==========\r\n");

	// Get heap statistics
	size_t free_heap = xPortGetFreeHeapSize();
	size_t min_free_heap = xPortGetMinimumEverFreeHeapSize();

	// Assuming total heap is 60KB (60 * 1024 = 61440)
	const size_t total_heap = 60 * 1024;
	size_t used_heap = total_heap - free_heap;

	uint32_t free_pct = (free_heap * 100) / total_heap;
	uint32_t used_pct = (used_heap * 100) / total_heap;

	HAL_USB_Log_Printf("\r\n[Current Status]\r\n");
	HAL_USB_Log_Printf("  Total Heap:      %6u bytes (%2u KB)\r\n", total_heap, total_heap / 1024);
	HAL_USB_Log_Printf("  Used (malloc):   %6u bytes (%2u KB, %3u%%)\r\n", used_heap, used_heap / 1024, used_pct);
	HAL_USB_Log_Printf("  Free:            %6u bytes (%2u KB, %3u%%)\r\n", free_heap, free_heap / 1024, free_pct);

	// Calculate peak usage
	size_t peak_used = total_heap - min_free_heap;
	uint32_t peak_pct = (peak_used * 100) / total_heap;

	HAL_USB_Log_Printf("\r\n[Historical Peak]\r\n");
	HAL_USB_Log_Printf("  Min Ever Free:   %6u bytes (%2u KB)\r\n", min_free_heap, min_free_heap / 1024);
	HAL_USB_Log_Printf("  Peak Used:       %6u bytes (%2u KB, %3u%%)\r\n", peak_used, peak_used / 1024, peak_pct);

	// Estimate allocation count (very rough)
	HAL_USB_Log_Printf("\r\n[Memory Pressure]\r\n");
	if (free_heap < 5000)
	{
		HAL_USB_Log_Printf("  Status:          CRITICAL - Less than 5KB free!\r\n");
	}
	else if (free_heap < 10000)
	{
		HAL_USB_Log_Printf("  Status:          WARNING - Less than 10KB free\r\n");
	}
	else if (free_heap < 15000)
	{
		HAL_USB_Log_Printf("  Status:          Caution - Less than 15KB free\r\n");
	}
	else
	{
		HAL_USB_Log_Printf("  Status:          OK - Sufficient free space\r\n");
	}

	// Calculate fragmentation risk
	uint32_t frag_risk = (peak_used > used_heap) ? ((peak_used - used_heap) * 100) / total_heap : 0;
	HAL_USB_Log_Printf("  Frag Risk:       %u%% (Peak-Current difference)\r\n", frag_risk);

	// Show allocation efficiency
	uint32_t alloc_efficiency = (used_heap * 100) / (peak_used > 0 ? peak_used : 1);
	HAL_USB_Log_Printf("  Alloc Efficiency: %u%% (Current/Peak)\r\n", alloc_efficiency);

	HAL_USB_Log_Printf("\r\n[Recommendations]\r\n");
	if (used_pct > 80)
	{
		HAL_USB_Log_Printf("  - Consider increasing heap size\r\n");
		HAL_USB_Log_Printf("  - Check for memory leaks\r\n");
	}
	else if (peak_pct > 90)
	{
		HAL_USB_Log_Printf("  - Peak usage very high! Monitor closely\r\n");
	}
	else if (frag_risk > 20)
	{
		HAL_USB_Log_Printf("  - High fragmentation risk detected\r\n");
		HAL_USB_Log_Printf("  - Consider using memory pools\r\n");
	}
	else
	{
		HAL_USB_Log_Printf("  - Heap usage healthy\r\n");
	}

	HAL_USB_Log_Printf("=====================================================\r\n\r\n");
}

/**
 * @brief Analyze all task stack usage via USB
 */
void HAL_USB_Stack_Analysis(void)
{
	if (!USB_init_flag)
		return;

	HAL_USB_Log_Printf("\r\n========== Task Stack Analysis ==========\r\n");

	UBaseType_t uxArraySize = uxTaskGetNumberOfTasks();
	TaskStatus_t *pxTaskStatusArray = (TaskStatus_t *)pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

	if (pxTaskStatusArray == NULL)
	{
		HAL_USB_Log_Printf("ERROR: Failed to allocate memory\r\n");
		return;
	}

	UBaseType_t uxTaskCount = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, NULL);

	HAL_USB_Log_Printf("%-20s %8s %8s %5s\r\n",
					   "Task Name", "Total", "Free", "Usage");
	HAL_USB_Log_Printf("-----------------------------------------------\r\n");

	uint32_t danger_count = 0;
	uint32_t warning_count = 0;

	for (UBaseType_t i = 0; i < uxTaskCount; i++)
	{
		TaskStatus_t *task = &pxTaskStatusArray[i];

		// Estimate total stack
		uint32_t stack_total_words = 1024;
		if (strstr(task->pcTaskName, "IDLE") != NULL)
			stack_total_words = 128;
		else if (strstr(task->pcTaskName, "Tmr") != NULL)
			stack_total_words = 256;

		uint32_t stack_free_words = task->usStackHighWaterMark;
		uint32_t stack_used_words = stack_total_words > stack_free_words ? (stack_total_words - stack_free_words) : 0;

		uint32_t stack_total_bytes = stack_total_words * 4;
		uint32_t stack_free_bytes = stack_free_words * 4;
		uint32_t usage_pct = stack_total_words > 0 ? (stack_used_words * 100) / stack_total_words : 0;

		// Status indicator
		char status = ' ';
		if (stack_free_words < 50)
		{
			status = 'X';
			danger_count++;
		}
		else if (stack_free_words < 128)
		{
			status = '!';
			warning_count++;
		}
		else
		{
			status = '+';
		}

		HAL_USB_Log_Printf("%c %-18s %8u %8u %4u%%\r\n",
						   status,
						   task->pcTaskName,
						   stack_total_bytes,
						   stack_free_bytes,
						   usage_pct);
	}

	HAL_USB_Log_Printf("-----------------------------------------------\r\n");

	if (danger_count > 0)
	{
		HAL_USB_Log_Printf("\r\nCRITICAL: %u task(s) with stack < 50 words!\r\n", danger_count);
	}
	else if (warning_count > 0)
	{
		HAL_USB_Log_Printf("\r\nWARNING: %u task(s) with stack < 128 words\r\n", warning_count);
	}
	else
	{
		HAL_USB_Log_Printf("\r\nAll task stacks: OK\r\n");
	}

	HAL_USB_Log_Printf("==========================================\r\n\r\n");

	vPortFree(pxTaskStatusArray);
}

/**
 * @brief Full memory check: LVGL + Heap + Stack
 */
void HAL_USB_Memory_Full_Check(void)
{
	if (!USB_init_flag)
		return;

	HAL_USB_Log_Printf("\r\n================================================\r\n");
	HAL_USB_Log_Printf("         FULL MEMORY HEALTH CHECK\r\n");
	HAL_USB_Log_Printf("================================================\r\n");

	// 1. LVGL Memory
	HAL_USB_LVGL_Memory_Analysis();

	// 2. FreeRTOS Heap
	HAL_USB_Heap_Analysis();

	// 3. Task Stacks
	HAL_USB_Stack_Analysis();

	HAL_USB_Log_Printf("================================================\r\n");
	HAL_USB_Log_Printf("         END OF MEMORY CHECK\r\n");
	HAL_USB_Log_Printf("================================================\r\n\r\n");
}

/**
 * @brief Diagnose task priority issues
 * Shows all tasks with their priorities and states
 */
void HAL_USB_Task_Priority_Diagnosis(void)
{
	if (!USB_init_flag)
		return;

	UBaseType_t uxArraySize = uxTaskGetNumberOfTasks();

	HAL_USB_Log_Printf("\r\n========== Task Priority Diagnosis ==========\r\n");
	HAL_USB_Log_Printf("Total Tasks: %u\r\n\r\n", uxArraySize);

	// Get task list
	TaskStatus_t *pxTaskStatusArray =
		(TaskStatus_t *)pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

	if (pxTaskStatusArray == NULL)
	{
		HAL_USB_Log_Printf("ERROR: Failed to allocate memory for task list\r\n");
		return;
	}

	// Get task details
	UBaseType_t uxTaskCount = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, NULL);

	// Sort by priority (descending)
	for (UBaseType_t i = 0; i < uxTaskCount - 1; i++)
	{
		for (UBaseType_t j = 0; j < uxTaskCount - i - 1; j++)
		{
			if (pxTaskStatusArray[j].uxCurrentPriority < pxTaskStatusArray[j + 1].uxCurrentPriority)
			{
				TaskStatus_t temp = pxTaskStatusArray[j];
				pxTaskStatusArray[j] = pxTaskStatusArray[j + 1];
				pxTaskStatusArray[j + 1] = temp;
			}
		}
	}

	// Print header
	HAL_USB_Log_Printf("Priority | Task Name            | State     | Stack Free\r\n");
	HAL_USB_Log_Printf("---------|----------------------|-----------|----------\r\n");

	const char *state_names[] = {"Running", "Ready", "Blocked", "Suspended", "Deleted"};

	for (UBaseType_t i = 0; i < uxTaskCount; i++)
	{
		TaskStatus_t *pxTaskStatus = &pxTaskStatusArray[i];

		const char *state = (pxTaskStatus->eCurrentState < 5)
								? state_names[pxTaskStatus->eCurrentState]
								: "Unknown";

		HAL_USB_Log_Printf("%8u | %-20s | %-9s | %u words\r\n",
						   pxTaskStatus->uxCurrentPriority,
						   pxTaskStatus->pcTaskName,
						   state,
						   pxTaskStatus->usStackHighWaterMark);
	}

	HAL_USB_Log_Printf("\r\n");
	HAL_USB_Log_Printf("DIAGNOSIS:\r\n");

	// Find highest priority task
	if (uxTaskCount > 0)
	{
		TaskStatus_t *highest = &pxTaskStatusArray[0];

		if (highest->eCurrentState == eRunning)
		{
			HAL_USB_Log_Printf("Running: %s (Priority %u)\r\n",
							   highest->pcTaskName, highest->uxCurrentPriority);

			if (highest->usStackHighWaterMark < 100)
			{
				HAL_USB_Log_Printf("  WARNING: Stack nearly full (%u words)!\r\n",
								   highest->usStackHighWaterMark);
			}
		}

		// Check for stuck high-priority tasks
		for (UBaseType_t i = 0; i < uxTaskCount; i++)
		{
			TaskStatus_t *pxTask = &pxTaskStatusArray[i];

			// If there are ready tasks but a higher priority one is stuck
			if (pxTask->eCurrentState == eReady &&
				pxTask->uxCurrentPriority > 2)
			{
				HAL_USB_Log_Printf("WARNING: High-priority task '%s' (Pri %u) is READY\r\n",
								   pxTask->pcTaskName, pxTask->uxCurrentPriority);
				HAL_USB_Log_Printf("  This task may not be getting CPU time!\r\n");
			}
		}
	}

	HAL_USB_Log_Printf("==========================================\r\n\r\n");

	vPortFree(pxTaskStatusArray);
}

/**
 * @brief Analyze and print LVGL memory status
 * Displays detailed memory usage including fragmentation info
 */
void HAL_USB_LVGL_Memory_Analysis(void)
{
	if (!USB_init_flag)
		return;

	lv_mem_monitor_t mon;
	lv_mem_monitor(&mon);

	uint32_t used_size = mon.total_size - mon.free_size;

	HAL_USB_Log_Printf("\r\n");
	HAL_USB_Log_Printf("============ LVGL Memory Analysis ============\r\n");
	HAL_USB_Log_Printf("Total Size:      %u bytes (%u KB)\r\n",
					   mon.total_size, mon.total_size / 1024);
	HAL_USB_Log_Printf("Used:            %u bytes (%u KB)\r\n",
					   used_size, used_size / 1024);
	HAL_USB_Log_Printf("Free:            %u bytes (%u KB)\r\n",
					   mon.free_size, mon.free_size / 1024);
	HAL_USB_Log_Printf("Max Used:        %u bytes (%u KB)\r\n",
					   mon.max_used, mon.max_used / 1024);
	HAL_USB_Log_Printf("Used Percent:    %u%%\r\n", mon.used_pct);
	HAL_USB_Log_Printf("Fragmentation:   %u%%\r\n", mon.frag_pct);
	HAL_USB_Log_Printf("Free Blocks:     %u\r\n", mon.free_cnt);
	HAL_USB_Log_Printf("Used Blocks:     %u\r\n", mon.used_cnt);
	HAL_USB_Log_Printf("Biggest Free:    %u bytes\r\n", mon.free_biggest_size);

	// Warning checks
	if (mon.frag_pct > 50)
	{
		HAL_USB_Log_Printf("WARNING: High fragmentation!\r\n");
	}
	if (mon.free_size < (mon.total_size / 10))
	{
		HAL_USB_Log_Printf("WARNING: Low free memory (<10%%)!\r\n");
	}
	if (mon.max_used > (mon.total_size * 9 / 10))
	{
		HAL_USB_Log_Printf("WARNING: Peak usage >90%%!\r\n");
	}

	HAL_USB_Log_Printf("==============================================\r\n");
	HAL_USB_Log_Printf("\r\n");
}

/**
 * @brief Process USB command input
 * @param cmd: command string from USB
 */
void HAL_USB_Command_Process(const char *cmd)
{
	if (!USB_init_flag)
		return;

	// Remove trailing whitespace and newline
	char cmd_buf[64];
	strncpy(cmd_buf, cmd, sizeof(cmd_buf) - 1);
	cmd_buf[sizeof(cmd_buf) - 1] = '\0';

	// Trim whitespace
	char *end = cmd_buf + strlen(cmd_buf) - 1;
	while (end > cmd_buf && (*end == ' ' || *end == '\r' || *end == '\n' || *end == '\t'))
	{
		*end = '\0';
		end--;
	}

	// Process commands
	if (strcmp(cmd_buf, "lvmem") == 0)
	{
		HAL_USB_LVGL_Memory_Analysis();
	}
	else if (strcmp(cmd_buf, "pri") == 0 || strcmp(cmd_buf, "priority") == 0 || strcmp(cmd_buf, "tasks") == 0)
	{
		HAL_USB_Task_Priority_Diagnosis();
	}
	else if (strcmp(cmd_buf, "heap") == 0 || strcmp(cmd_buf, "malloc") == 0)
	{
		HAL_USB_Heap_Analysis();
	}
	else if (strcmp(cmd_buf, "stack") == 0)
	{
		HAL_USB_Stack_Analysis();
	}
	else if (strcmp(cmd_buf, "alltask") == 0)
	{
		HAL_USB_Log_Task_Detail_all();
	}
	else if (strcmp(cmd_buf, "memcheck") == 0 || strcmp(cmd_buf, "mem") == 0)
	{
		HAL_USB_Memory_Full_Check();
	}
	else if (strcmp(cmd_buf, "help") == 0)
	{
		HAL_USB_Log_Printf("\r\nAvailable Commands:\r\n");
		HAL_USB_Log_Printf("  lvmem    - Show LVGL memory status\r\n");
		HAL_USB_Log_Printf("  heap     - Show FreeRTOS heap status\r\n");
		HAL_USB_Log_Printf("  malloc   - Show FreeRTOS heap status\r\n");
		HAL_USB_Log_Printf("  stack    - Show all task stack usage\r\n");
		HAL_USB_Log_Printf("  memcheck - Full memory check (heap+stack+lvgl)\r\n");
		HAL_USB_Log_Printf("  mem      - Full memory check (short)\r\n");
		HAL_USB_Log_Printf("  pri      - Show task priority & state\r\n");
		HAL_USB_Log_Printf("  tasks    - Show task priority & state\r\n");
		HAL_USB_Log_Printf("  help     - Show this help message\r\n");
		HAL_USB_Log_Printf("\r\n");
	}
	else if (strlen(cmd_buf) > 0)
	{
		HAL_USB_Log_Printf("Unknown command: '%s'\r\n", cmd_buf);
		HAL_USB_Log_Printf("Type 'help' for available commands.\r\n");
	}
}

// USB command buffer and state
static uint8_t usb_rx_buffer[64];
static uint8_t usb_cmd_buffer[64];
static uint8_t usb_cmd_idx = 0;

/**
 * @brief USB command polling handler - call this in main loop
 * Receives characters from USB and processes complete commands
 */
void HAL_USB_Command_Handler(void)
{
	if (!USB_init_flag)
		return;

	uint16_t rx_len = HAL_USB_ACM_Read(usb_rx_buffer, sizeof(usb_rx_buffer) - 1);
	if (rx_len > 0)
	{
		for (uint16_t i = 0; i < rx_len; i++)
		{
			uint8_t ch = usb_rx_buffer[i];

			// Echo character (optional - uncomment if needed)
			// HAL_USB_ACM_Send(&ch, 1);

			if (ch == '\r' || ch == '\n')
			{
				if (usb_cmd_idx > 0)
				{
					usb_cmd_buffer[usb_cmd_idx] = '\0';
					HAL_USB_Command_Process((char *)usb_cmd_buffer);
					usb_cmd_idx = 0;
				}
			}
			else if (ch == 0x08 || ch == 0x7F) // Backspace or Delete
			{
				if (usb_cmd_idx > 0)
				{
					usb_cmd_idx--;
				}
			}
			else if (ch >= 0x20 && ch < 0x7F) // Printable characters
			{
				if (usb_cmd_idx < sizeof(usb_cmd_buffer) - 1)
				{
					usb_cmd_buffer[usb_cmd_idx++] = ch;
				}
			}
		}
	}
}
#endif

#ifdef use_linux
void HAL_USB_Command_Process(const char *cmd)
{
	if(!USB_init_flag || cmd == NULL) return;
	if(strcmp(cmd, "help") == 0)
		HAL_USB_Log_Printf("Linux USB ACM is ready. MCU-only diagnostics are unavailable.\r\n");
	else if(cmd[0] != '\0')
		HAL_USB_Log_Printf("Linux USB command received: %s\r\n", cmd);
}

void HAL_USB_Command_Handler(void)
{
	static uint8_t command[64];
	static uint8_t index = 0;
	uint8_t received[64];
	uint16_t count = HAL_USB_ACM_Read(received, sizeof(received));
	for(uint16_t i = 0; i < count; ++i) {
		uint8_t ch = received[i];
		if(ch == '\r' || ch == '\n') {
			if(index > 0) {
				command[index] = '\0';
				HAL_USB_Command_Process((char *)command);
				index = 0;
			}
		}
		else if((ch == 0x08 || ch == 0x7f) && index > 0) --index;
		else if(ch >= 0x20 && ch < 0x7f && index < sizeof(command) - 1) command[index++] = ch;
	}
}
#endif

void HAL_communication_Init(void)
{
	Rk3588_Init();
	HAL_communication_Mpu_Init();
	HAL_communication_Bl_Init();
	HAL_BL_USART_INIT();
	HAL_USB_ACM_Init(); // Initialize USB ACM
}
