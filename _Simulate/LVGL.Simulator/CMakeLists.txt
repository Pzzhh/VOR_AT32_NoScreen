cmake_minimum_required(VERSION 3.16)
project(lvgl_simulator_linux LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)
find_package(Threads REQUIRED)

set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE STRING "" FORCE)
set(LV_CONF_INCLUDE_SIMPLE ON CACHE BOOL "" FORCE)
set(LV_LVGL_H_INCLUDE_SIMPLE ON CACHE BOOL "" FORCE)
add_subdirectory(lvgl EXCLUDE_FROM_ALL)
target_compile_definitions(lvgl PUBLIC LV_PLATFORM_LINUX=1 use_linux=1)
target_include_directories(lvgl BEFORE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/linux")

set(APP_SOURCES
    linux/lvgl_linux_main.cpp
    linux/linux_serial.c
    lv_drivers/sdl/sdl.c
    lv_drivers/sdl/sdl_common.c
    ../../commuication/communication.c
    ../../commuication/communication_API.c
    ../../commuication/usb_vor_control.c
    ../../commuication/usb_control_base.c
    ../../commuication/rk3588_uart.c
    ../../commuication/BLUETOOTH/BlueTooth.c
    ../../commuication/mpu/jy60.c
    ../../INTERFACE/UI/control/control.c
    ../../INTERFACE/UI/control/control_base.c
    ../../INTERFACE/UI/control/control_CONT.c
    ../../INTERFACE/UI/control/control_Hardware_API.c
    ../../INTERFACE/UI/control/control_OVAR.c
    ../../INTERFACE/UI/control/control_TC.c
    ../../INTERFACE/UI/control/control_VHIT.c
    ../../INTERFACE/UI/control/control_VOR.c
    ../../INTERFACE/UI/control/control_ZERO.c
    ../../INTERFACE/UI/control/task_control.c
    ../../INTERFACE/UI/other/list.c
    ../../INTERFACE/UI/other/meassage_center.c
    ../../INTERFACE/UI/other/system_function.c
    ../../INTERFACE/UI/Task/task_manager.c
    ../../INTERFACE/UI/Task/task_stroage.c
    ../../INTERFACE/UI/UI_Component/UI_Btn.c
    ../../INTERFACE/UI/UI_Component/UI_Dropdown.c
    ../../INTERFACE/UI/UI_Component/UI_KeyBorad.c
    ../../INTERFACE/UI/UI_Component/UI_ListBox.c
    ../../INTERFACE/UI/UI_Component/UI_SpinBox.c
    ../../INTERFACE/UI/UI_page/page1.c
    ../../INTERFACE/UI/UI_page/PageManagment.c
    ../../INTERFACE/UI/UI_page/Page_Maintain.c
    ../../INTERFACE/UI/UI_page/Page_Outage_Event.c
    ../../INTERFACE/UI/UI_page/Page_Varify.c
    ../../INTERFACE/UI/UI_Parameter/UI_Parameter_Cont.c
    ../../INTERFACE/UI/UI_Parameter/UI_Parameter_Ovar.c
    ../../INTERFACE/UI/UI_Parameter/UI_Parameter_Set.c
    ../../INTERFACE/UI/UI_Parameter/UI_Parameter_TC.c
    ../../INTERFACE/UI/UI_Parameter/UI_Parameter_VHIT.c
    ../../INTERFACE/UI/UI_Parameter/UI_Parameter_Vor.c
    ../../INTERFACE/UI/varify/verfiy.c
    GUI/arrow_rotate_right.c
    GUI/GUI_BackGround.c
)

add_executable(lvgl_simulator ${APP_SOURCES})
target_compile_definitions(lvgl_simulator PRIVATE
    LV_PLATFORM_LINUX=1
    use_linux=1
    LV_CONF_INCLUDE_SIMPLE=1
    LV_LVGL_H_INCLUDE_SIMPLE=1
)
target_include_directories(lvgl_simulator PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/linux"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/lvgl"
    "${CMAKE_CURRENT_SOURCE_DIR}/lv_drivers"
    "${CMAKE_CURRENT_SOURCE_DIR}/../.."
    "${CMAKE_CURRENT_SOURCE_DIR}/../../INTERFACE"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../INTERFACE/UI"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../INTERFACE/UI/control"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../INTERFACE/UI/other"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../INTERFACE/UI/Task"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../INTERFACE/UI/UI_Component"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../INTERFACE/UI/UI_page"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../INTERFACE/UI/UI_Parameter"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../INTERFACE/UI/varify"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../commuication"
)
target_link_libraries(lvgl_simulator PRIVATE lvgl::lvgl SDL2::SDL2 Threads::Threads m)
target_compile_options(lvgl_simulator PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:-Wall;-Wextra;-Wno-unused-parameter>)
