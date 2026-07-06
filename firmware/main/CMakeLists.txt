idf_component_register(
  SRC_DIRS "src"
  INCLUDE_DIRS "include"
  REQUIRES espressif__esp32-camera esp_driver_gpio esp_event esp_http_client esp_http_server esp_netif esp_timer esp-tls esp_wifi nvs_flash
)
