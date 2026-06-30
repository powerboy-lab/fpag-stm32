#include "spectrum_analyzer.h"
#include "my_usart.h"
#include <math.h>
#include <string.h>

static arm_rfft_fast_instance_f32 s_rfft;
static float s_window[SPECTRUM_FFT_LENGTH];
static float s_input[SPECTRUM_FFT_LENGTH];
static float s_fft_output[SPECTRUM_FFT_LENGTH];
static float s_magnitude[SPECTRUM_FFT_LENGTH / 2U + 1U];
static float s_sampling_freq = 48000000.0f;
static uint8_t s_ready = 0U;
static float s_last_peak_frequency = 0.0f;

static void spectrum_build_window(void)
{
    for (uint16_t i = 0; i < SPECTRUM_FFT_LENGTH; i++)
    {
        s_window[i] = 0.5f * (1.0f - arm_cos_f32(2.0f * 3.14159265f * (float)i / (float)(SPECTRUM_FFT_LENGTH - 1U)));
    }
}

static void spectrum_ensure_ready(void)
{
    if (s_ready)
    {
        return;
    }

    arm_rfft_fast_init_f32(&s_rfft, SPECTRUM_FFT_LENGTH);
    spectrum_build_window();
    s_ready = 1U;
}

static void spectrum_compute_magnitude(void)
{
    const float scale = 2.0f / (float)SPECTRUM_FFT_LENGTH;

    s_magnitude[0] = fabsf(s_fft_output[0]) * scale;
    s_magnitude[SPECTRUM_FFT_LENGTH / 2U] = fabsf(s_fft_output[1]) * scale;

    for (uint16_t k = 1; k < SPECTRUM_FFT_LENGTH / 2U; k++)
    {
        const float re = s_fft_output[2U * k];
        const float im = s_fft_output[2U * k + 1U];
        s_magnitude[k] = sqrtf(re * re + im * im) * scale;
    }
}

static uint16_t spectrum_peak_index(void)
{
    uint16_t peak_bin = 1U;
    float peak_mag = s_magnitude[1];

    for (uint16_t i = 2U; i < SPECTRUM_FFT_LENGTH / 2U; i++)
    {
        if (s_magnitude[i] > peak_mag)
        {
            peak_mag = s_magnitude[i];
            peak_bin = i;
        }
    }

    return peak_bin;
}

static float spectrum_parabolic_peak(uint16_t peak_bin)
{
    if (peak_bin <= 1U || peak_bin >= (SPECTRUM_FFT_LENGTH / 2U - 1U))
    {
        return (float)peak_bin * s_sampling_freq / (float)SPECTRUM_FFT_LENGTH;
    }

    const float y1 = s_magnitude[peak_bin - 1U];
    const float y2 = s_magnitude[peak_bin];
    const float y3 = s_magnitude[peak_bin + 1U];
    const float denom = y1 - 2.0f * y2 + y3;

    if (fabsf(denom) < 1e-12f)
    {
        return (float)peak_bin * s_sampling_freq / (float)SPECTRUM_FFT_LENGTH;
    }

    float delta = 0.5f * (y1 - y3) / denom;
    if (delta > 0.5f)
    {
        delta = 0.5f;
    }
    else if (delta < -0.5f)
    {
        delta = -0.5f;
    }

    return ((float)peak_bin + delta) * s_sampling_freq / (float)SPECTRUM_FFT_LENGTH;
}

static float spectrum_calculate_thd(float fundamental_freq)
{
    const float freq_resolution = s_sampling_freq / (float)SPECTRUM_FFT_LENGTH;
    const uint16_t fundamental_bin = (uint16_t)(fundamental_freq / freq_resolution + 0.5f);

    if (fundamental_bin < 1U || fundamental_bin >= SPECTRUM_FFT_LENGTH / 2U)
    {
        return 0.0f;
    }

    float fundamental_power = s_magnitude[fundamental_bin] * s_magnitude[fundamental_bin];
    const float leakage_weight = 0.3f;

    if (fundamental_bin > 1U)
    {
        fundamental_power += leakage_weight * s_magnitude[fundamental_bin - 1U] * s_magnitude[fundamental_bin - 1U];
    }
    if (fundamental_bin + 1U < SPECTRUM_FFT_LENGTH / 2U)
    {
        fundamental_power += leakage_weight * s_magnitude[fundamental_bin + 1U] * s_magnitude[fundamental_bin + 1U];
    }

    float noise_floor = 0.0f;
    for (uint16_t i = SPECTRUM_FFT_LENGTH / 4U; i < SPECTRUM_FFT_LENGTH / 2U; i++)
    {
        noise_floor += s_magnitude[i] * s_magnitude[i];
    }
    noise_floor = sqrtf(noise_floor / (float)(SPECTRUM_FFT_LENGTH / 4U)) * 2.0f;

    float harmonic_power = 0.0f;
    for (uint8_t harmonic = 2U; harmonic <= 10U; harmonic++)
    {
        const uint16_t harmonic_bin = (uint16_t)(harmonic * fundamental_bin);
        if (harmonic_bin >= SPECTRUM_FFT_LENGTH / 2U)
        {
            break;
        }

        const float harmonic_mag = s_magnitude[harmonic_bin];
        if (harmonic_mag <= noise_floor)
        {
            continue;
        }

        float power = harmonic_mag * harmonic_mag;
        if (harmonic_bin > 1U && s_magnitude[harmonic_bin - 1U] > noise_floor)
        {
            power += 0.15f * s_magnitude[harmonic_bin - 1U] * s_magnitude[harmonic_bin - 1U];
        }
        if (harmonic_bin + 1U < SPECTRUM_FFT_LENGTH / 2U && s_magnitude[harmonic_bin + 1U] > noise_floor)
        {
            power += 0.15f * s_magnitude[harmonic_bin + 1U] * s_magnitude[harmonic_bin + 1U];
        }
        harmonic_power += power;
    }

    if (fundamental_power <= noise_floor * noise_floor)
    {
        return 0.0f;
    }

    return sqrtf(harmonic_power) / sqrtf(fundamental_power) * 100.0f;
}

static float spectrum_calculate_thd_n(float fundamental_freq)
{
    const float freq_resolution = s_sampling_freq / (float)SPECTRUM_FFT_LENGTH;
    const uint16_t fundamental_bin = (uint16_t)(fundamental_freq / freq_resolution + 0.5f);

    if (fundamental_bin < 1U || fundamental_bin >= SPECTRUM_FFT_LENGTH / 2U)
    {
        return 0.0f;
    }

    float fundamental_power = s_magnitude[fundamental_bin] * s_magnitude[fundamental_bin];
    const float leakage_weight = 0.3f;

    if (fundamental_bin > 1U)
    {
        fundamental_power += leakage_weight * s_magnitude[fundamental_bin - 1U] * s_magnitude[fundamental_bin - 1U];
    }
    if (fundamental_bin + 1U < SPECTRUM_FFT_LENGTH / 2U)
    {
        fundamental_power += leakage_weight * s_magnitude[fundamental_bin + 1U] * s_magnitude[fundamental_bin + 1U];
    }

    float total_power = 0.0f;
    for (uint16_t i = 1U; i < SPECTRUM_FFT_LENGTH / 2U; i++)
    {
        total_power += s_magnitude[i] * s_magnitude[i];
    }

    const float distortion_noise_power = total_power - fundamental_power;
    if (fundamental_power <= 0.0f || distortion_noise_power < 0.0f)
    {
        return 0.0f;
    }

    return sqrtf(distortion_noise_power) / sqrtf(fundamental_power) * 100.0f;
}

void spectrum_analyzer_init(void)
{
    spectrum_ensure_ready();
}

void spectrum_analyzer_set_sampling_freq(float sampling_freq)
{
    if (sampling_freq > 0.0f)
    {
        s_sampling_freq = sampling_freq;
    }
}

float spectrum_analyzer_get_sampling_freq(void)
{
    return s_sampling_freq;
}

void spectrum_analyzer_analyze(const float *input, uint16_t length, spectrum_result_t *result)
{
    if (result == NULL)
    {
        return;
    }

    spectrum_ensure_ready();
    memset(s_input, 0, sizeof(s_input));
    memset(s_fft_output, 0, sizeof(s_fft_output));
    memset(s_magnitude, 0, sizeof(s_magnitude));

    const uint16_t copy_len = (length > SPECTRUM_FFT_LENGTH) ? SPECTRUM_FFT_LENGTH : length;
    for (uint16_t i = 0; i < copy_len; i++)
    {
        s_input[i] = input[i] * s_window[i];
    }

    arm_rfft_fast_f32(&s_rfft, s_input, s_fft_output, 0);
    spectrum_compute_magnitude();

    const uint16_t peak_bin = spectrum_peak_index();
    const float peak_freq_raw = spectrum_parabolic_peak(peak_bin);
    const float peak_freq = roundf(peak_freq_raw / 1000.0f) * 1000.0f;
    const float thd = spectrum_calculate_thd(peak_freq);
    const float thd_n = spectrum_calculate_thd_n(peak_freq);
    const float sinad = (thd_n > 0.0f) ? (-20.0f * log10f(thd_n / 100.0f)) : 0.0f;

    result->sampling_freq = s_sampling_freq;
    result->freq_resolution = s_sampling_freq / (float)SPECTRUM_FFT_LENGTH;
    result->dc_magnitude = s_magnitude[0];
    result->peak_bin = peak_bin;
    result->peak_frequency_raw = peak_freq_raw;
    result->peak_frequency = peak_freq;
    result->peak_magnitude = s_magnitude[peak_bin];
    result->thd_percent = thd;
    result->thd_n_percent = thd_n;
    result->sinad_db = sinad;
    s_last_peak_frequency = peak_freq;
}

uint16_t spectrum_analyzer_copy_bins(float *dst, uint16_t max_points)
{
    if (dst == NULL || max_points == 0U)
    {
        return 0U;
    }

    const uint16_t bin_count = (SPECTRUM_FFT_LENGTH / 2U) + 1U;
    const uint16_t copy_count = (max_points < bin_count) ? max_points : bin_count;

    if (copy_count == bin_count)
    {
        memcpy(dst, s_magnitude, sizeof(float) * bin_count);
        return copy_count;
    }

    for (uint16_t i = 0; i < copy_count; i++)
    {
        const float ratio = (copy_count == 1U) ? 0.0f : (float)i / (float)(copy_count - 1U);
        const uint16_t index = (uint16_t)roundf(ratio * (float)(bin_count - 1U));
        dst[i] = s_magnitude[index];
    }

    return copy_count;
}

void spectrum_analyzer_print_uart(UART_HandleTypeDef *huart, const spectrum_result_t *result)
{
    if (huart == NULL || result == NULL)
    {
        return;
    }

    my_printf(huart, "Spectrum\r\n");
    my_printf(huart, "Fs=%.0fHz, Res=%.2fHz\r\n", result->sampling_freq, result->freq_resolution);
    my_printf(huart, "Peak=%.0fHz Raw=%.2fHz Bin=%u Mag=%.6f\r\n",
              result->peak_frequency,
              result->peak_frequency_raw,
              result->peak_bin,
              result->peak_magnitude);
    my_printf(huart, "THD=%.3f%% THD+N=%.3f%% SINAD=%.2fdB DC=%.6f\r\n",
              result->thd_percent,
              result->thd_n_percent,
              result->sinad_db,
              result->dc_magnitude);
}

void spectrum_analyzer_send_firewater(UART_HandleTypeDef *huart, const spectrum_result_t *result)
{
    if (huart == NULL || result == NULL)
    {
        return;
    }

    float bins[SPECTRUM_VOFA_POINTS];
    const uint16_t bin_count = spectrum_analyzer_copy_bins(bins, SPECTRUM_VOFA_POINTS);

    my_printf(huart, "meta:%.6f,%.6f,%.6f,%.6f,%.6f\r\n",
              result->sampling_freq,
              result->peak_frequency,
              result->peak_magnitude,
              result->thd_percent,
              result->sinad_db);

    my_printf(huart, "spec:");
    for (uint16_t i = 0; i < bin_count; i++)
    {
        my_printf(huart, "%s%.6f", (i == 0U) ? "" : ",", bins[i]);
    }
    my_printf(huart, "\r\n");
}

float spectrum_analyzer_get_peak_frequency(void)
{
    return s_last_peak_frequency;
}
