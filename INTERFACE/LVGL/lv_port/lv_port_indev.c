/**
 * @file lv_port_indev.c
 *
 */

/*Copy this file as "lv_port_indev.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_indev.h"
#include "ft5206.h"
#include "gt9147.h"
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);

u8 Touch_active_flag = 1;

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_indev_init(void)
{
    static lv_indev_drv_t indev_drv;

    /*------------------
     * Touchpad
     * -----------------*/

    /*Register a touchpad input device*/
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
void touch_sever(unsigned char mode, unsigned short Pos_XY[3])
{
    static unsigned short Pos_data[3];
    static unsigned char touch_iic_data[4];
    if (mode == 1)
    {
        if (GT9147_Online())
        {
            GT9147_GET_Touch(Pos_data);
        }
        else
        {

            FT5206_RD_Reg(0X02, touch_iic_data, 1);
            // Usart1_printf("\r\n %x",touch_iic_data[0]);
            if (touch_iic_data[0] < 0X05 && touch_iic_data[0] != 0X0 && Touch_active_flag)
            {
                FT5206_RD_Reg(0X03, touch_iic_data, 4);
                Pos_data[0] = touch_iic_data[0] & 0X7F;
                Pos_data[0] = Pos_data[0] * 256 + touch_iic_data[1];
                Pos_data[1] = touch_iic_data[2] & 0X0F;
                Pos_data[1] = Pos_data[1] * 256 + touch_iic_data[3];
                Pos_data[2] = 1;
            }
            else
            {
                Pos_data[0] = 0XFFFF;
                Pos_data[1] = 0XFFFF;
                Pos_data[2] = false;
                // printf("\r\n                      ,NO");
            }
        }
    }
    else
    {
        Pos_XY[0] = Pos_data[0]; // y
        Pos_XY[1] = Pos_data[1];
        Pos_XY[2] = Pos_data[2];
    }
}

/* Will be called by the library to read the touchpad */

static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    static uint16_t last_x = 0;
    static uint16_t last_y = 0;
    static uint8_t count = 0;
    static u16 Pos_XY[3];
    touch_sever(0, Pos_XY);
    if (Pos_XY[2] == 1) //?????
    {
        if (count < 5)
            count++;
        // printf("\r\n touch");
        if (count > 1 || last_x != Pos_XY[1] || last_y != Pos_XY[0])
        {
            last_x = Pos_XY[1];
            last_y = Pos_XY[0];
            data->point.x = last_x;
            data->point.y = last_y;
            data->state = LV_INDEV_STATE_PR;
        }
    }
    else
    {
        if (count > 0)
            count--;
        data->point.x = last_x;
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_REL;
    }
    /*Return `false` because we are not buffering and no more data to read*/
    return;
}
// static bool touchpad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
// {
// 		static uint16_t last_x = 0;
// 		static uint16_t last_y = 0;
//         static u8 touch_iic_data[4];
//         static u16 pos[2];
//         static u8 time;
//         time++;
//         if(1)
//         {
//             FT5206_RD_Reg(0X02, touch_iic_data, 1);
//             if (touch_iic_data[0] < 0X05 && touch_iic_data[0] != 0X0 && Touch_active_flag)
//             {
//                  printf("\r\n                  Touch:%d",touch_iic_data[0]);
//                 FT5206_RD_Reg(0X03, touch_iic_data, 4);
//                 pos[0] = touch_iic_data[0] & 0X7F;
//                 pos[0] = pos[0] * 256 + touch_iic_data[1];
//                 pos[1] = touch_iic_data[2] & 0X0F;
//                 pos[1] = pos[1] * 256 + touch_iic_data[3];
//                 //  if (last_x ==480- pos[0])
//                 //  {
//                 //     data->state = LV_INDEV_STATE_REL;
//                 //     printf("\r\n x, %d, Y %d  LONG",last_x,last_y);
//                 //  }
//                 //  else
//                  data->state = LV_INDEV_STATE_PR;

//                 last_x = pos[0];
//                 last_y = pos[1];
//                 data->point.x = last_y;
//                 data->point.y = last_x;

//             }
//             else
//             {
//                 data->point.x = 0XFFFF;
//                 data->point.y = 0XFFFF;
//                 data->state = LV_INDEV_STATE_REL;
//                 return false;
//                 // printf("\r\n                      ,NO");
//             }
//             time=0;
//         }
//         // if(tp_dev.sta&TP_PRES_DOWN)//´¥Ãþ°´ÏÂÁË
// 		// {
// 		// 	last_x = tp_dev.x[0];
// 		// 	last_y = tp_dev.y[0];
// 		// 	data->point.x = last_x;
// 		// 	data->point.y = last_y;
// 		// 	data->state = LV_INDEV_STATE_PR;
// 		// }else{
// 		// 	data->point.x = last_x;
// 		// 	data->point.y = last_y;
// 		// 	data->state = LV_INDEV_STATE_REL;
// 		// }
//     /*Return `false` because we are not buffering and no more data to read*/
//     return true;
// }

#else /* Enable this file at the top */

/* This dummy typedef exists purely to silence -Wpedantic. */
typedef int keep_pedantic_happy;
#endif
