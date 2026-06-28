/*******************************************************************************
 * @file      phase_measure.c
 * @author    左岚
 * @version   V1.0
 * @date      2025-07-18
 * @brief     使用过零点检测法实现相位差测量
 * @note      此文件中的方法通过寻找两个信号的上升沿过零点来计算它们之间的
 * 时间差，并将其转换为相位差。此方法原理简单、计算快速，但其精度
 * 强依赖于采样率，并且对信号中的噪声和直流偏置较为敏感。
 *
 * **核心假设**: 该算法假定传入的数据长度 `lengh` 恰好对应信号的
 * 一个完整周期(360°)。若此假设不成立，则计算结果不准确。
 *******************************************************************************/

#include "phase_measure.h"

// 全局变量，用于保存最终计算出的相位差（单位：角度）
float phase_diff;

/**
 * @brief  使用过零点检测法计算两个信号的相位差
 * @param  fifo_data1_f 指向信号1的采样数据数组
 * @param  fifo_data2_f 指向信号2的采样数据数组
 * @param  lengh        (应为length) 采样数据的长度，代表一个信号周期
 * @retval None (结果直接存入全局变量 `phase_diff`)
 */
void calculate_phase_diff(float *fifo_data1_f, float *fifo_data2_f, int lengh)
{
    // 初始化变量
    int zero_cross1 = -1, zero_cross2 = -1; // 两路信号的过零点索引, -1表示尚未找到
    int i;

    // 1. 遍历采样数据，寻找两个信号各自的第一个上升沿过零点
    for (i = 1; i < lengh; i++)
    {
        // AD1 上升沿过零点检测：当信号从非正数(<=0)变为正数(>0)时
        if (zero_cross1 == -1 && fifo_data1_f[i] > 0 && fifo_data1_f[i - 1] <= 0)
        {
            // 记录下当前点的索引
            zero_cross1 = i;
        }

        // AD2 上升沿过零点检测
        if (zero_cross2 == -1 && fifo_data2_f[i] > 0 && fifo_data2_f[i - 1] <= 0)
        {
            zero_cross2 = i;
        }

        // 优化：如果两个过零点都已找到，则提前退出循环以提高效率
        if (zero_cross1 != -1 && zero_cross2 != -1)
        {
            break;
        }
    }

    // 2. 合法性检查：确保两个信号都找到了过零点
    if (zero_cross1 == -1 || zero_cross2 == -1)
    {
        phase_diff = 0.0f; // 如果任一信号没有找到过零点，则无法计算，相位差置为0
        return;
    }

    // 3. 计算时间差（单位：采样点数）
    int delta_t = zero_cross2 - zero_cross1;

    // 4. 将时间差（采样点数差）转换为相位差（单位：角度）
    // 核心公式：相位差 = (时间差 / 周期) * 360°
    // 此处，`delta_t`是时间差（点数），`lengh`被假定为周期（点数）
    phase_diff = ((float)delta_t / (float)lengh) * 360.0f;

    // 5. 将相位差归一化到 [-180, 180] 度的标准范围内
    if (phase_diff > 180.0f)
    {
        phase_diff -= 360.0f;
    }
    else if (phase_diff < -180.0f)
    {
        phase_diff += 360.0f;
    }
}
