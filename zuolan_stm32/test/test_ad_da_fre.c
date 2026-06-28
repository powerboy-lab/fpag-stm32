#include "ad_measure.h"
#include "freq_measure.h"
#include "da_output.h"
void main()
{
    vpp_adc_parallel(1000, 0);      // 设置AD1采样频率为1000Hz,AD2采样频率为0（不采样）
    vpp_adc_parallel(1000, 1000);   // 设置AD1采样频率为1000Hz,AD2采样频率为1000Hz
    fre_measure_ad1();              // 测量AD1的频率
    fre_measure_ad2();              // 测量AD2的频率
    vpp_adc_parallel(freq1, freq2); // 设置AD1和AD2的采样频率为freq1和freq2
    DA1_OUT(1000);                  // DA1输出1000Hz
    DA2_OUT(1000);                  // DA2输出1000Hz
}