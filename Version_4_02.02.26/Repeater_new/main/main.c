//#include <stdio.h>
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
//#include "lora.h"
//#include "string.h"
//#include "esp_log.h"
//
//#define TAG "LORA"
//
//
//
//
//static void Rx_task () {
//    lora_receive();
//    
//    uint8_t buff[256];
//    int len;
//
//    while (1) {
//    len =  lora_receive_packet(buff, sizeof(buff));
//    if (len > 0 ){
//        buff[len] = 0;
//       ESP_LOGI(TAG, "Packet recived: %s (len = %d)",(char *)buff, len );
//    }
//    vTaskDelay(pdMS_TO_TICKS(10));
//    }
//
//}
//
//void app_main(void)
//{
//
//     lora_init();
//     lora_set_bandwidth  (125000);
//     lora_set_coding_rate (5); 
//     lora_set_frequency (865000000);
//     lora_set_sync_word (0XF3);
//     lora_set_spreading_factor (11);
//     lora_set_tx_power (17);
//     lora_enable_crc();
//
//     ESP_LOGI(TAG, "starting lora in Rx..");
//     //vTaskDelay(pdMS_TO_TICKS(5000));
//     xTaskCreate(Rx_task, "Receiving",4096, NULL, 5, NULL);
//}
//


#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lora.h"

#define TAG "LoRa_RX"

void lora_rx_task(void *pvParameter)
{
    uint8_t buf[25];
    
    uint8_t buff[25];

    
    int len;
   
    ESP_LOGI(TAG, "LoRa Receiver Task Started");

    // Initialize LoRa
    lora_init();
   lora_set_bandwidth  (250000);
   lora_set_coding_rate (5); 
   lora_set_frequency (865000000);
   lora_set_sync_word (0XF3);
   lora_set_spreading_factor (12);
   lora_set_tx_power (17);
   lora_enable_crc();            // Put LoRa in RX continuous mode
   lora_receive();
    while (1) {
        
        len = lora_receive_packet(buf, sizeof(buf));
        if (len > 0) {
            buf[len] = 0; // Null-terminate for printing
            ESP_LOGI(TAG, "Received: %s (len=%d)", (char*)buf, len);
           
            for (int i =  0 ; i < sizeof(buf); i++){
                buff[i] =  buf[i];
            }
            memset(buf, 0, sizeof(buf));
            vTaskDelay(pdTICKS_TO_MS(10));
            printf("Packet sending %s\n len =  %d\n" , (char *)buff, len);
            lora_send_packet(buff,sizeof(buff));
             memset(buff, 0, sizeof(buff));
            vTaskDelay(pdTICKS_TO_MS(10));
            lora_receive();
            
         
    }
    vTaskDelay(pdMS_TO_TICKS(100)); // Small delay to yield CPU
    
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting LoRa RX Example...");
    xTaskCreate(&lora_rx_task, "lora_rx_task", 4096, NULL, 5, NULL);
   
}

