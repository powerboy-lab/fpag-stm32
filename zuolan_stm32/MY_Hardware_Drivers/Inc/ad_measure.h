/*******************************************************************************
 * @file      ad_measure.h
 * @author    左岚
 * @version   V1.0
 * @date      2025-07-18
 * @brief     ADC（模数转换器）模块的头文件??
 * @note      此文件定义了ADC数据采集和处理所需的数据结构、外部变量和函数原型??
 * 它支持双通道并行采集，并提供了灵活的频率设置模式，可以根??
 * 信号频率自动计算采样率，或直接指定采样率??
 *******************************************************************************/

#ifndef __AD_MEASUFRE_H__
#define __AD_MEASUFRE_H__

#include "commond_init.h"
#include "cmd_to_fun.h"
#include "bsp_system.h"

#define AD_SIGNAL_SAMPLE_MIN_HZ 1000000U
#define AD_SIGNAL_SAMPLE_MAX_HZ 48000000U
#define AD_SIGNAL_SAMPLE_MULTIPLE 20.0f
// --- 枚举定义 ---

/**
 * @brief  定义传递给ADC配置函数的频率参数的类型??
 * @details 使用此枚举可以明确函数调用时传入的频率值是信号本身的频率，
 * 还是用户期望的ADC采样频率，从而避免了使用“魔术数字”，
 * 使代码意图更清晰，可读性更强??
 */
typedef enum
{
    FREQ_MODE_SIGNAL,   ///< 模式0: 输入值为信号频率。程序将基于此频率计算一个合适的采样率，以保证采样的周期完整性，便于后续FFT等处理??
    FREQ_MODE_SAMPLING  ///< 模式1: 输入值为直接指定的ADC采样频率。程序将直接使用此值配置ADC硬件??
} AdcFrequencyMode;


// --- 外部变量声明 ---
// 这些变量??ad.c 中定义，此处声明以便在工程的其他文件中访问ADC的最终处理结果??

/**
 * @brief ADC采样数据缓冲??(浮点数形????
 * @details 这两个数组用于存储经过转换的电压值。原始的ADC采样值（通常??2位或16位整数）
 * ??`ad_proc` 函数中被处理并转换为对应的实际电压浮点数后，存放在这里??
 * AD_FIFO_SIZE 是在其他地方定义的宏，代表采样点的数量??
 */
extern float fifo_data1_f[AD_FIFO_SIZE], fifo_data2_f[AD_FIFO_SIZE];

/**
 * @brief 存储计算出的电压幅值??
 * @details ??`ad_proc` 函数对采集到??`fifo_data` 数据进行处理（如FFT或查找最大最小值）后，
 * 计算出的两个通道信号的电压幅值（Amplitude）或峰峰值（Vpp）将保存在这两个变量中??
 */
extern float vol_amp1, vol_amp2;


// --- 函数原型声明 ---

/**
 * @brief  配置并启动一次双通道并行的ADC采集??
 * @param  freq1  通道1的频率设置值??
 * @param  mode1  通道1的频率模??(信号频率或采样频????
 * @param  freq2  通道2的频率设置值??
 * @param  mode2  通道2的频率模式??
 * @retval None
 */
void vpp_adc_parallel_ex(float freq1, float freq2);
void vpp_adc_parallel(float freq1, AdcFrequencyMode mode1, float freq2, AdcFrequencyMode mode2);
void updateAdcFrequencyResults(float sample_rate1, float sample_rate2);
void setSamplingFrequency(float freq, int channel, AdcFrequencyMode mode);

/**
 * @brief  ADC数据后处理函数??
 * @details 此函数通常在DMA传输完成（即一次采集结束后）被调用??
 * 它的核心任务是：
 * 1. 将DMA缓冲区中的原始ADC值转换为实际电压值，并存??`fifo_dataN_f` 数组??
 * 2. 对电压数据进行算法处理，例如计算其峰峰??Vpp)、有效??RMS)或进行FFT分析??
 * 3. 将最终计算结果（如幅值）存入 `vol_ampN` 变量中??
 * @retval None
 */
void ad_proc(void);

#endif //__AD_MEASUFRE_H__
