#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "mqtt_client.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"

#include "structs.h"
#include "fft_config.h"
#include "wifi.h"
#include "mqtt.h"
#include "sampling.h"
#include "sender.h"


void app_main(){
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }

    wifi_init_sta();
    esp_mqtt_client_handle_t client = mqtt_app_start();


    if(CONFIG_RTT_TIME_MEASUREMENT)rtt_measures(client);

    int max_freq = CONFIG_SAMPLING_FREQUENCY;

    if(CONFIG_FTT_ENABLED){
      max_freq = exectute_fft(NULL);
    }
    
    
    StreamBufferHandle_t stream_average_handler = xStreamBufferCreate((sizeof(uint32_t)*5*max_freq),(sizeof(uint32_t)*5*max_freq));

    struct samplig_tools tools1 = {max_freq,stream_average_handler};
    struct sender_tools tools2 = {max_freq,stream_average_handler,client};

    if(max_freq!=0){
      int size = 4*(5*max_freq)*sizeof(uint32_t)+2048;
      xTaskCreatePinnedToCore(&sampling_task,"sampling_task",size,&tools1,5,NULL,0);
      xTaskCreatePinnedToCore(&message_sender,"message_sender_task",size,&tools2,5,NULL,1);
      //sampling_task(&tools1);
    }
    else{
      printf("no signal detected restart");
    }
}
