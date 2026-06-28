/**
 * @file   da_output.c
 * @brief   双通道DA波形发生器驱动程序
 * @details  本文件负责实现对FPGA控制的双通道DA输出模块的软件接口。
 * 它通过管理一个内存中的配置结构体（作为硬件寄存器的“影子”），
 * 提供了设置波形参数（频率、幅度、相位等）的API，并能将这些
 * 配置最终应用到硬件，从而驱动DA模块产生指定波形。
 * * @author  左岚
 * @date   2025-07-20
 * @version  1.1
 */
#include "da_output.h"
#include "math.h"

/**
 * @brief DA通道配置参数的内存缓存
 * @details 这是一个模块内的全局数组，用作硬件寄存器配置的软件“影子”或缓存。
 * 所有对DA参数的修改都先发生在这里，之后再通过 DA_Apply_Settings()
 * 函数统一写入硬件。
 * @note   请确保在头文件 da_output.h 中也从 DA_Channel_t 结构体里移除了 step 成员。
 */
DA_Channel_t da_channels[NUM_DA_CHANNELS] = {0};

/**
 * @brief 初始化DA模块
 * @details
 * 此函数在系统启动流程中调用，用于为所有DA通道建立一个已知的、
 * 安全的初始状态。它会设置一组默认波形参数，并立即将其应用到硬件，
 * 确保设备上电后即有稳定的默认波形输出。
 */
void DA_Init(void)
{
    // 为通道0设置默认参数: 20kHz, 幅度1000, 相位90°, 正弦波
    DA_SetConfig(0, 20000.0f, 3000, 0, WAVE_SAWTOOTH);

    // 为通道1设置默认参数: 20kHz, 幅度1000, 相位0°, 正弦波
    DA_SetConfig(1, 20000.0f, 2000, 0, WAVE_SQUARE);

    // 将上述默认配置写入FPGA硬件寄存器使其生效
    DA_Apply_Settings();
}

// =================================================================================
//   单个参数的 Get / Set 函数
// =================================================================================
// 这些函数提供了对单个参数的原子化读写能力，仅操作内存中的配置缓存。
// =================================================================================

/**
 * @brief 设置指定通道的频率 (仅更新内存配置)
 * @param channel_index: 目标通道索引 (0 或 1)
 * @param freq: 频率值，单位 Hz
 */
void DA_SetFREQ(uint8_t channel_index, float freq)
{
    // 参数校验：防止数组越界访问
    if (channel_index >= NUM_DA_CHANNELS)
        return;
    if (freq < 0 || freq * DA_ROM_SIZE > DA_MAX_CLK)
        return;
    // 更新内存中的配置值
    da_channels[channel_index].frequency = freq;
}

/**
 * @brief 获取指定通道的当前频率配置
 * @param channel_index: 目标通道索引 (0 或 1)
 * @return float: 当前配置的频率值(Hz)。若通道索引无效，则返回0.0f。
 */
float DA_GetFREQ(uint8_t channel_index)
{
    // 参数校验：若通道索引无效，返回一个安全的默认值
    if (channel_index >= NUM_DA_CHANNELS)
        return 0.0f;
    return da_channels[channel_index].frequency;
}

void DA1_OUT(float freq)
{
    DA_SetFREQ(0, freq);
    DA_Apply_Settings();
}

void DA2_OUT(float freq)
{
    DA_SetFREQ(1, freq);
    DA_Apply_Settings();
}

/**
 * @brief 设置指定通道的幅度 (仅更新内存配置)
 * @param channel_index: 目标通道索引 (0 或 1)
 * @param amp: 幅度值 (无单位，与DAC分辨率相关)
 */
void DA_SetAmp(uint8_t channel_index, uint16_t amp)
{
    if (channel_index >= NUM_DA_CHANNELS)
        return;
    da_channels[channel_index].amplitude = amp;
}

/**
 * @brief 获取指定通道的当前幅度配置
 * @param channel_index: 目标通道索引 (0 或 1)
 * @return uint16_t: 当前配置的幅度值。若通道索引无效，则返回0。
 */
uint16_t DA_GetAmp(uint8_t channel_index)
{
    if (channel_index >= NUM_DA_CHANNELS)
        return 0;
    return da_channels[channel_index].amplitude;
}

/**
 * @brief 设置指定通道的相位 (仅更新内存配置)
 * @param channel_index: 目标通道索引 (0 或 1)
 * @param angle: 相位值 (单位: 度, 0-359)
 */
void DA_SetPhase(uint8_t channel_index, uint16_t angle)
{
    if (channel_index >= NUM_DA_CHANNELS)
        return;
    da_channels[channel_index].phase = angle;
}

/**
 * @brief 获取指定通道的当前相位配置
 * @param channel_index: 目标通道索引 (0 或 1)
 * @return uint16_t: 当前配置的相位值(度)。若通道索引无效，则返回0。
 */
uint16_t DA_GetPhase(uint8_t channel_index)
{
    if (channel_index >= NUM_DA_CHANNELS)
        return 0;
    return da_channels[channel_index].phase;
}

/**
 * @brief 设置指定通道的波形类型 (仅更新内存配置)
 * @param channel_index: 目标通道索引 (0 或 1)
 * @param wave: 波形类型 (枚举 Waveform_t)
 */
void DA_SetWaveform(uint8_t channel_index, Waveform_t wave)
{
    if (channel_index >= NUM_DA_CHANNELS)
        return;
    da_channels[channel_index].waveform = wave;
}

/**
 * @brief 获取指定通道的当前波形类型配置
 * @param channel_index: 目标通道索引 (0 或 1)
 * @return Waveform_t: 当前配置的波形类型。若通道索引无效，则返回WAVE_SINE。
 */
Waveform_t DA_GetWaveform(uint8_t channel_index)
{
    if (channel_index >= NUM_DA_CHANNELS)
        return WAVE_SINE;
    return da_channels[channel_index].waveform;
}

/**
 * @brief 设置指定通道的全部配置参数（推荐的便捷API）
 * @details
 * 此函数通过调用一系列独立的Set函数，一次性更新通道的所有参数。
 * 这种实现方式提升了代码的复用性和可维护性。
 * @param  channel_index: DA通道索引 (0 for DA1, 1 for DA2)
 * @param  freq: 频率 (Hz)
 * @param  amp: 幅度
 * @param  angle: 相位 (0-359度)
 * @param  wave: 波形类型
 */
void DA_SetConfig(uint8_t channel_index, float freq, uint16_t amp, uint16_t angle, Waveform_t wave)
{
    // 委托给各个独立的Set函数来完成配置更新
    DA_SetFREQ(channel_index, freq);
    DA_SetAmp(channel_index, amp);
    DA_SetPhase(channel_index, angle);
    DA_SetWaveform(channel_index, wave);
}

/**
 * @brief 将内存中的配置应用到硬件
 * @details
 * 此函数是连接软件配置与硬件行为的核心。它负责：
 * 1. 请求FPGA暂停波形生成，以确保寄存器更新的原子性和安全性。
 * 2. 从 `da_channels` 数组中读取两个通道的完整配置。
 * 3. 执行必要的数学计算，如频率控制字和相位映射。
 * 4. 将计算结果写入FPGA内对应的硬件寄存器。
 * 5. 将波形类型参数打包写入组合寄存器。
 * 6. 请求FPGA依据新配置重新生成波形。
 * @note  所有通过Set/SetConfig的修改，必须调用此函数后才能在硬件上生效。
 */
void DA_Apply_Settings(void)
{
    // 1. 请求FPGA暂停输出，以安全更新寄存器
    DA_FPGA_STOP();

    // 2. --- 通道0 (DA1) 参数计算与写入 ---
    // 根据NCO(数控振荡器)原理计算32位频率控制字M
    unsigned int M_DA1 = DA_FREQ_CONSTANT * da_channels[0].frequency / FPGA_BASE_CLK * DA_ROM_SIZE;
    // 将32位的M值拆分为高16位和低16位写入硬件寄存器
    DA1_H = M_DA1 >> 16;
    DA1_L = M_DA1 & 0x0000FFFF;
    // 写入幅度寄存器
    DA1_AMP = da_channels[0].amplitude;
    // 将0-359度的相位归一化，并映射到FPGA内部的相位分辨率 (例如 0-1023)
    uint16_t normalized_angle1 = da_channels[0].phase % 360;
    DA1_PHASE = (uint16_t)(roundf(normalized_angle1 * (DA_ROM_SIZE / 360.0f)));

    // 3. --- 通道1 (DA2) 参数计算与写入 ---
    unsigned int M_DA2 = DA_FREQ_CONSTANT * da_channels[1].frequency / FPGA_BASE_CLK * DA_ROM_SIZE;
    DA2_H = M_DA2 >> 16;
    DA2_L = M_DA2 & 0x0000FFFF;
    DA2_AMP = da_channels[1].amplitude;
    uint16_t normalized_angle2 = da_channels[1].phase % 360;
    DA2_PHASE = (uint16_t)(roundf(normalized_angle2 * (DA_ROM_SIZE / 360.0f)));

    // 4. --- 组合寄存器写入 ---
    // FPGA DA_PARAMETER_DEAL expects: high 8 bits = DA1, low 8 bits = DA2.
    DA_WAVEFORM = (da_channels[0].waveform << 8) | (da_channels[1].waveform);

    // 5. 请求FPGA依据新配置重新生成波形
    DA_FPGA_START();
}

// =================================================================================
//   [DEPRECATED] 废弃的测试功能
// =================================================================================
// 以下代码为早期开发阶段的测试功能，现已废弃，不应在正式代码中调用。
// =================================================================================

/**
 * @brief 用于非阻塞延时的计时器变量 (配合已废弃的wave_test)
 */
static uint32_t wave_tick = 0;

/**
 * @brief  [已废弃] 波形变换测试函数
 * @details 原用于演示和验证波形切换功能。每隔3秒，在所有预设波形之间循环切换。
 * @deprecated 该函数为早期调试功能，现已不再使用。相关功能已迁移或移除。
 */
static void wave_test(void)
{
    // uwTick是一个全局的毫秒计时器变量
    extern volatile uint32_t uwTick;

    if (uwTick - wave_tick < 3000)
    {
        return;
    }
    wave_tick = uwTick;

    static Waveform_t current_waveform = WAVE_SINE;

    switch (current_waveform)
    {
    case WAVE_SINE:
        current_waveform = WAVE_SQUARE;
        break;
    case WAVE_SQUARE:
        current_waveform = WAVE_TRIANGLE;
        break;
    case WAVE_TRIANGLE:
        current_waveform = WAVE_SAWTOOTH;
        break;
    case WAVE_SAWTOOTH:
    default:
        current_waveform = WAVE_SINE;
        break;
    }

    // 将两个通道设置为相同的测试波形
    DA_SetWaveform(0, current_waveform);
    DA_SetWaveform(1, current_waveform);

    // 将新配置应用到硬件
    DA_Apply_Settings();
}
