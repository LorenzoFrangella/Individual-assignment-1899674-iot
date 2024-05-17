void sampling_task(void* args){

  size_t free_stack = uxTaskGetStackHighWaterMark(NULL);
  printf("Free stack space for this task: %d bytes\n", free_stack);

  uint32_t voltage;
  struct samplig_tools* tool_argument = args;
  int sampling_frequency = tool_argument->sampling_frequency;

  StreamBufferHandle_t buffer_handler = tool_argument->buffer_handler;
  uint32_t buffer[sampling_frequency*5];
  printf("space occupied by the buffer: %d bytes\n", sizeof(buffer));
  
  
  int counter=0;
  
  ESP_LOGW("SAMPLING_TASK","Sampling frequency: %d\n", sampling_frequency);
  int sampling_interval = configTICK_RATE_HZ / sampling_frequency;
  //printf("Time interval between each sample: %d\n", sampling_interval);

  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 0, &adc1_chars);

  ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
  ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11));

  while (1)
  {
      voltage = esp_adc_cal_raw_to_voltage(adc1_get_raw(ADC1_CHANNEL_6), &adc1_chars);
      //printf("%" PRIu32 "\n", voltage);
      buffer[counter]=voltage;
      counter++;
      //printf("%d\n",counter);
      if(counter == (sampling_frequency*5)){
        counter =0;
        xStreamBufferSend(buffer_handler,buffer,sizeof(buffer),100);
        ESP_LOGI("SAMPLING_TASK","Buffer sent!");
      }
      vTaskDelay(pdMS_TO_TICKS(sampling_interval));
      
  }

}