#include "esp01s.h"
#include <string.h>
#include <stdio.h>

/* ===== 私有变量 ===== */
static UART_HandleTypeDef *esp_huart;

static uint8_t rx_buf[ESP_RX_BUF_SIZE];
static uint16_t rx_len = 0;

/* ===== 私有函数 ===== */
static void ESP_SendCmd(const char *cmd)
{
    HAL_UART_Transmit(esp_huart,
                      (uint8_t *)cmd,
                      strlen(cmd),
                      HAL_MAX_DELAY);
}

/* ===== 公共函数 ===== */

int ESP01S_Init(UART_HandleTypeDef *huart,
                const char *ssid,
                const char *pwd,
                const char *ip,
                uint16_t port)
{
    char cmd[128];
    esp_huart = huart;

    HAL_Delay(1500);                 // ESP 上电稳定

    ESP_SendCmd("AT\r\n");
    HAL_Delay(300);

    ESP_SendCmd("ATE0\r\n");
    HAL_Delay(300);

    ESP_SendCmd("AT+CWMODE=1\r\n");
    HAL_Delay(300);

    snprintf(cmd, sizeof(cmd),
             "AT+CWJAP=\"%s\",\"%s\"\r\n",
             ssid, pwd);
    ESP_SendCmd(cmd);
    HAL_Delay(6000);

    snprintf(cmd, sizeof(cmd),
             "AT+CIPSTART=\"TCP\",\"%s\",%d\r\n",
             ip, port);
    ESP_SendCmd(cmd);
    HAL_Delay(2000);

    ESP_SendCmd("AT+CIPMODE=1\r\n");
    HAL_Delay(300);

    ESP_SendCmd("AT+CIPSEND\r\n");
    HAL_Delay(300);

    ESP_SendCmd("reg|1\r\n");

    return 0;
}

void ESP01S_SendData(const uint8_t *data, uint16_t len)
{
    HAL_UART_Transmit(esp_huart,
                      (uint8_t *)data,
                      len,
                      HAL_MAX_DELAY);
}

void ESP01S_RxHandler(uint8_t byte)
{
    rx_buf[rx_len++] = byte;

    if (rx_len >= ESP_RX_BUF_SIZE)
        rx_len = 0;

    /* 预留：
     * 这里可以做 AT 응答 / TCP 下行数据解析
     */
}
