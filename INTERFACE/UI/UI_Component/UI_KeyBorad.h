/*
 * @Author: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @Date: 2025-09-11 15:50:16
 * @LastEditors: pzzhh2 101804901+Pzzhh@users.noreply.github.com
 * @LastEditTime: 2025-10-11 11:12:44
 * @FilePath: \VOR_V3.0_AT32\INTERFACE\UI\UI_Component\UI_KeyBorad.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef  __UI_KeyBorad_H_
#define __UI_KeyBorad_H_
#include "../UI_Include.h"
typedef enum 
{
	NumberKey,
	NumberChar,
}UI_keyBorad_Typed;

lv_obj_t* create_custom_keyboard(lv_obj_t* parent, int typed);
void del_custom_keyboard(void);
lv_obj_t *get_custom_keyboard(void);

#endif // ! __UI_Btn_H_
