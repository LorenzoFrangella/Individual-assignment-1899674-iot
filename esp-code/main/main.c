#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"

struct samplig_tools{
  int sampling_frequency;
  StreamBufferHandle_t buffer_handler;
};

#include "fft_config.h"
#include "wifi.h"
#include "mqtt.h"
#include "sampling.h"

void message_sender(void* args){
  struct samplig_tools* tool_argument = args;
  int sampling_frequency = tool_argument->sampling_frequency;
  StreamBufferHandle_t buffer_handler = tool_argument->buffer_handler;

  uint32_t buffer_receiver[sampling_frequency*5];
  uint32_t sum =0;
  while(1){
  printf("waiting for a buffer");
  xStreamBufferReceive(buffer_handler,buffer_receiver,sizeof(buffer_receiver),portMAX_DELAY);

  printf("received buffer!");
  sum=0;
  for(int i=0;i<(sampling_frequency*5);i++){
    //printf("%ld\n",buffer_receiver[i]);
    sum+=buffer_receiver[i];
  }
  float mean = sum / (sampling_frequency*5);
  printf("mean of received buffer: %f\n",mean);

  }

}



void app_main(){
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }

    
    
    int max_freq = exectute_fft(NULL);

    

    StreamBufferHandle_t stream_average_handler = xStreamBufferCreate((sizeof(uint32_t)*5*max_freq),(sizeof(uint32_t)*5*max_freq));
    struct samplig_tools tools1 = {max_freq,stream_average_handler};

    if(max_freq!=0){
      xTaskCreatePinnedToCore(&sampling_task,"sampling_task",1024*30,&tools1,5,NULL,0);
      xTaskCreatePinnedToCore(&message_sender,"message_sender_task",1024*30,&tools1,5,NULL,1);
      //sampling_task(&tools1);
    }
    else{
      printf("no signal detected restart");
    }


    
    
    wifi_init_sta();

    mqtt_app_start();
    

}
