struct samplig_tools{
  int sampling_frequency;
  StreamBufferHandle_t buffer_handler;
};
struct sender_tools{
  int sampling_frequency;
  StreamBufferHandle_t buffer_handler;
  esp_mqtt_client_handle_t sender_mqtt_handler;
};