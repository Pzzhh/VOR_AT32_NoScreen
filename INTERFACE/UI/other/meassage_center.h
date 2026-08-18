#ifndef __Meassage_Center_H_
#define __Meassage_Center_H_

#include "stdint.h"
#include "string.h"
#include "stdio.h"

#ifdef __cplusplus
extern "C"
{
#endif //
#define Msg_COMPARE(a,b) (strncmp(a,b,sizeof(a)-1)==0) //Ç°±ØÐëÊÇconst×Ö·û´®

	typedef enum 
	{
		msg_match = 0,
		msg_nomatch
	}MsgReadReturn;

	typedef  uint8_t(*function_cb)(uint8_t* msg, uint16_t msg_size,
		uint8_t* src, uint16_t SrcSize);
	uint8_t Message_Center_Add_Send_CB(
		const char* name,
		function_cb cb
	);

	void message_free(void* pt);
	void* message_malloc(size_t size);

	uint8_t Message_Center_Add_Read_CB(
		const char* name,
		function_cb cb
	);

	uint8_t Message_Center_Read_prinft(
		const char* name,
		void* Src,
		uint8_t SrcSize,
		const char* format,
		...);

	uint8_t Message_Center_Send_prinft(
		const char* name,
		void* Src,
		uint8_t SrcSize,
		const char* format,
		...);

	uint16_t  Message_Center_Get_Str_Len(
		const char* str,
		uint16_t maxlen);

	uint8_t Message_Center_Clean_Flag(
		const char* name,
		uint16_t fun_id);

	//uint8_t Message_Center_Send(
	//	const char* name,
	//	uint16_t fun_id,
	//	void* source,
	//	char* msg,
	//	uint16_t msgLen);
	//uint8_t Message_Center_Send_prinft(
	//	const char* name,
	//	uint8_t* Src,
	//	uint8_t SrcSize,
	//	const char* format,
	//	...);

	//uint8_t Message_Center_Send_prinft_OverWrite(
	//	const char* name,
	//	uint16_t fun_id,
	//	void* source,
	//	const char* format,
	//	...);

	//uint8_t Message_Center_Get(
	//	const char* name,
	//	uint16_t fun_id,
	//	void** source,
	//	char* msg,
	//	uint16_t msgLen);

	//uint8_t Message_Center_Clean_Flag(
	//	const char* name,
	//	uint16_t fun_id);

	//uint8_t Message_Center_Receive_Scanf(
	//	const char* name,
	//	uint16_t FilterId,
	//	void** source,
	//	const char* format,
	//	...);

	////0 match
	//uint8_t Message_Center_Receive_Compare(
	//	const char* name,
	//	uint16_t FilterId,
	//	void** source,
	//	const char* str1);

	void Meassage_Center_Add(const char* name);
	void Message_Center_Init();


#ifdef __cplusplus
}
#endif //


#endif