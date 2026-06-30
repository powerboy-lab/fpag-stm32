#ifndef __SPECTRUM_ANALYZER_H__
#define __SPECTRUM_ANALYZER_H__

#include "bsp_system.h"
#include "arm_math.h"

#define SPECTRUM_FFT_LENGTH FFT_LENGTH
#define SPECTRUM_VOFA_POINTS 128U

typedef struct
{
    float sampling_freq;
    float freq_resolution;
    float dc_magnitude;
    float peak_frequency_raw;
    float peak_frequency;
    float peak_magnitude;
    float thd_percent;
    float thd_n_percent;
    float sinad_db;
    uint16_t peak_bin;
} spectrum_result_t;

void spectrum_analyzer_init(void);
void spectrum_analyzer_set_sampling_freq(float sampling_freq);
float spectrum_analyzer_get_sampling_freq(void);
void spectrum_analyzer_analyze(const float *input, uint16_t length, spectrum_result_t *result);
uint16_t spectrum_analyzer_copy_bins(float *dst, uint16_t max_points);
void spectrum_analyzer_print_uart(UART_HandleTypeDef *huart, const spectrum_result_t *result);
void spectrum_analyzer_send_firewater(UART_HandleTypeDef *huart, const spectrum_result_t *result);
float spectrum_analyzer_get_peak_frequency(void);

#endif
