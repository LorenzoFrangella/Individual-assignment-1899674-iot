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


    if(CONFIG_RTT_TIME_MEASUREMENT){
      setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
      
      esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
      esp_sntp_setservername(0, "pool.ntp.org");
      esp_sntp_init();

      while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
      }

      while (1) {
        struct timeval tv_now;
        gettimeofday(&tv_now, NULL);
        struct tm timeinfo;
        localtime_r(&tv_now.tv_sec, &timeinfo);

        char strftime_buf[64];
        char msg[128];
        strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        sprintf(msg,"%s.%06ld", strftime_buf, tv_now.tv_usec);
        int msg_id = esp_mqtt_client_publish(client, "/rtt", msg, 0, 1, 0);
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);
      }
    }

    int max_freq = CONFIG_SAMPLING_FREQUENCY;

    if(CONFIG_FTT_ENABLED){
      max_freq = exectute_fft(NULL);
    }
    
    
    StreamBufferHandle_t stream_average_handler = xStreamBufferCreate((sizeof(uint32_t)*5*max_freq),(sizeof(uint32_t)*5*max_freq));

    struct samplig_tools tools1 = {max_freq,stream_average_handler};
    struct sender_tools tools2 = {max_freq,stream_average_handler,client};

    if(max_freq!=0){
      xTaskCreatePinnedToCore(&sampling_task,"sampling_task",1024*45,&tools1,5,NULL,0);
      xTaskCreatePinnedToCore(&message_sender,"message_sender_task",1024*45,&tools2,5,NULL,1);
      //sampling_task(&tools1);
    }
    else{
      printf("no signal detected restart");
    }
}
