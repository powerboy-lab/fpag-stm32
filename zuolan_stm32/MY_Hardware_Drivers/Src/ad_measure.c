/*******************************************************************************
 * @file      ad_measure.c
 * @author    左岚
 * @version   V1.1
 * @date      2025-07-18
 * @brief     ADC 测量与数据处理功能实??
 * @note      此文件实现了ADC采样的频率设置、FIFO数据读取以及电压幅值计算等功能??
 * 新增了频率模式枚举，使得调用者可以明确指定输入的频率值是信号频率
 * 还是直接的采样频率，增强了代码的清晰度和灵活性??
 *******************************************************************************/

#include "ad_measure.h"
#include "stm32f4xx_hal.h"

// --- 宏定??---

// ADC原始数据到实际电压值的转换比例因子??
// 例如，如果ADC??2位的??-4095），而参考电压范围是20V，则此值可能基??(2^12 - 1) / 2 = 2047.5??
#define ADC_SCALE 2048.0f

// 电压计算的偏移量。用于将单极性ADC数据转换为双极性电压信号??
// 转换公式?? Voltage = (ADC_Value / ADC_SCALE) * VOLTAGE_OFFSET - VOLTAGE_OFFSET
#define VOLTAGE_OFFSET 10.0f

#define AD_SAMPLE_SETTLE_MS 2U
#define AD_FIFO_FULL_TIMEOUT 3000000UL




// --- 全局变量 ---

// 用于存储ADC通道1和通道2采样数据中的最大值和最小值（原始ADC值）
u16 vol_maxnum1, vol_minnum1, vol_maxnum2, vol_minnum2;

// 用于存储从硬件FIFO中读取的原始采样数据 (16位无符号整数)
// AD_FIFO_SIZE 是在 "ad_measure.h" 中定义的FIFO缓冲区大??
u16 fifo_data1[AD_FIFO_SIZE], fifo_data2[AD_FIFO_SIZE];

// 用于存储转换为实际电压值的浮点数结??
float fifo_data1_f[AD_FIFO_SIZE], fifo_data2_f[AD_FIFO_SIZE];

// 用于存储最终计算出的电压峰峰值（幅值）
float vol_amp1, vol_amp2;



// --- 函数实现 ---

/**
 * @brief  在给定的无符??6位整型数组中查找最大值和最小值??
 * @param  numbers: 指向待搜索数组的指针??
 * @param  size:    数组的元素个数??
 * @param  max:     指向用于存储最大值的变量的指针??
 * @param  min:     指向用于存储最小值的变量的指针??
 * @retval None
 */
void findMinMax(u16 *numbers, int size, u16 *max, u16 *min)
{
    // 初始化最大值和最小值为数组的第一个元??
    *max = numbers[0];
    *min = numbers[0];

    // 从第二个元素开始遍历整个数??
    for (int i = 1; i < size; i++)
    {
        // 如果当前元素大于当前最大值，则更新最大??
        if (numbers[i] > *max)
        {
            *max = numbers[i];
        }
        // 如果当前元素小于当前最小值，则更新最小??
        if (numbers[i] < *min)
        {
            *min = numbers[i];
        }
    }
}

static float estimateFrequencyFromSamples(u16 *samples, int size, float sample_rate)
{
    if (samples == 0 || size < 4 || sample_rate <= 0.0f)
        return 0.0f;

    u16 max_value, min_value;
    findMinMax(samples, size, &max_value, &min_value);

    if (max_value <= min_value || (max_value - min_value) < 64)
        return 0.0f;

    const float center = ((float)max_value + (float)min_value) * 0.5f;
    float hysteresis = ((float)max_value - (float)min_value) * 0.08f;

    if (hysteresis < 8.0f)
        hysteresis = 8.0f;

    const float high_threshold = center + hysteresis;
    const float low_threshold = center - hysteresis;

    int is_high = (samples[0] > center) ? 1 : 0;
    int rising_count = 0;
    float first_rising_pos = -1.0f;
    float last_rising_pos = -1.0f;

    for (int i = 1; i < size; i++)
    {
        int next_high = is_high;

        if (samples[i] > high_threshold)
            next_high = 1;
        else if (samples[i] < low_threshold)
            next_high = 0;

        if (!is_high && next_high)
        {
            const float previous = (float)samples[i - 1];
            const float current = (float)samples[i];
            float crossing_pos = (float)i;

            if (current != previous)
            {
                float frac = (center - previous) / (current - previous);
                if (frac < 0.0f)
                    frac = 0.0f;
                else if (frac > 1.0f)
                    frac = 1.0f;
                crossing_pos = (float)(i - 1) + frac;
            }

            if (first_rising_pos < 0.0f)
                first_rising_pos = crossing_pos;

            last_rising_pos = crossing_pos;
            rising_count++;
        }

        is_high = next_high;
    }

    if (rising_count < 2 || last_rising_pos <= first_rising_pos)
        return 0.0f;

    return ((float)(rising_count - 1) * sample_rate) / (last_rising_pos - first_rising_pos);
}

void updateAdcFrequencyResults(float sample_rate1, float sample_rate2)
{
    freq1 = estimateFrequencyFromSamples(fifo_data1, AD_FIFO_SIZE, sample_rate1);
    freq2 = estimateFrequencyFromSamples(fifo_data2, AD_FIFO_SIZE, sample_rate2);
}
/**
 * @brief  设置指定ADC通道的采样频率??
 * @param  freq:    频率??(单位: Hz)。其具体含义??'mode' 参数决定??
 * @param  channel: 目标ADC通道??(1 ??2)??
 * @param  mode:    频率模式，决定如何解??'freq' 参数??
 * - FREQ_MODE_SIGNAL: 'freq' 被视为输入信号的频率??
 * - FREQ_MODE_SAMPLING: 'freq' 被视为要直接设置的采样频率??
 * @retval None
 * @note   此函数会根据所选模式计算出最终的采样??fs)，并生成一个频率控制字(M)??
 * 然后将该控制字写入对应的硬件寄存器来配置FPGA或相关外设??
 */
void setSamplingFrequency(float freq, int channel, AdcFrequencyMode mode)
{
    // 如果频率??，则不进行任何操作，直接返回
    if (freq == 0)
        return;

    unsigned int fs = 0; // 最终要设置的采样频??(Sampling Frequency)

    if (mode == FREQ_MODE_SAMPLING)
    {
        // 直接指定采样频率模式：直接使用传入的值作为采样率
        fs = (unsigned int)freq;
    }
    else // 默认??FREQ_MODE_SIGNAL 模式
    {
        // 信号频率模式：根据信号频率计算采样频率，以确保采样的完整性??
        // AD_FIFO_SIZE_N 可能是为了满足奈奎斯特采样定理而设定的一个倍数??
        // 例如，对于一个频率为f的信号，采样率fs至少要大??f??
        float sample_rate = freq * AD_SIGNAL_SAMPLE_MULTIPLE;

        if (sample_rate < AD_SIGNAL_SAMPLE_MIN_HZ)
            sample_rate = AD_SIGNAL_SAMPLE_MIN_HZ;
        else if (sample_rate > AD_SIGNAL_SAMPLE_MAX_HZ)
            sample_rate = AD_SIGNAL_SAMPLE_MAX_HZ;

        fs = (unsigned int)sample_rate;
    }

    // 计算频率控制??M??
    // M 是一个通过特定公式转换得到的数值，用于配置硬件的时钟分频或频率合成??
    // AD_FREQ_CONSTANT ??FPGA_BASE_CLK 应该是在头文件中定义的常量??
    unsigned int M = AD_FREQ_CONSTANT * fs / FPGA_BASE_CLK;

    // --- 将频率控制字写入硬件寄存??---
    // 这部分代码直接操作硬件，可能是通过总线与FPGA进行通信??
    if (channel == 1)
    {
        AD_FREQ_SET(1);      // 发送命??片选，准备设置通道1的频??
        HAL_Delay(1);        // 短暂延时，确保命令被接收
        AD1_FS_H = M >> 16;  // 写入频率控制字的??6??
        HAL_Delay(1);        // 延时
        AD1_FS_L = M & 0xFFFF; // 写入频率控制字的??6??
    }
    else
    {
        AD_FREQ_SET(2);      // 发送命??片选，准备设置通道2的频??
        HAL_Delay(1);
        AD2_FS_H = M >> 16;  // 写入频率控制字的??6??
        HAL_Delay(1);
        AD2_FS_L = M & 0xFFFF; // 写入频率控制字的??6??
    }
}

/**
 * @brief  从指定通道的硬件FIFO中读取数据到内存缓冲区??
 * @param  channel:      目标ADC通道??(1 ??2)??
 * @param  fifo_data:    用于存储原始ADC采样数据的数组指针??
 * @param  fifo_data_f:  用于存储转换为浮点电压值的数组指针??
 * @retval None
 */
void readFIFOData(int channel, u16 *fifo_data, float *fifo_data_f)
{
    // 使能FIFO读取操作
    if (channel == 1)
        AD_FIFO_READ_ENABLE(1);
    else
        AD_FIFO_READ_ENABLE(2);

    HAL_Delay(1); // 延时确保使能生效

    // 循环读取整个FIFO的数??
    for (int i = 0; i < AD_FIFO_SIZE; i++)
    {
        // 从硬件数据端口读取一??6位的数据
        if (channel == 1)
            fifo_data[i] = AD1_DATA_SHOW;
        else
            fifo_data[i] = AD2_DATA_SHOW;

        // 将读取到的原始ADC值转换为实际的电压值（浮点数）
        // 公式将ADC的数字量??-4095）映射到电压范围（例??10V??10V??
        fifo_data_f[i] = fifo_data[i] * VOLTAGE_OFFSET / ADC_SCALE - VOLTAGE_OFFSET;
    }

    // 禁止FIFO读取操作，释放总线或相关资??
    if (channel == 1)
        AD_FIFO_READ_DISABLE(1);
    else
        AD_FIFO_READ_DISABLE(2);
}

static int waitFifoFull(int channel)
{
    u32 timeout = AD_FIFO_FULL_TIMEOUT;

    if (channel == 1)
    {
        while (AD1_FULL_FLAG != 0x0001)
        {
            if (timeout-- == 0)
                return 0;
        }
    }
    else
    {
        while (AD2_FULL_FLAG != 0x0001)
        {
            if (timeout-- == 0)
                return 0;
        }
    }

    return 1;
}

static int captureFifosOnce(int capture_ch1, int capture_ch2)
{
    int result = 0;

    if (capture_ch1)
        AD_FIFO_WRITE_ENABLE(1);

    if (capture_ch2)
        AD_FIFO_WRITE_ENABLE(2);

    if (capture_ch1 && waitFifoFull(1))
        result |= 0x01;

    if (capture_ch2 && waitFifoFull(2))
        result |= 0x02;

    if (capture_ch1)
        AD_FIFO_WRITE_DISABLE(1);

    if (capture_ch2)
        AD_FIFO_WRITE_DISABLE(2);

    if (result & 0x01)
        readFIFOData(1, fifo_data1, fifo_data1_f);

    if (result & 0x02)
        readFIFOData(2, fifo_data2, fifo_data2_f);

    return result;
}

void vpp_adc_parallel_ex(float freq1, float freq2)
{
    vpp_adc_parallel(freq1, FREQ_MODE_SAMPLING, freq2, FREQ_MODE_SAMPLING);
}

/**
 * @brief  同时（并行地）设置AD1和AD2的采集任务，等待完成后读取并处理数据??
 * @param  freq1: AD1 的频率??(单位: Hz)。如果为0，则不采集此通道??
 * @param  mode1: AD1 的频率模式??
 * @param  freq2: AD2 的频率??(单位: Hz)。如果为0，则不采集此通道??
 * @param  mode2: AD2 的频率模式??
 * @retval None
 */
void vpp_adc_parallel(float freq1, AdcFrequencyMode mode1, float freq2, AdcFrequencyMode mode2)
{
    int capture_ch1 = (freq1 > 0);
    int capture_ch2 = (freq2 > 0);
    int restore_sample_clk1 = (capture_ch1 && mode1 == FREQ_MODE_SIGNAL);
    int restore_sample_clk2 = (capture_ch2 && mode2 == FREQ_MODE_SIGNAL);
    int discard_first_capture = restore_sample_clk1 || restore_sample_clk2;
    int capture_result;

    // --- 步骤1: 配置通道1 ---
    if (capture_ch1)
    {
        setSamplingFrequency(freq1, 1, mode1);
    }

    // --- 步骤2: 配置通道2 ---
    if (capture_ch2)
    {
        setSamplingFrequency(freq2, 2, mode2);
    }

    HAL_Delay(AD_SAMPLE_SETTLE_MS);

    if (discard_first_capture)
        captureFifosOnce(capture_ch1, capture_ch2);

    // --- 步骤3: 双通道同时采集，读取数据并进行处理 ---
    capture_result = captureFifosOnce(capture_ch1, capture_ch2);

    if (capture_ch1)
    {
        if (capture_result & 0x01)
        {
            findMinMax(fifo_data1, AD_FIFO_SIZE, &vol_maxnum1, &vol_minnum1);
            vol_amp1 = (vol_maxnum1 - vol_minnum1) * VOLTAGE_OFFSET / ADC_SCALE;
        }
        else
        {
            vol_amp1 = 0.0f;
        }
    }

    if (capture_ch2)
    {
        if (capture_result & 0x02)
        {
            findMinMax(fifo_data2, AD_FIFO_SIZE, &vol_maxnum2, &vol_minnum2);
            vol_amp2 = (vol_maxnum2 - vol_minnum2) * VOLTAGE_OFFSET / ADC_SCALE;
        }
        else
        {
            vol_amp2 = 0.0f;
        }
    }

    // Restore the stable default AD sample clock after automatic Vpp sampling.
    if (restore_sample_clk1)
        setSamplingFrequency((float)AD_SIGNAL_SAMPLE_MAX_HZ, 1, FREQ_MODE_SAMPLING);

    if (restore_sample_clk2)
        setSamplingFrequency((float)AD_SIGNAL_SAMPLE_MAX_HZ, 2, FREQ_MODE_SAMPLING);
}
/**
 * @brief  ADC处理流程的顶层示例函数??
 * @param  None
 * @retval None
 * @note   此函数演示了如何调用 `vpp_adc_parallel`??
 */
void ad_proc(void)
{
    // 调用并行采集函数，完成一次双通道测量任务??
    // - 通道1: 采集一个频率为 1MHz 的信号，使用 FREQ_MODE_SIGNAL 模式，让函数自动计算合适的采样率??
    // - 通道2: 直接40MHz 的固定频率进行采样，使用 FREQ_MODE_SAMPLING 模式??
    vpp_adc_parallel(freq1, FREQ_MODE_SIGNAL, 
										 40000000, FREQ_MODE_SAMPLING);
		my_printf(&huart1, "AD1 Vpp=%f, AD2 Vpp=%f\r\n", vol_amp1, vol_amp2);
\
	
}
