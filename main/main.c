#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "esp_http_client.h"

#define ECHO_UART_PORT_NUM UART_NUM_2
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)
#define ECHO_UART_BAUD_RATE   (115200)
#define ECHO_TASK_STACK_SIZE  (CONFIG_EXAMPLE_TASK_STACK_SIZE)

static const char *TAG = "ESPUSB";

void uart_init()
{
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    int intr_alloc_flags = 0;
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM,17,16, ECHO_TEST_RTS, ECHO_TEST_CTS));
    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, 1024 * 2, 0, 0, NULL, intr_alloc_flags)); 
}
void resetALL()
{
    uint8_t hexdata[3] = {0x57,0xAB,0x05};
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[0],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[1],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[2],(size_t)1);
    vTaskDelay(pdMS_TO_TICKS(200));
    ESP_LOGI(TAG,"Reset_Done");
}
void set_USB_Mode (){
    uint8_t hexdata[4] = {0x57,0xAB,0x15,0x06};
    uint8_t recvbyte[10];
    int length = 0;
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[0],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[1],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[2],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[3],(size_t)1);
    vTaskDelay(pdMS_TO_TICKS(20));
    ESP_ERROR_CHECK(uart_get_buffered_data_len(ECHO_UART_PORT_NUM, (size_t*)&length));
    length = uart_read_bytes(ECHO_UART_PORT_NUM,recvbyte,length,100);
    for(int i=0;i<length;i++)
    {
        ESP_LOGI(TAG,"RECV %02x",recvbyte[i]);
    }
    vTaskDelay(pdMS_TO_TICKS(20));
}
void diskConnectionStatus(){
    uint8_t hexdata[3] = {0x57,0xAB,0x30};
    uint8_t recvbyte[10];
    int length = 0;
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[0],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[1],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[2],(size_t)1);
    ESP_ERROR_CHECK(uart_get_buffered_data_len(ECHO_UART_PORT_NUM, (size_t*)&length));
    length = uart_read_bytes(ECHO_UART_PORT_NUM,recvbyte,length,100);
    for(int i=0;i<length;i++)
    {
        ESP_LOGI(TAG,"RECV %02x",recvbyte[i]);
    }
}
void USBdiskMount(){
    uint8_t hexdata[3] = {0x57,0xAB,0x31};
    uint8_t recvbyte[10];
    int length = 0;
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[0],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[1],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[2],(size_t)1);
    while(true)
    {
        ESP_ERROR_CHECK(uart_get_buffered_data_len(ECHO_UART_PORT_NUM, (size_t*)&length));
        length = uart_read_bytes(ECHO_UART_PORT_NUM,recvbyte,length,100);
        ESP_LOGI(TAG,"MOUNT_RESP %02x",recvbyte[0]);
        vTaskDelay(pdMS_TO_TICKS(50));
        if(recvbyte[0] == 0x14)
        {
            break;
        }
       // break;
    }
}
void setFileName()
{
    vTaskDelay(pdMS_TO_TICKS(100)); //Important else file name will give you error
    uint8_t hexdata[4] = {0x57,0xAB,0x2F,0x00};
    uint8_t recvbyte[10];
    int length = 0;
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[0],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[1],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[2],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[2],(size_t)1);
    char* filename = "file.txt";
    ESP_LOGI(TAG,"LEN : %d",strlen(filename));
    uart_write_bytes(ECHO_UART_PORT_NUM,(const char*)filename,strlen(filename));
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[3],(size_t)1);
    vTaskDelay(pdMS_TO_TICKS(30));
    while(true)
    {
        ESP_ERROR_CHECK(uart_get_buffered_data_len(ECHO_UART_PORT_NUM, (size_t*)&length));
        length = uart_read_bytes(ECHO_UART_PORT_NUM,recvbyte,length,100);
        ESP_LOGI(TAG,"FILE_RESP %02x",recvbyte[0]);
        vTaskDelay(pdMS_TO_TICKS(100));
        if(recvbyte[0] == 0x14)
        {
            vTaskDelay(pdMS_TO_TICKS(20));
            break;
        }
       // break;
    }
}
void filecreate(){
    uint8_t hexdata[3] = {0x57,0xAB,0x34};
    uint8_t recvbyte[10];
    int length = 0;
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[0],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[1],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[2],(size_t)1);
    while(true)
    {
        ESP_ERROR_CHECK(uart_get_buffered_data_len(ECHO_UART_PORT_NUM, (size_t*)&length));
        length = uart_read_bytes(ECHO_UART_PORT_NUM,recvbyte,length,100);
        ESP_LOGI(TAG,"create_RESP %02x",recvbyte[0]);
        vTaskDelay(pdMS_TO_TICKS(100));
        if(recvbyte[0] == 0x14)
        {
            break;
        }
    }
}
void fileopen(){
    uint8_t hexdata[3] = {0x57,0xAB,0x32};
    uint8_t recvbyte[10];
    int length = 0;
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[0],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[1],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[2],(size_t)1);
    while(true)
    {
        ESP_ERROR_CHECK(uart_get_buffered_data_len(ECHO_UART_PORT_NUM, (size_t*)&length));
        length = uart_read_bytes(ECHO_UART_PORT_NUM,recvbyte,length,100);
        ESP_LOGI(TAG,"OPEN_RESP %02x",recvbyte[0]);
        vTaskDelay(pdMS_TO_TICKS(100));
        if(recvbyte[0] == 0x14)
        {
            break;
        }
    }
}
void fileappend(){
    uint8_t hexdata[4] = {0x57,0xAB,0x39,0xFF};
    uint8_t recvbyte[10];
    int length = 0;
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[0],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[1],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[2],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[3],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[3],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[3],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[3],(size_t)1);
    while(true)
    {
        ESP_ERROR_CHECK(uart_get_buffered_data_len(ECHO_UART_PORT_NUM, (size_t*)&length));
        length = uart_read_bytes(ECHO_UART_PORT_NUM,recvbyte,length,100);
        ESP_LOGI(TAG,"OPEN_RESP %02x",recvbyte[0]);
        vTaskDelay(pdMS_TO_TICKS(100));
        if(recvbyte[0] == 0x14)
        {
            break;
        }
    }
}
void fileend(){
    uint8_t hexdata[3] = {0x57,0xAB,0x3D};
    uint8_t recvbyte[10];
    int length = 0;
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[0],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[1],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[2],(size_t)1);
    while(true)
    {
        ESP_ERROR_CHECK(uart_get_buffered_data_len(ECHO_UART_PORT_NUM, (size_t*)&length));
        length = uart_read_bytes(ECHO_UART_PORT_NUM,recvbyte,length,100);
        ESP_LOGI(TAG,"END_RESP %02x",recvbyte[0]);
        vTaskDelay(pdMS_TO_TICKS(100));
        if(recvbyte[0] == 0x14)
        {
            break;
        }
        if(recvbyte[0] == 0x1e)
        {
            fileend();
        }
    }
}
void fileclose(){
    uint8_t hexdata[4] = {0x57,0xAB,0x36,0x01};
    uint8_t recvbyte[10];
    int length = 0;
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[0],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[1],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[2],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[3],(size_t)1);
    while(true)
    {
        ESP_ERROR_CHECK(uart_get_buffered_data_len(ECHO_UART_PORT_NUM, (size_t*)&length));
        length = uart_read_bytes(ECHO_UART_PORT_NUM,recvbyte,length,100);
        ESP_LOGI(TAG,"CLOSE_RESP %02x",recvbyte[0]);
        vTaskDelay(pdMS_TO_TICKS(100));
        if(recvbyte[0] == 0x14)
        {
            break;
        }
    }
}
void writedata(char* test_str)
{
    uint8_t hexdata[4] = {0x57,0xAB,0x2D,0x00};
    uint8_t recvbyte[10];
    int length = 0;
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[0],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[1],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[2],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM, (const char*)test_str,strlen(test_str));
    while(true)
    {
        ESP_ERROR_CHECK(uart_get_buffered_data_len(ECHO_UART_PORT_NUM, (size_t*)&length));
        length = uart_read_bytes(ECHO_UART_PORT_NUM,recvbyte,length,100);
        ESP_LOGI(TAG,"SAVE_RESP %02x",recvbyte[0]);
        vTaskDelay(pdMS_TO_TICKS(100));
        if(recvbyte[0] != 0xE0)
        {
            fileend();
            break;
        }
    }
}
void filewrite(char* payload)
{
    vTaskDelay(pdMS_TO_TICKS(100));
    uint8_t hexdata[4] = {0x57,0xAB,0x3C,0x00};
    uint8_t recvbyte[10];
    int length = 0;
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[0],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[1],(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[2],(size_t)1);
    uint8_t len = (uint8_t)strlen(payload);
    ESP_LOGI(TAG,"Length : %02x",len);
    uart_write_bytes(ECHO_UART_PORT_NUM,&len,(size_t)1);
    uart_write_bytes(ECHO_UART_PORT_NUM,&hexdata[3],(size_t)1);
    while(true)
    {
        ESP_ERROR_CHECK(uart_get_buffered_data_len(ECHO_UART_PORT_NUM, (size_t*)&length));
        length = uart_read_bytes(ECHO_UART_PORT_NUM,recvbyte,length,100);
        ESP_LOGI(TAG,"WRITE_RESP %02x",recvbyte[0]);
        vTaskDelay(pdMS_TO_TICKS(100));
        if(recvbyte[0] == 0x1E)
        {
            writedata(payload);
            break;
        }
    }
}
void app_main(void)
{
    uart_init();
    resetALL();
    set_USB_Mode();
    diskConnectionStatus();
    USBdiskMount();
    setFileName();
    filecreate();
    //fileopen();
    ESP_LOGI(TAG,"--------APP--------");
    for(int i=0;i<100;i++)
    {
        char* sample_string = "SR meters that are available in the market which can be useful to measure the ESR of a capacitor. These meters use Alternating current, such as square wave in a specific frequency across the capacitor. Based on the change in frequency of the signal the ESR value of the capacitor can be calculated. An advantage with this method is that, since the ESR is measured directly across the two terminals of a capacitor it can be measured without de-soldering it from the circuit board.Another theoretical way to calculate ESR of the capacitor is to measure the Ripple voltage and Ripple current of the capacitor and then the ratio of both will give the value of ESR in the capacitor. However, a more common ESR measurement model is to apply alternating current source across the capacitor with an additional resistance. A crude circuit to measure ESR is shown belowkaptcha e-white board usb file data is used to write data to the end of the file, without erasing the contents of the file Check that communication with the USB device is possible\n";
        filewrite(sample_string);
        vTaskDelay(pdMS_TO_TICKS(200));
        if(i%10 == 0)
        {
            ESP_LOGI(TAG,"----FILE_CLOSE");
            fileclose();
            vTaskDelay(pdMS_TO_TICKS(200));
            setFileName();
            fileopen();
            fileappend();
        }
    }
    fileclose();
}
