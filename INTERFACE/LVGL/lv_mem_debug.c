/**
 * @file lv_mem_debug.c
 * @brief LVGL 内存调试工具
 */

#include "lv_mem_check.h"
#include "../commuication/communication_API.h"
#include <stdio.h>

/**
 * @brief 测试内存分配并检查地址
 */
void lv_mem_debug_test_allocation(void)
{
    HAL_USB_Log_Printf("\r\n========== LVGL Memory Debug Test ==========\r\n");
    
    // 1. 打印内存池基本信息
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    HAL_USB_Log_Printf("[MEM] Pool: 0x%08X - 0x%08X (%u bytes)\r\n", 
                        LVGL_MEM_POOL_START, LVGL_MEM_POOL_END, 
                        LVGL_MEM_POOL_SIZE);
    HAL_USB_Log_Printf("[MEM] Used=%u Free=%u Frag=%u%%\r\n", 
                        mon.used_size, mon.free_size, mon.frag_pct);
    
    // 2. 测试分配
    void* test_ptr = lv_mem_alloc(128);
    if (test_ptr) {
        uintptr_t addr = (uintptr_t)test_ptr;
        uint32_t offset = lv_mem_get_offset(test_ptr);
        bool in_pool = lv_mem_is_in_pool(test_ptr);
        
        HAL_USB_Log_Printf("[TEST] Allocated 128 bytes\r\n");
        HAL_USB_Log_Printf("[TEST] Address: 0x%08X\r\n", addr);
        HAL_USB_Log_Printf("[TEST] Offset: %u (0x%X)\r\n", offset, offset);
        HAL_USB_Log_Printf("[TEST] In Pool: %s\r\n", in_pool ? "YES" : "NO");
        
        // 释放内存
        lv_mem_free(test_ptr);
        HAL_USB_Log_Printf("[TEST] Memory freed\r\n");
    } else {
        HAL_USB_Log_Printf("[ERROR] Allocation failed!\r\n");
    }
    
    // 3. 分配后再次检查
    lv_mem_monitor(&mon);
    HAL_USB_Log_Printf("[MEM] After test: Used=%u Free=%u Frag=%u%%\r\n", 
                        mon.used_size, mon.free_size, mon.frag_pct);
    HAL_USB_Log_Printf("==========================================\r\n\r\n");
}

/**
 * @brief 检查指针合法性（用于调试卡死问题）
 * 
 * @param ptr 要检查的指针
 * @param func_name 调用函数名
 */
void lv_mem_debug_check_ptr(const void* ptr, const char* func_name)
{
    if (ptr == NULL) {
        HAL_USB_Log_Printf("[MEM_CHECK] %s: NULL pointer!\r\n", func_name);
        return;
    }
    
    uintptr_t addr = (uintptr_t)ptr;
    bool in_pool = lv_mem_is_in_pool(ptr);
    
    if (!in_pool) {
        HAL_USB_Log_Printf("[MEM_CHECK] %s: ptr=0x%08X OUT OF POOL! (0x%08X-0x%08X)\r\n", 
                            func_name, addr, 
                            LVGL_MEM_POOL_START, LVGL_MEM_POOL_END);
    } else {
        uint32_t offset = lv_mem_get_offset(ptr);
        HAL_USB_Log_Printf("[MEM_CHECK] %s: ptr=0x%08X OK (offset=0x%X)\r\n", 
                            func_name, addr, offset);
    }
}

/**
 * @brief 定期监控内存使用情况（可在主循环调用）
 */
void lv_mem_debug_monitor(void)
{
    static uint32_t last_max_used = 0;
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    
    // 只在最大使用量变化时输出
    if (mon.max_used != last_max_used) {
        HAL_USB_Log_Printf("[MEM_MONITOR] Max used: %u/%u (%.1f%%), Frag=%u%%\r\n", 
                            mon.max_used, mon.total_size,
                            (float)mon.max_used * 100.0f / mon.total_size,
                            mon.frag_pct);
        last_max_used = mon.max_used;
    }
    
    // 如果碎片率过高，发出警告
    if (mon.frag_pct > 50) {
        HAL_USB_Log_Printf("[MEM_WARNING] High fragmentation: %u%%!\r\n", mon.frag_pct);
    }
    
    // 如果可用内存不足 10%，发出警告
    if (mon.free_size < (mon.total_size / 10)) {
        HAL_USB_Log_Printf("[MEM_WARNING] Low memory: only %u bytes free!\r\n", mon.free_size);
    }
}
