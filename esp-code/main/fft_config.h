#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "soc/uart_struct.h"
#include <math.h>
#include <driver/adc.h>
#include "esp_adc_cal.h"
#include <inttypes.h>
#include "esp_dsp.h"

static const char *TAG = "fft_operation";
static esp_adc_cal_characteristics_t adc1_chars;


#define N_SAMPLES 4096
int N = N_SAMPLES;
// Input test array
__attribute__((aligned(16)))
float x1[N_SAMPLES];
__attribute__((aligned(16)))
float x2[N_SAMPLES];
// Window coefficients
__attribute__((aligned(16)))
float wind[N_SAMPLES];
// working complex array
__attribute__((aligned(16)))
float y_cf[N_SAMPLES * 2];
float result[N_SAMPLES];
// Pointers to result arrays
float *y1_cf = &y_cf[0];
float *y2_cf = &y_cf[N_SAMPLES];

// Sum of y1 and y2
__attribute__((aligned(16)))
float sum_y[N_SAMPLES / 2];



float compute_array_mean(float* array, int lenght){
    float mean =0;
    for(int i=0;i<lenght;i++){
        mean+=array[i];
    }
    mean = mean/lenght; 
    return mean;
}

double compute_array_std_dev(float* array,float mean, int lenght){
    double sum =0;
    for(int i=0;i<lenght;i++){
        sum+=pow((array[i]-mean),2);
    }
    sum = sum / lenght;
    
    return sqrt(sum);

}



int exectute_fft(void* arg){
    int sampling_interval = configTICK_RATE_HZ / CONFIG_SAMPLING_FREQUENCY;
    ESP_LOGI(TAG, "sampling each %d msec \n",sampling_interval);
    esp_err_t ret;
    ESP_LOGI(TAG, "Start Example.");
    ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    uint32_t voltage;

    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 0, &adc1_chars);
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_12));
    if (ret  != ESP_OK) {
        ESP_LOGE(TAG, "Not possible to initialize FFT. Error = %i", ret);
        return 0;
    }

    // Generate hann window
    dsps_wind_hann_f32(wind, N);

    

    for(int i=0; i<N_SAMPLES;i++){
        voltage = esp_adc_cal_raw_to_voltage(adc1_get_raw(ADC1_CHANNEL_6), &adc1_chars);
        x1[i] = (float) voltage;
        x2[i]=0;
        
        vTaskDelay(pdMS_TO_TICKS(sampling_interval));
        //printf("%" PRIu32 "\n", voltage);
    }
    // Generate input signal for x2 A=0.1,F=0.2
   //dsps_tone_gen_f32(x2, N, 1.0, 3, 0);

    // Convert two input vectors to one complex vector
    for (int i = 0 ; i < N ; i++) {
        y_cf[i * 2 + 0] = x1[i] * wind[i];
        y_cf[i * 2 + 1] = x2[i] * wind[i];
    }
    // FFT
    unsigned int start_b = dsp_get_cpu_cycle_count();
    dsps_fft2r_fc32(y_cf, N);
    unsigned int end_b = dsp_get_cpu_cycle_count();
    // Bit reverse
    dsps_bit_rev_fc32(y_cf, N);
    // Convert one complex vector to two complex vectors
    dsps_cplx2reC_fc32(y_cf, N);

    for (int i = 0 ; i <N  / 2 ; i++) { 
        y1_cf[i] = 10 * log10f((y1_cf[i * 2 + 0] * y1_cf[i * 2 + 0] + y1_cf[i * 2 + 1] * y1_cf[i * 2 + 1]) / N);
        y2_cf[i] = 01 * log10f((y2_cf[i * 2 + 0] * y2_cf[i * 2 + 0] + y2_cf[i * 2 + 1] * y2_cf[i * 2 + 1]) / N);

        // Simple way to show two power spectrums as one plot
        sum_y[i] = fmax(y1_cf[i], y2_cf[i]);
    }
    

    // Show power spectrum in 64x10 window from -100 to 0 dB from 0..N/4 samples
    ESP_LOGW(TAG, "Signal x1");
    dsps_view(y1_cf, N / 2, 64, 10,  0, 100, '|');
    /*ESP_LOGW(TAG, "Signal x2");
    dsps_view(y2_cf, N / 2, 64, 10,  -60, 40, '|');
    ESP_LOGW(TAG, "Signals x1 and x2 on one plot");
    dsps_view(sum_y, N / 2, 64, 10,  -60, 40, '|');
    ESP_LOGI(TAG, "FFT for %i complex points take %i cycles", N, end_b - start_b);*/

    float mean = compute_array_mean(y1_cf,N/2);
    
    double std_dev= compute_array_std_dev(y1_cf,mean,N/2);

    printf("The spectrogram mean value is: %f\n",mean);
    printf("The spectrogram standard deviation is: %lf\n",std_dev);
    
    
    double z_score;
    int max_rel_freq=0;
    for(int i=0;i<N/2;i++){
        z_score = (y1_cf[i]-mean)/std_dev;
        if(z_score > CONFIG_ZSCORE_THRESHOLD){
            printf("outlier at frequency: %d Hz with z-score: %lf\n",((i*CONFIG_SAMPLING_FREQUENCY)/N_SAMPLES), z_score);
            max_rel_freq = (i*CONFIG_SAMPLING_FREQUENCY)/N_SAMPLES;
        }
    }
    ESP_LOGW(TAG,"sampling frequency will be set to: %d \n",2*max_rel_freq);

    return (max_rel_freq*2);


}