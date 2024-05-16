static const char *TAG_SENDER = "SENDER_TASK";

void message_sender(void* args){

  struct sender_tools* tool_argument = args;
  int sampling_frequency = tool_argument->sampling_frequency;
  StreamBufferHandle_t buffer_handler = tool_argument->buffer_handler;
  esp_mqtt_client_handle_t sender_handler = tool_argument->sender_mqtt_handler;
  

  uint32_t buffer_receiver[sampling_frequency*5];
  uint32_t sum =0;
  while(1){
  xStreamBufferReceive(buffer_handler,buffer_receiver,sizeof(buffer_receiver),portMAX_DELAY);

  ESP_LOGI(TAG_SENDER,"received buffer!");
  sum=0;
  for(int i=0;i<(sampling_frequency*5);i++){
    //printf("%ld\n",buffer_receiver[i]);
    sum+=buffer_receiver[i];
  }
  float mean = sum / (sampling_frequency*5);
  ESP_LOGI(TAG_SENDER,"mean of received buffer: %f",mean);
  char msg[32];
  snprintf(msg,sizeof(msg),"avg: %f",mean);
  int msg_id = esp_mqtt_client_publish(sender_handler, CONFIG_MQTT_TOPIC, msg, 0, 1, 0);


  }

}

void rtt_measures(esp_mqtt_client_handle_t client){
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