#include <SDL2/SDL.h>

extern "C" {
#include "lvgl.h"
#include "lv_drivers/sdl/sdl.h"
#include "../../INTERFACE/UI/control/control.h"
#include "../../INTERFACE/UI/UI_page/PageManagment.h"
}

static constexpr lv_coord_t kHorizontalResolution = 800;
static constexpr lv_coord_t kVerticalResolution = 480;

int main()
{
    lv_init();
    sdl_init();

    static lv_color_t draw_buffer_memory[kHorizontalResolution * 100];
    static lv_disp_draw_buf_t draw_buffer;
    lv_disp_draw_buf_init(&draw_buffer, draw_buffer_memory, nullptr, kHorizontalResolution * 100);

    static lv_disp_drv_t display_driver;
    lv_disp_drv_init(&display_driver);
    display_driver.draw_buf = &draw_buffer;
    display_driver.flush_cb = sdl_display_flush;
    display_driver.hor_res = kHorizontalResolution;
    display_driver.ver_res = kVerticalResolution;
    lv_disp_drv_register(&display_driver);

    static lv_indev_drv_t pointer_driver;
    lv_indev_drv_init(&pointer_driver);
    pointer_driver.type = LV_INDEV_TYPE_POINTER;
    pointer_driver.read_cb = sdl_mouse_read;
    lv_indev_drv_register(&pointer_driver);

    controlInit();
    UI_Page_Management_Init();

    while(!sdl_quit_qry) {
        lv_timer_handler();
        SDL_Delay(1);
    }

    SDL_Quit();
    return 0;
}
