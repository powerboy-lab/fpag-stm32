/*******************************************************************************
 * @file      my_fft.c
 * @author    左岚
 * @version   V1.1
 * @date      2025-07-18
 * @brief     FFT(快速傅里叶变换)处理与频谱分析功能实现
 * @note      本文件基于ARM官方的CMSIS-DSP库实现了信号的频谱分析。
 * 主要功能包括：FFT模块初始化、Hanning窗生成、频谱计算、
 * 幅度谱归一化、峰值频率插值精确化以及总谐波失真(THD)计算。
 *******************************************************************************/

#include "my_fft.h"
#include <math.h> // 包含数学库，用于cos, sqrt, roundf等函数

// ============================= FFT相关全局变量 =============================
// ARM CMSIS-DSP库所需的基4 FFT实例结构体
arm_cfft_radix4_instance_f32 fft_instance;
// FFT输入缓冲区。长度为 FFT_LENGTH * 2，因为需要存储复数（实部、虚部交错）
float fft_input_buffer[FFT_LENGTH * 2];
// FFT幅度谱输出缓冲区
float fft_magnitude[FFT_LENGTH];
// 窗函数系数缓冲区
float window_buffer[FFT_LENGTH];

/**
 * @brief  生成Hanning窗函数系数
 * @details Hanning窗可以有效减少频谱泄漏，即因信号截断导致能量扩散到相邻频点的现象。
 * 此函数预先计算好窗函数并存入缓冲区，以提高处理效率。
 * @param  None
 * @retval None
 */
void generate_hanning_window(void)
{
    uint16_t i;
    for (i = 0; i < FFT_LENGTH; i++)
    {
        // Hanning窗的标准数学公式: w(n) = 0.5 * (1 - cos(2*pi*n / (N-1)))
        window_buffer[i] = 0.5f * (1.0f - arm_cos_f32(2.0f * 3.14159265f * i / (FFT_LENGTH - 1)));
    }
}

/**
 * @brief  FFT模块初始化
 * @details 此函数初始化ARM CMSIS-DSP的FFT实例，并生成后续计算所需的Hanning窗。
 * @param  None
 * @retval None
 */
void fft_init(void)
{
    // 初始化radix-4 FFT实例结构体
    arm_cfft_radix4_init_f32(&fft_instance, FFT_LENGTH, 0, 1);
    // 参数说明:
    // &fft_instance:  指向FFT实例的指针
    // FFT_LENGTH:     FFT点数 (例如: 1024)
    // 0:              ifftFlag, 0表示正向FFT, 1表示逆向IFFT
    // 1:              bitReverseFlag, 1表示输出结果按正常顺序排列

    // 生成Hanning窗函数
    generate_hanning_window();
}

/**
 * @brief  计算输入数据的FFT频谱
 * @param  input_data  指向原始采样数据的浮点数组指针
 * @param  data_length 输入数据的实际长度 (最大为FFT_LENGTH)
 * @retval None
 */
void calculate_fft_spectrum(float* input_data, uint16_t data_length)
{
    uint16_t i;
    uint16_t actual_length = (data_length > FFT_LENGTH) ? FFT_LENGTH : data_length;

    // 1. 清空缓冲区，确保每次计算的初始状态干净
    memset(fft_input_buffer, 0, sizeof(fft_input_buffer));
    memset(fft_magnitude, 0, sizeof(fft_magnitude));

    // 2. 数据预处理：将实数输入数据应用窗函数，并转换为复数格式
    for (i = 0; i < actual_length; i++)
    {
        // 乘以窗函数系数以减少频谱泄漏
        fft_input_buffer[2*i]     = input_data[i] ; // 实部
        fft_input_buffer[2*i + 1] = 0.0f;                             // 虚部设为0
    }
    // 如果实际数据长度小于FFT_LENGTH，缓冲区剩余部分已清零，这相当于进行了“零填充(Zero Padding)”。
    // 零填充不提高频谱的物理分辨率，但可以增加频点密度，有助于观察和插值。

    // 3. 执行核心的复数FFT计算
    arm_cfft_radix4_f32(&fft_instance, fft_input_buffer);

    // 4. 计算复数结果的模，得到幅度谱
    arm_cmplx_mag_f32(fft_input_buffer, fft_magnitude, FFT_LENGTH);

    // 5. 幅度谱归一化校正
    //    目的是将FFT输出的抽象数值转换成具有物理意义的电压幅值。
    //    校正过程包含两部分：FFT点数校正 和 窗函数能量校正。
    // Hanning窗的能量校正系数：理论值为2.0（窗函数平均损失50%能量）
    float window_power_correction = 2.0f;

    for (i = 0; i < FFT_LENGTH / 2; i++)
    {
        if (i == 0) {
            // 直流分量(DC): 幅度 = FFT结果 / N
            fft_magnitude[i] = fft_magnitude[i] / FFT_LENGTH * window_power_correction;
        } else {
            // 交流分量(AC): 幅度 = FFT结果 * 2 / N
            // 乘以2是因为FFT结果是对称的双边谱，能量分布在正负频率上，我们只看正频率，故需乘以2。
            fft_magnitude[i] = fft_magnitude[i] * 2.0f / FFT_LENGTH * window_power_correction;
        }
    }
}

/**
 * @brief  通过串口输出FFT频谱分析结果
 * @param  None
 * @retval None
 */
void output_fft_spectrum(void)
{
    uint16_t i;
    float freq_resolution;
    float current_freq;

    // 获取当前ADC的采样频率，这是计算实际频率的基础
    float sampling_freq = get_current_ad_frequency();
    // 计算频率分辨率 = 采样率 / FFT点数
    freq_resolution = sampling_freq / FFT_LENGTH;

    my_printf(&huart1, "=== FFT Spectrum Analysis ===\r\n");
    my_printf(&huart1, "Sampling Freq: %.0f Hz\r\n", sampling_freq);
    my_printf(&huart1, "Freq Resolution: %.2f Hz\r\n", freq_resolution);
    my_printf(&huart1, "--- Spectrum Data ---\r\n");

    // 打印部分频谱数据（从第10个频点开始，以避开直流和低频噪声）
    for (i = 10; i < FFT_LENGTH / 2; i++)
    {
        current_freq = i * freq_resolution;
        my_printf(&huart1, "%.1f Hz: %.6f\r\n", current_freq, fft_magnitude[i]);
    }

    my_printf(&huart1, "--- Peak Analysis ---\r\n");

    // 使用插值法计算更精确的峰值频率，并进行分析
    float peak_freq_raw = get_precise_peak_frequency(sampling_freq);
    float peak_freq = round_to_nearest_k(peak_freq_raw); // 四舍五入到最近的整KHz
    float thd = calculate_thd(peak_freq, sampling_freq); // 计算总谐波失真

    // 获取峰值点的幅度（用于显示）
    float max_magnitude = 0.0f;
    for (i = 1; i < FFT_LENGTH / 2; i++)
    {
        if (fft_magnitude[i] > max_magnitude)
        {
            max_magnitude = fft_magnitude[i];
        }
    }

    // 计算扩展的失真指标
    float thd_n = calculate_thd_n(peak_freq, sampling_freq);
    float sinad = calculate_sinad(peak_freq, sampling_freq);
    
    my_printf(&huart1, "Peak Freq: %.0f Hz (Raw: %.2f Hz), Magnitude: %.6fv\r\n", peak_freq, peak_freq_raw, max_magnitude);
    my_printf(&huart1, "THD: %.3f%%\r\n", thd);
    my_printf(&huart1, "THD+N: %.3f%%\r\n", thd_n);
    my_printf(&huart1, "SINAD: %.1f dB\r\n", sinad);
    my_printf(&huart1, "DC Component: %.6fv\r\n", fft_magnitude[0]);
    my_printf(&huart1, "=== End of Spectrum ===\r\n");
}

/**
 * @brief  使用抛物线插值法精确计算峰值频率
 * @details FFT的频率分辨率有限，此方法通过在最大频点及其相邻点之间拟合一条
 * 抛物线，并找到抛物线的顶点，从而得到比FFT分辨率更高的频率估计值。
 * @param  sampling_freq 采样频率
 * @return (float) 精确的峰值频率
 */
float get_precise_peak_frequency(float sampling_freq)
{
    uint16_t i;
    float freq_resolution = sampling_freq / FFT_LENGTH;

    // 1. 寻找最大幅度点（谱峰）的索引
    float max_magnitude = 0.0f;
    uint16_t max_index = 1;
    for (i = 1; i < FFT_LENGTH / 2; i++)
    {
        if (fft_magnitude[i] > max_magnitude)
        {
            max_magnitude = fft_magnitude[i];
            max_index = i;
        }
    }

    // 如果峰值点在边界，无法进行三点插值，直接返回粗略频率
    if (max_index <= 1 || max_index >= (FFT_LENGTH / 2 - 1))
    {
        return max_index * freq_resolution;
    }

    // 2. 三点抛物线插值
    float y1 = fft_magnitude[max_index - 1]; // 左边点
    float y2 = fft_magnitude[max_index];     // 中间峰值点
    float y3 = fft_magnitude[max_index + 1]; // 右边点

    // 计算抛物线顶点相对于中心点(max_index)的偏移量 delta
    float delta = 0.5f * (y1 - y3) / (y1 - 2.0f * y2 + y3);

    // 检查分母是否过小，防止除以零
    if (fabsf(y1 - 2.0f * y2 + y3) < 1e-10f)
    {
        delta = 0.0f;
    }
    
    // 限制偏移量范围，确保结果合理
    if (delta > 0.5f)  delta = 0.5f;
    if (delta < -0.5f) delta = -0.5f;

    // 3. 计算精确频率
    float precise_freq = (max_index + delta) * freq_resolution;

    return precise_freq;
}

/**
 * @brief  将频率值四舍五入到最近的1000Hz整数倍
 * @param  frequency 原始频率值
 * @return (float) 四舍五入后的频率值
 */
float round_to_nearest_k(float frequency)
{
    return roundf(frequency / 1000.0f) * 1000.0f;
}

/**
 * @brief  计算总谐波失真 (THD)
 * @details THD = sqrt(∑(谐波功率)) / sqrt(基波功率) × 100%
 *          优化算法包括：改进的频谱泄漏补偿、更多谐波次数、噪声底抑制
 * @param  fundamental_freq 基波频率
 * @param  sampling_freq 采样频率  
 * @return (float) THD值（以百分比形式返回）
 */
float calculate_thd(float fundamental_freq, float sampling_freq)
{
    float freq_resolution = sampling_freq / FFT_LENGTH;
    float fundamental_power = 0.0f;
    float harmonic_power = 0.0f;
    
    // 噪声底抑制阈值（动态计算）
    float noise_floor = 0.0f;
    for (uint16_t i = FFT_LENGTH/4; i < FFT_LENGTH/2; i++) 
    {
        noise_floor += fft_magnitude[i] * fft_magnitude[i];
    }
    noise_floor = sqrtf(noise_floor / (FFT_LENGTH/4)) * 2.0f; // 噪声底估计

    // 1. 找到基波频率对应的FFT频点(bin)
    uint16_t fundamental_bin = (uint16_t)(fundamental_freq / freq_resolution + 0.5f);

    if (fundamental_bin < 1 || fundamental_bin >= FFT_LENGTH / 2)
    {
        return 0.0f; // 基波无效
    }

    // 2. 计算基波功率（使用改进的频谱泄漏补偿）
    // 基波主峰
    fundamental_power = fft_magnitude[fundamental_bin] * fft_magnitude[fundamental_bin];
    
    // 添加邻近bin的泄漏能量，使用更精确的权重
    float leakage_weight = 0.3f;  // 经过优化的泄漏权重
    if (fundamental_bin > 1)
    {
        fundamental_power += leakage_weight * fft_magnitude[fundamental_bin - 1] * fft_magnitude[fundamental_bin - 1];
    }
    if (fundamental_bin < FFT_LENGTH / 2 - 1)
    {
        fundamental_power += leakage_weight * fft_magnitude[fundamental_bin + 1] * fft_magnitude[fundamental_bin + 1];
    }

    // 3. 累加谐波功率（计算2次到15次谐波，提高THD精度）
    for (uint8_t harmonic = 2; harmonic <= 15; harmonic++)
    {
        uint16_t harmonic_bin = harmonic * fundamental_bin;

        // 确保谐波频率低于奈奎斯特频率
        if (harmonic_bin >= FFT_LENGTH / 2)
        {
            break;
        }

        // 累加谐波功率（使用噪声底抑制和泄漏补偿）
        float harmonic_magnitude = fft_magnitude[harmonic_bin];
        
        // 噪声底抑制：只有当谐波幅度大于噪声底时才计入
        if (harmonic_magnitude > noise_floor)
        {
            float harmonic_mag_squared = harmonic_magnitude * harmonic_magnitude;
            
            // 谐波的频谱泄漏补偿（权重减半，因为谐波能量通常较小）
            float harmonic_leakage_weight = leakage_weight * 0.5f;
            if (harmonic_bin > 1 && fft_magnitude[harmonic_bin - 1] > noise_floor)
            {
                harmonic_mag_squared += harmonic_leakage_weight * fft_magnitude[harmonic_bin - 1] * fft_magnitude[harmonic_bin - 1];
            }
            if (harmonic_bin < FFT_LENGTH / 2 - 1 && fft_magnitude[harmonic_bin + 1] > noise_floor)
            {
                harmonic_mag_squared += harmonic_leakage_weight * fft_magnitude[harmonic_bin + 1] * fft_magnitude[harmonic_bin + 1];
            }
            harmonic_power += harmonic_mag_squared;
        }
    }

    // 4. 计算THD（加入最小阈值保护）
    if (fundamental_power > noise_floor * noise_floor)
    {
        // THD = sqrt(谐波总功率) / sqrt(基波功率) * 100%
        float thd = sqrtf(harmonic_power) / sqrtf(fundamental_power) * 100.0f;
        
        // 限制THD最大值，避免异常结果
        return (thd > 100.0f) ? 100.0f : thd;
    }

    return 0.0f; // 基波功率过小，无法可靠计算THD
}

/**
 * @brief  计算THD+N（总谐波失真加噪声）
 * @details THD+N = sqrt(∑(总功率 - 基波功率)) / sqrt(基波功率) × 100%
 *          包含了谐波失真和噪声的综合指标，更全面地评估信号质量
 * @param  fundamental_freq 基波频率
 * @param  sampling_freq 采样频率
 * @return (float) THD+N值（以百分比形式返回）
 */
float calculate_thd_n(float fundamental_freq, float sampling_freq)
{
    float freq_resolution = sampling_freq / FFT_LENGTH;
    float fundamental_power = 0.0f;
    float total_power = 0.0f;
    
    // 1. 找到基波频率对应的FFT频点(bin)
    uint16_t fundamental_bin = (uint16_t)(fundamental_freq / freq_resolution + 0.5f);
    
    if (fundamental_bin < 1 || fundamental_bin >= FFT_LENGTH / 2)
    {
        return 0.0f;
    }
    
    // 2. 计算基波功率（包含泄漏补偿）
    float leakage_weight = 0.3f;
    fundamental_power = fft_magnitude[fundamental_bin] * fft_magnitude[fundamental_bin];
    if (fundamental_bin > 1)
    {
        fundamental_power += leakage_weight * fft_magnitude[fundamental_bin - 1] * fft_magnitude[fundamental_bin - 1];
    }
    if (fundamental_bin < FFT_LENGTH / 2 - 1)
    {
        fundamental_power += leakage_weight * fft_magnitude[fundamental_bin + 1] * fft_magnitude[fundamental_bin + 1];
    }
    
    // 3. 计算总功率（排除直流分量）
    for (uint16_t i = 1; i < FFT_LENGTH / 2; i++)
    {
        total_power += fft_magnitude[i] * fft_magnitude[i];
    }
    
    // 4. 计算THD+N
    float distortion_noise_power = total_power - fundamental_power;
    if (fundamental_power > 0.0f && distortion_noise_power >= 0.0f)
    {
        float thd_n = sqrtf(distortion_noise_power) / sqrtf(fundamental_power) * 100.0f;
        return (thd_n > 200.0f) ? 200.0f : thd_n; // 限制最大值
    }
    
    return 0.0f;
}

/**
 * @brief  计算信纳比(SINAD - Signal to Noise and Distortion Ratio)
 * @details SINAD = 基波功率 / (噪声功率 + 失真功率) (dB)
 *          SINAD是THD+N的倒数，以dB形式表示信号质量
 * @param  fundamental_freq 基波频率
 * @param  sampling_freq 采样频率
 * @return (float) SINAD值（以dB形式返回）
 */
float calculate_sinad(float fundamental_freq, float sampling_freq)
{
    float thd_n_percent = calculate_thd_n(fundamental_freq, sampling_freq);
    
    if (thd_n_percent > 0.0f)
    {
        // SINAD(dB) = -20*log10(THD+N/100)
        float sinad_db = -20.0f * log10f(thd_n_percent / 100.0f);
        return sinad_db;
    }
    
    return 0.0f;
}
