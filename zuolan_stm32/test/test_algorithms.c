#include "my_fft.h"
#include "my_filter.h"
#include "phase_measure.h"

#define FFT_LENGTH 1024   // 定义FFT长度
#define INPUT_LENGTH 1024 // 定义输入信号长度

int main(void)
{
    // 定义输入信号、FIR滤波输出和FFT幅值结果
    float input_signal[INPUT_LENGTH];    // 输入信号数组
    float filtered_signal[INPUT_LENGTH]; // FIR滤波后的信号
    float fft_magnitude[FFT_LENGTH];     // FFT计算得到的幅值

    // 初始化模拟输入信号（正弦波 ）
    for (int i = 0; i < INPUT_LENGTH; i++)
    {
        input_signal[i] = 1.0f * arm_sin_f32(2 * 3.14159265f * i / FFT_LENGTH);
    }

    // 使用FIR低通滤波对信号进行处理
    arm_fir_f32_lp(input_signal, INPUT_LENGTH, filtered_signal);

    // 对FIR滤波后的信号进行FFT计算
    perform_fft(filtered_signal, fft_magnitude, FFT_LENGTH);

    calculate_phase_diff(filtered_signal, fft_magnitude, FFT_LENGTH);
    return 0;
}