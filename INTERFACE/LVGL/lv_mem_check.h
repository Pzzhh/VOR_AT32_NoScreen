/**
 * @file lv_mem_check.h
 * @brief LVGL 内存检查辅助函数
 * @date 2026-01-20
 */

#ifndef LV_MEM_CHECK_H
#define LV_MEM_CHECK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief LVGL 内存池信息 (从 map 文件获取)
 * 
 * work_mem_int 地址: 0x2004C19C
 * 大小: 46080 字节 (45KB) - 修改前的配置
 * 修改后: 131072 字节 (128KB)
 */
#define LVGL_MEM_POOL_START   0x2004C19C
#define LVGL_MEM_POOL_SIZE    (LV_MEM_SIZE)
#define LVGL_MEM_POOL_END     (LVGL_MEM_POOL_START + LVGL_MEM_POOL_SIZE)

/**
 * @brief 检查指针是否在 LVGL 内存池范围内
 * 
 * @param ptr 要检查的指针
 * @return true 在范围内
 * @return false 不在范围内
 */
static inline bool lv_mem_is_in_pool(const void* ptr)
{
    uintptr_t addr = (uintptr_t)ptr;
    return (addr >= LVGL_MEM_POOL_START) && (addr < LVGL_MEM_POOL_END);
}

/**
 * @brief 获取指针在内存池中的偏移量
 * 
 * @param ptr 指针
 * @return uint32_t 偏移量（字节）
 */
static inline uint32_t lv_mem_get_offset(const void* ptr)
{
    if (!lv_mem_is_in_pool(ptr)) {
        return 0xFFFFFFFF;  // 无效
    }
    return (uintptr_t)ptr - LVGL_MEM_POOL_START;
}

/**
 * @brief 打印内存池信息
 */
static inline void lv_mem_print_pool_info(void)
{
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    
    LV_LOG_USER("========== LVGL Memory Pool ==========");
    LV_LOG_USER("Start Address: 0x%08X", LVGL_MEM_POOL_START);
    LV_LOG_USER("End Address:   0x%08X", LVGL_MEM_POOL_END);
    LV_LOG_USER("Total Size:    %u bytes (%u KB)", 
                 LVGL_MEM_POOL_SIZE, LVGL_MEM_POOL_SIZE / 1024);
    LV_LOG_USER("Used:          %u bytes", mon.used_size);
    LV_LOG_USER("Free:          %u bytes", mon.free_size);
    LV_LOG_USER("Fragmentation: %u%%", mon.frag_pct);
    LV_LOG_USER("Max Used:      %u bytes", mon.max_used);
    LV_LOG_USER("======================================");
}

#ifdef __cplusplus
}
#endif

#endif /* LV_MEM_CHECK_H */
