/**
 * @file key_app.h
 * @brief 按键应用处理程序的头文件
 * @details
 * 该文件为按键应用模块（key_app.c）提供了公共接口。
 * 它声明了外部模块需要调用的函数，包括按键处理主函数以及
 * 用于获取和设置AD采集频率的函数。
 *
 * @author 左岚
 * @date 2025-07-18
 * @version 2.0 (已重构，提高可扩展性)
 */
#ifndef __KEY_APP_H__
#define __KEY_APP_H__

#include "bsp_system.h"

// *********************************************************************************
// 1. 外部函数声明
// *********************************************************************************

/**
 * @brief 按键处理主函数。
 * @details 应在主循环中周期性调用此函数来扫描按键并执行相应功能。
 */
void key_proc(void);

/**
 * @brief 设置当前AD（模数转换器）的采集频率。
 * @param freq 要设置的新的采集频率，单位为Hz。
 */
void set_current_ad_frequency(float freq);

/**
 * @brief 获取当前AD（模数转换器）的采集频率。
 * @return float 返回当前配置的采集频率，单位为Hz。
 */
float get_current_ad_frequency(void);


#endif // __KEY_APP_H__
