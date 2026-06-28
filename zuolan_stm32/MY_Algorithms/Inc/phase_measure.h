/*******************************************************************************
 * @file      phase_measure.h
 * @author    左岚
 * @version   V1.0
 * @date      2025-07-18
 * @brief     相位差测量模块的头文件
 * @note      此文件声明了相位差测量的全局结果变量和核心计算函数的外部接口。
 *******************************************************************************/

#ifndef __PHASE_MEASURE_H__
#define __PHASE_MEASURE_H__

// 包含AD测量模块的头文件，可能用于获取采样数据。
#include "ad_measure.h"
// 包含项目通用初始化或类型定义头文件。(文件名可能存在拼写错误, 疑为 command_init.h)
#include "commond_init.h"

/**
 * @brief 全局变量，用于存储计算出的相位差（单位：角度）。
 * @details 该变量由 `calculate_phase_diff` 函数直接写入结果。
 */
extern float phase_diff;

/**
 * @brief  使用过零点检测法计算相位差的函数声明
 * @param  fifo_data1_f 指向信号1的采样数据数组
 * @param  fifo_data2_f 指向信号2的采样数据数组
 * @param  lengh        (应为length) 采样数据长度。**此参数被假定为一个完整周期的点数。**
 * @see    phase_measure.c 文件中的具体实现
 */
void calculate_phase_diff(float *fifo_data1_f, float *fifo_data2_f, int lengh);

#endif // __PHASE_MEASURE_H__
