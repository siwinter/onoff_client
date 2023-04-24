/* UART asynchronous example, that uses separate RX and TX tasks

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _UART_H_
#define _UART_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

#define UART_TAG "UART"

static const int RX_BUF_SIZE = 1024;

#define TXD_PIN (GPIO_NUM_4)
#define RXD_PIN (GPIO_NUM_5)

extern void send_lighting_level_set(uint16_t adr, uint16_t level) ;
extern void send_gen_onoff_set(uint16_t adr, bool state) ;

/*
class cDevices {
  private:
    cDevice * aDevice ;
  public:
    cDevices() {aDevice = NULL ;}

    void do(char* msg) {
        printf(msg) ;
    }
}

*/

bool msgReading = false ;
bool msgReady = false ;
int8_t msgIndex = 0 ;

# define bufMax 100
char msg[bufMax] ;
char msgStr[16] ;

uint16_t adr = 0 ;
uint8_t onOffState = 0 ;
uint16_t lightness = 0 ;

uint16_t level = 0;

char * cmdStr = "cmd/" ;
void msgDo(uint8_t nbrOfBytes, uint8_t* inBytes) {
    inBytes[nbrOfBytes] = 0;
    printf((char*)inBytes);

    for (int i=0; i<nbrOfBytes; i++) {
        if (inBytes[i] == '>') {
            msgReading = true ;
            msgReady = false ;
            msgIndex = 0 ; }
        else {
            if (msgReading) {
                if (msgIndex > bufMax) msgReading = false ;
                else if ( (inBytes[i] == 10) || (inBytes[i] == 13)) {
                    msg[msgIndex] = 0 ;
                    msgReady = true ;
                    msgReading = false ;
                }
                else msg[msgIndex ++] = (char)inBytes[i] ;
            }
            int j = 0 ;
            if (msgReady) {
                printf("ready \n") ;
                printf(msg) ;
                for (j=0 ; j<strlen(cmdStr) ; j++) { 
                    if (msg[j] != cmdStr[j]) msgReady = false ; } 
                    }
                printf("\n j = %i \n" , j);
            if (msgReady) {
                char* device = msg + strlen(cmdStr) ;
                printf(" device \n") ;
                printf(device) ;
                j = 0 ;
                while (device[j] != ':') j++ ;
                char* code = device + j+1;
                printf("\n code: ") ;
                printf(code) ;
                if ((code[0] == 'o') && (code[1] == 'n')) send_gen_onoff_set(adr, true) ;
                else if  ((code[0] == 'o') && (code[1] == 'f')) send_gen_onoff_set(adr, false) ;
                else {
                    j = 0 ;
                    level = 0;
                    while (code[j] != 0) {
                        if ((code[j] <'0') || (code[j] > '9')) break;
                        level = (level *10) + code[j++] -'0' ;
                    }
                    send_lighting_level_set(adr, level) ;
                }
            }
            } } }


void evtMsg(uint8_t nbr, uint16_t* msg) {

    ESP_LOGI(UART_TAG, "received event from address %i", msg[0]);
    adr = msg[0] ;
    uint8_t strI = 0 ;
    msgStr[strI++] = '>' ;

    int msgI ;


    for (msgI = 0; msgI < nbr; msgI++) {
        uint16_t aCode = msg[msgI] ;
        char helpStr[5];
        helpStr[4] = 0;
        int8_t codeI=3;
        do {
            helpStr[codeI] = "0123456789ABCDEF"[ aCode% 16];
            aCode = aCode / 16 ;
            codeI-- ;
        } while (aCode > 0) ;

        while (codeI < 3) {
            msgStr[strI++] = helpStr[++codeI] ;
        }

        if(msgI < (nbr-1)) msgStr[strI++] = ';' ;
        else msgStr[strI++] = '!' ; 
    }
    msgStr[strI++] = '\n' ; 
    msgStr[strI] = 0;

    const int len = strlen(msgStr);
    const int txBytes = uart_write_bytes(UART_NUM_1, msgStr, len);
}
/*
void msgWork(uint8_t nbrOfBytes, uint8_t* inBytes) {
    inBytes[nbrOfBytes] = 0 ;

    for (int i=0; i<nbrOfBytes; i++) {
        uint8_t b = inBytes[i] ;
        if (b=='>') msgInit();
        else if (reading) {
            if (b >= '0' && b <= '9') msgCodes[msgIndex] = (msgCodes[msgIndex] * 16) + b - '0';
            else if (b >= 'a' && b <='f') msgCodes[msgIndex] = (msgCodes[msgIndex] * 16) + b - 'a' + 10;
            else if (b >= 'A' && b <='F') msgCodes[msgIndex] = (msgCodes[msgIndex] * 16) + b - 'A' + 10;
            else if (b == ';' ) nextCode() ;
            else if (b == '!' ) msgReady() ;
            else reading = false ;

            if ((codeIndex++) > 4) reading = false; } } }
*/

// char inBuffer[bufMax] ;
uint8_t bI = 0 ;

static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);

    uint8_t* inBuffer = (uint8_t*) malloc(RX_BUF_SIZE+1);
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, inBuffer, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {

            msgDo(rxBytes, inBuffer) ;
            inBuffer[rxBytes] = 0;
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, inBuffer);
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, inBuffer, rxBytes, ESP_LOG_INFO);
        }
    }
    free(inBuffer);
}

void uartInit(void){
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    
    xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
}


#endif