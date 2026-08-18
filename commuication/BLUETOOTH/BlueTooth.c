#include "BlueTooth.h"
#include "string.h"
#include "../../INTERFACE/UI/other/meassage_center.h"
#include "../../INTERFACE/UI/other/system_function.h"
#include "../../INTERFACE/UI/Task/task_manager.h"
#include "../../INTERFACE/UI/Task/task_info_struct.h"

extern uint8_t HAL_USB_TX(uint8_t* str, uint16_t len);

void (*SendCb)(uint8_t* buf, uint16_t len) = 0;
#define MsgRecBufLen (50)

typedef enum
{
	Cmd_error,
	Cmd_no_match,
	Cmd_match,
} DecodeFuntionReturn;
#if 0
DecodeFuntionReturn Decode_UploadTask_Cmd(uint8_t* bytes);
DecodeFuntionReturn Decode_downloadTask_Cmd(uint8_t* bytes);
DecodeFuntionReturn Decode_ReturnSlaveState_Cmd(uint8_t* bytes);
DecodeFuntionReturn Decode_Start_Cmd(uint8_t* bytes);
DecodeFuntionReturn Decode_Deleta_Cmd(uint8_t* bytes);
#define DecodingFuntion_Length 10
DecodeFuntionReturn(*decodingFuntion[DecodingFuntion_Length])(uint8_t* bytes) = {
	Decode_ReturnSlaveState_Cmd,
	Decode_Start_Cmd,
	Decode_UploadTask_Cmd,
	Decode_downloadTask_Cmd,
	Decode_Deleta_Cmd,
};
#else
#define DecodingFuntion_Length 10
DecodeFuntionReturn Decode_OtherCmd(uint8_t* bytes);
DecodeFuntionReturn(*decodingFuntion[DecodingFuntion_Length])(uint8_t* bytes) = {
	Decode_OtherCmd,
};
#endif

static uint8_t receiveBuf[MsgRecBufLen];
uint8_t BLisupdata;

void Task_Info_Package(Task_Parameter_Struct e, uint8_t id, uint8_t* buf)
{
	uint16_t temp;
	buf[0] = id;
	buf[1] = e.mode + 1;
	temp = e.VOR.Counter;
	buf[2] = ((uint16_t)temp >> 8);
	buf[3] = temp;
	temp = e.VOR.Vel;
	buf[4] = temp & 0xff;
	temp = e.VOR.Freq * 10;
	buf[5] = ((uint16_t)temp >> 8);
	buf[6] = (temp);
}

void ReturnBytes_Package(uint8_t* buf, uint16_t len)
{
	uint8_t sendbuf[11] = { 0 };
	sendbuf[0] = 0xfe;
	sendbuf[1] = 0xfd;
	sendbuf[10] = 0xfc;
	if (len > 9)
		len = 9;
	memcpy(&sendbuf[2], buf, len);
	if (SendCb)
		SendCb(sendbuf, 11);
}
#if 0
void Return_Handle()
{
	//static int size, isupdata = 0, currentCount;
	//if (Message_Center_Receive_Scanf("task", 1, 0,
	//	"ReadState %d %d", &size, &isupdata) == 2)
	//{
	//	uint8_t flag = 0, count = 0;;
	//	while (flag != 1 && count < 10)
	//	{
	//		count++;
	//		flag = Message_Center_Receive_Scanf("Ctrl", 1, 0,
	//			"ReadState %d", &currentCount);
	//		ControlDelay(5);
	//	}
	//	if (flag == 1)
	//	{
	//		uint8_t returnbytes[] = {
	//			(uint8_t)1,                                    // ID
	//			size,                                          // 当前任务总数
	//			(currentCount == 0xff) ? 0 : currentCount + 1, // 第几个在执行
	//			isupdata                                       // 是否更新
	//		};
	//		ReturnBytes_Package(returnbytes, sizeof(returnbytes));
	//		if (isupdata == 1)
	//		{
	//			uint8_t returnbytes_task[8];
	//			returnbytes_task[0] = 0x04;
	//			Task_Parameter_Struct task = { 0 };
	//			for (int i = 0; i < size; i++)
	//			{
	//				if (task_Manager_Get_Para(&task, i))
	//				{
	//					Task_Info_Package(task, i + 1, &returnbytes_task[1]);
	//				}
	//				ReturnBytes_Package(returnbytes_task, sizeof(returnbytes_task));
	//			}
	//		}
	//	}
	//}
	//if (Message_Center_Receive_Compare("Ctrl", 1, 0,
	//	"Strat") == 0)
	//{
	//	uint8_t returnbytes[] = {
	//		(uint8_t)2, // ID
	//		1,          // 成功启动
	//		0           //
	//	};
	//	ReturnBytes_Package(returnbytes, sizeof(returnbytes));
	//}
	//if (Message_Center_Receive_Compare("Ctrl", 1, 0,
	//	"Stop") == 0)
	//{
	//	uint8_t returnbytes[] = {
	//		(uint8_t)2, // 当前任务总数
	//		1,          // 命令成功
	//		0           // 是否更新
	//	};
	//	ReturnBytes_Package(returnbytes, sizeof(returnbytes));
	//}
	//if (Message_Center_Receive_Compare("Ctrl", 1, 0,
	//	"CmdError") == 0)
	//{
	//	uint8_t returnbytes[] = {
	//		(uint8_t)2, // 当前任务总数
	//		0,          // cmd fail
	//		0           // 是否更新
	//	};
	//	ReturnBytes_Package(returnbytes, sizeof(returnbytes));
	//}
}

// void Decode_downloadTask_Return(uint8_t ID)
// {
//     usart_data_send(ID);
// }

void ReturnSlaveState(void)
{
	const int mallocSize = 40;
	char* str = message_malloc(mallocSize);
	if (str == 0)
		return;
	memset(str, 0, mallocSize);
	int size = 0, isupdata = 0, currentCount = 0, isrunning = 0;
	Message_Center_Read_prinft("task", str, mallocSize, "ReqReadState");
	if (sscanf(str, "ReadState %d %d", &size, &isupdata) == 2);
	{
		memset(str, 0, mallocSize);
		Message_Center_Read_prinft("Ctrl", str, mallocSize, "ReqReadState");
		if (sscanf(str, "ReadState %d %d", &isrunning, &currentCount) == 2)
		{
			currentCount = (isrunning) ? currentCount + 1 : 0;
			uint8_t returnbytes[] = {
							(uint8_t)1,                                    // ID
							size,                                          // 当前任务总数
							 currentCount , // 第几个在执行
							isupdata                                       // 是否更新
			};
			ReturnBytes_Package(returnbytes, sizeof(returnbytes));
		}
	}
	free(str);
}

DecodeFuntionReturn Decode_ReturnSlaveState_Cmd(uint8_t* bytes)
{
	const uint8_t ModeId = 0x01;
	// uint8_t res = 0;
	typedef struct
	{
		uint8_t Task_Count;
		uint8_t Runing_ID;
	} state_parameter; // 注意不能直接使用内存赋值 要逐个赋值 涉及内存分配问题

	if (bytes == 0)
		return Cmd_error;
	if (bytes[0] != ModeId)
		return Cmd_no_match;
	//match
	ReturnSlaveState();
	return Cmd_match;
}

void ReturnStart_Cmd(uint8_t isstart)
{
	const char* stratcmd = "ReqStrat";
	const char* StopCmd = "ReqStop";
	const char* cmdpt = 0;

	const int mallocSize = 10;
	char* str = message_malloc(mallocSize);
	if (str == 0)
		return;
	Message_Center_Send_prinft("Ctrl", 0, 0,
		(isstart == 1) ?
		stratcmd : StopCmd);
	ControlDelay(50);

	Message_Center_Read_prinft("Ctrl", str, mallocSize,
		"run_State?");
	if ((Msg_COMPARE("isStart", str) && isstart) ||
		(Msg_COMPARE("isStop", str) && isstart == 0))
	{
		uint8_t returnbytes[] = {
					(uint8_t)2, // ID
					1,          // 成功启动
					0           //
		};
		ReturnBytes_Package(returnbytes, sizeof(returnbytes));
	}
	else
	{
		uint8_t returnbytes[] = {
					(uint8_t)2, // ID
					0,          // 成功启动
					0           //
		};
		ReturnBytes_Package(returnbytes, sizeof(returnbytes));
	}

}

DecodeFuntionReturn Decode_Start_Cmd(uint8_t* bytes)
{

	const uint8_t ModeId = 0x02;
	typedef struct
	{
		uint8_t start;
	} Start_parameter;
	if (bytes == 0)
		return Cmd_error;
	if (bytes[0] != ModeId)
		return Cmd_no_match;
	//match
	Start_parameter e = {
		.start = bytes[1] };
	ReturnStart_Cmd((e.start == 1) ? 1 : 0);

	return Cmd_match;
}

void Return_downloadTask_Cmd(uint8_t id)
{
	/*const int mallocSize = sizeof(Task_Parameter_Struct);
	Task_Parameter_Struct* para = message_malloc(mallocSize);
	if (para == 0)
		return;
	memset(para, 0, mallocSize);
	para->mode = 0xff;*/
	/*Message_Center_Read_prinft("Ctrl", para, mallocSize,
		"GetTaskPara %d", id);
	if (para->mode != 0xff) {

	}*/
	ControlDelay(20);
	Task_Parameter_Struct task;
	uint8_t returnbytes[8] = { 0 };
	returnbytes[0] = 0x3;
	if (task_Manager_Get_Para(&task, id))
	{
		Task_Info_Package(task, id + 1, &returnbytes[1]);
		ReturnBytes_Package(returnbytes, sizeof(returnbytes));
	}
	else
	{
		ReturnBytes_Package(returnbytes, sizeof(returnbytes));
	}
}

DecodeFuntionReturn Decode_downloadTask_Cmd(uint8_t* bytes)
{
	Task_Parameter_Struct task;
	const uint8_t ModeId = 0x03;
	typedef struct
	{
		uint8_t id;
		uint8_t mode;
		uint16_t sec_times;
		uint8_t velocity;
		uint16_t freq_X10;
	} Download_parameter; // 注意不能直接使用内存赋值 要逐个赋值 涉及内存分配问题

	if (bytes == 0)
		return Cmd_error;
	if (bytes[0] != ModeId)
		return Cmd_no_match;
	uint8_t ID = bytes[1] - 1;
	uint16_t Freq = (((uint16_t)bytes[6]) * 256 + (uint16_t)bytes[7]);
	task.mode = bytes[2] - 1;
	task.VOR.Counter = ((uint16_t)bytes[3]) * 256 + (uint16_t)bytes[4];
	task.VOR.Vel = bytes[5];
	task.VOR.Freq = Freq / 10.0f;

	uint32_t handleID = 0;
#ifndef HARDWARE_TEST
	Task_manager_Begin_Req(&handleID);
	Task_manager_Req_saveCreate(handleID, ID, task);
	Task_manager_End_release(handleID);
#endif
	Return_downloadTask_Cmd(ID);
	return Cmd_match;
}

DecodeFuntionReturn Decode_UploadTask_Cmd(uint8_t* bytes)
{
	const uint8_t ModeId = 0x04;
	if (bytes == 0)
		return Cmd_error;
	if (bytes[0] != ModeId)
		return Cmd_no_match;
	uint8_t ReadID = bytes[1] - 1;
	Task_Parameter_Struct task;
	uint8_t buf[8] = { 0 };
	buf[0] = 0x04;
#ifndef HARDWARE_TEST
	OutputDebugPrintf("\r\n[ss] %d", ReadID);
	if (task_Manager_Get_Para(&task, ReadID))
	{
		Task_Info_Package(task, ReadID + 1, &buf[1]);
	}
#endif
#ifndef ATM32F43_xx
	uint32_t id = GetCurrentThreadId();
	//OutputDebugPrintf("\r\nID %ld\r\n", id);
#endif
	ReturnBytes_Package(buf, sizeof(buf));
	return Cmd_match;
}

extern short Table_Choose;

DecodeFuntionReturn Decode_Deleta_Cmd(uint8_t* bytes)
{
	uint32_t handleID;
	const uint8_t ModeId = 0x05;
	if (bytes == 0)
		return Cmd_error;
	if (bytes[0] != ModeId)
		return Cmd_no_match;
	typedef struct
	{
		uint8_t Id;
	} Delate_parameter;
	Task_Parameter_Struct  task;
	Delate_parameter e = {
		.Id = bytes[1] - 1 };
	uint8_t buf[] = {
		0x05,
		0x00,
	};
#ifndef HARDWARE_TEST
	Task_manager_Begin_Req(&handleID);
	if (Task_manager_Req_Del(handleID, e.Id) == 0)
		buf[1] = 0x01;
	Task_manager_End_release(handleID);
#endif
	ReturnBytes_Package(buf, sizeof(buf));
	return Cmd_match;
}
#endif

DecodeFuntionReturn Decode_OtherCmd(uint8_t* msg)
{
	char* token;
	char Port[10];
	uint8_t RWdir = 0;//0:R,1:W
	token = strtok(msg, "$"); //check port Name
	if (token == NULL)
		return Cmd_error;
	strcpy(Port, token);
	token = strtok(NULL, "$");//check R/W direction
	if (token == NULL)
		return Cmd_error;
	RWdir = (token[0] == 'R') ? 0 : 1;
	token = strtok(NULL, "$"); //Get Msg
	if (token == NULL)
		return Cmd_error;

	const int RxMsgBufSize = 50;
	char* RxMsg =(void *) malloc(RxMsgBufSize);
	memset(RxMsg, 0, RxMsgBufSize);
	const char errorMsg[] = "errorMsg\r\n";
	const char okMsg[] = "W OK\r\n";
	memcpy(RxMsg, okMsg, sizeof(okMsg));
	uint8_t Nack = 0;

	if (RWdir == 1) //Write stage
	{
		Message_Center_Send_prinft(Port, 0, 0,
			"%s", token);

	}
	else
	{
		Nack = Message_Center_Read_prinft(Port, RxMsg, RxMsgBufSize,
			"%s", token);
	}
	uint16_t RxMsgLen = strlen(RxMsg);
	memmove(&RxMsg[1], &RxMsg[0], RxMsgLen);
	RxMsg[0] = '(';
	RxMsgLen = strlen(RxMsg);
	if (RxMsgLen <= RxMsgBufSize - 2)
		memcpy(&RxMsg[RxMsgLen], ")", 1);
	SendCb(RxMsg, strlen(RxMsg));
	free(RxMsg);
}

uint8_t BLctrl_Vor_Para_pkg(char* msg, Task_Parameter_Struct* par)
{
	extern uint8_t VOR_exam_Para_Check(Task_Parameter_Struct * e);
	par->mode = Task_VOR;
	uint8_t res = sscanf(msg, "vor,%d,%f,%f",
		&par->VOR.Counter, &par->VOR.Vel, &par->VOR.Freq);
	if (VOR_exam_Para_Check(par) == 0)
		return 0;
	if (res == 3)
		return 1;
	else
		return 0;
}

uint8_t BLctrl_CNT_Vor_Para_pkg(char* msg, Task_Parameter_Struct* par)
{
	extern uint8_t Cont_exam_Para_Check(Task_Parameter_Struct * e);
	par->mode = Task_Continue;
	uint8_t res = sscanf(msg, "cont,%d,%f",
		&par->CONT.Sec, &par->CONT.Vel);
	if (Cont_exam_Para_Check(par) == 0)
		return 0;
	if (res == 2)
		return 1;
	else
		return 0;
}

uint8_t BLctrl_OVAR_Para_pkg(char* msg, Task_Parameter_Struct* par)
{
	/*char* token = strtok(msg, ',');*/
	extern uint8_t Ovar_exam_Para_Check(Task_Parameter_Struct * e);
	par->mode = Task_OVAR;
	uint8_t res = sscanf(msg, "ovar,%d,%f,%f",
		&par->OVAR.Sec, &par->OVAR.Vel, &par->OVAR.Inc_Degree);
	if (Ovar_exam_Para_Check(par) == 0)
		return 0;
	if (res == 3)
		return 1;
	else
		return 0;
}

void BLctrl_Check_task_Msg(char* msg)
{
	int id;
	char mode[10] = { 0 };
	uint8_t tokenCount = 1;
	char* token = strstr(msg, ",");
	token++;
	Task_Parameter_Struct  par = { 0 };
	while (token != NULL)
	{
		if (tokenCount == 1)
		{
			uint8_t res = 0;
			if (Msg_COMPARE("vor", token))
				res = BLctrl_Vor_Para_pkg(token, &par);
			if (Msg_COMPARE("cont", token))
				res = BLctrl_CNT_Vor_Para_pkg(token, &par);
			if (Msg_COMPARE("ovar", token))
				res = BLctrl_OVAR_Para_pkg(token, &par);
			if (res == 1)
			{
				int handleID;
				Task_manager_Begin_Req(&handleID);
				Task_manager_Req_append(handleID, par);
				Task_manager_End_release(handleID);
				break;
			}
		}
		token = strstr(token, ",");
		tokenCount++;
	}

}

void BLctrl_Set_Task_Size_Msg()
{
	int handleID;
	Task_manager_Begin_Req(&handleID);
	Task_manager_Req_Set_Size(handleID);
	Task_manager_End_release(handleID);
}

uint8_t BLctrl_Read_Handle(uint8_t* msg, uint16_t msg_size,
	uint8_t* src, uint16_t SrcSize)
{
	if (Msg_COMPARE("addtask,", msg))
	{
		BLctrl_Check_task_Msg(msg);
		//ControlDelay(1);
		const char str1[] = "addtask,OK";
		memcpy(src, str1, strlen(str1));
		return 0;
	}
	if (Msg_COMPARE("delAll", msg))
	{
		BLctrl_Set_Task_Size_Msg();
		//ControlDelay(50);
		const char str1[] = "delAll,OK";
		memcpy(src, str1, strlen(str1));
		return 0;
	}
	return 1;
}

//搭建BLinterFace 的接口 负责 任务添加和修改

void usart_protocol_InterruptHandle(uint8_t dr) // 串口1中断服务程序
{
	const uint8_t match_head[] = { 0xfe, 0xfd };
	const uint8_t match_tail = 0xfc;
	static uint8_t bytes[MsgRecBufLen];
	static uint16_t current_Index;
	if (BLisupdata)
		return;
	switch (current_Index)
	{
	case 0:
		if (dr == '(')
		{
			//bytes[current_Index] = dr;
			current_Index++;
		}
		break;
	default:
		if (current_Index == sizeof(bytes))
		{
			current_Index = 0;
			return;
		}
		if (dr == ')')
		{
			//bytes[current_Index] = dr;
			memcpy(receiveBuf, &bytes[1], current_Index);
			current_Index = 0;
			BLisupdata = 1;
		}
		else
		{
			bytes[current_Index++] = dr;
		}
		break;
	}
}

void usart_protocol_decoding(uint8_t* bytes)
{
	//Return_Handle();
	if (BLisupdata == 0)
		return;
#if 1
	uint8_t* buf = receiveBuf;
	//    HAL_USB_TX(buf, 11);
#else
	uint8_t* buf = &bytes[2];
#endif
	for (int i = 0; i < DecodingFuntion_Length; i++)
	{
		if (decodingFuntion[i] != 0)
		{
			DecodeFuntionReturn e = (decodingFuntion[i])(buf);
			if (e == Cmd_match)
			{
			}
		}
	}
	BLisupdata = 0;
}

void usart_protocol_init(void (*cb)(uint8_t* buf, uint16_t len))
{
	BLisupdata = 0;
	SendCb = cb;
	Meassage_Center_Add("BLctrl");
	Message_Center_Add_Read_CB("BLctrl", BLctrl_Read_Handle);

}