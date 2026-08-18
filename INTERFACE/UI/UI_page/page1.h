/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @Date: 2024-08-05 14:28:30
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-04-25 09:47:06
 * @FilePath: \USERd:\workfile\项目3 vor\software\VOR_V2.0\INTERFACE\UI\UI_page\page1.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef  __page1_H_
#define __page1_H_
#include "../UI_Include.h"
#ifdef __cplusplus
extern "C"
{
#endif //
#pragma anon_unions
	void Page1_init(lv_obj_t* parent);
	char* UI_Page1_Get_Msg_Array(uint16_t* len);

	typedef struct
	{
		union
		{
			uint16_t flag;
			struct
			{
				uint8_t H_init : 1;//hardware init
				uint8_t isRun : 1;
				uint8_t WaitRk : 1;
				uint8_t IsRunning : 1;
				uint8_t Exit : 1;
				uint8_t VHIT_Next : 1;
				uint8_t finish : 1;
				uint8_t reqPause : 1;
				uint8_t isPause : 1;
				uint8_t powerUp : 2;
			};

		};
		union
		{
			uint16_t ui_para;
		};
	} State_Bit;
#ifdef __cplusplus
}
#endif // 

#endif // ! UI_Paremeter_Set_H_
