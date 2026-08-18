#ifndef  __UI_ListBox_H_
#define __UI_ListBox_H_
#include "../UI_Include.h"
#ifdef __cplusplus
extern "C"
{
#endif //
#include "../other/list.h"

	typedef struct
	{
		lv_obj_t* obj;
		lv_style_t style;
		float set_pos;
		float pos_y;
		int32_t set_clearence;
		const int* ID;
		lv_anim_t* anim;

		uint32_t last_tick;
		void (*updata_cd)(void* table_Property);
		void* Updata_Source;
		uint8_t isremove;
	}Table_Property;

	typedef struct
	{
		lv_obj_t* obj;
		List* list;
		uint8_t hightLight_index;
	}Table_Contain_Property;

	Table_Contain_Property* UI_ListBox_Create(lv_obj_t* parent);
	Table_Property* UI_Table_Get_Property(lv_obj_t* obj);
	void UI_Table_Set_Property(lv_obj_t* obj, void* src);
	lv_obj_t* UI_Table_Create(Table_Contain_Property* table_Contain_Property, void* updata_cd, uint16_t insert_index);
	void UI_Table_Clean(List* head, int index, lv_obj_t* parent_);
	int UI_Table_Find_Size(Table_Contain_Property* head);

	int UI_Table_Get_Fouces(Table_Contain_Property* head);
	void UI_Table_Set_Fouces(Table_Contain_Property* table_Contain_Property, int index);

	void UI_Table_Move(Table_Contain_Property* table_Contain_Property, int fromIndex, int toIndex);
	Table_Property* UI_Table_Find_Obj_User_Data(Table_Contain_Property* table_Contain_Property, int index);

#ifdef __cplusplus
}
#endif // 

#endif // ! UI_Paremeter_Set_H_
