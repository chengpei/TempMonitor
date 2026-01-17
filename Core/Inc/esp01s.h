#ifndef __ESP01S_H
#define __ESP01S_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* ===== 可配置参数 ===== */
#define ESP_RX_BUF_SIZE   256

/* ===== ESP01S API ===== */

/**
 * @brief  初始化 ESP-01S 并建立 TCP 透传连接
 * @param  huart   使用的 UART 句柄
 * @param  ssid    WiFi 名称
 * @param  pwd     WiFi 密码
 * @param  ip      服务器 IP
 * @param  port    服务器端口
 * @retval 0 成功，非 0 失败（当前版本恒返回 0）
 */
int ESP01S_Init(UART_HandleTypeDef *huart,
                const char *ssid,
                const char *pwd,
                const char *ip,
                uint16_t port);

/**
 * @brief  发送透传数据（TCP）
 */
void ESP01S_SendData(const uint8_t *data, uint16_t len);

/**
 * @brief  UART 接收中断喂数据
 *         在 HAL_UART_RxCpltCallback 中调用
 */
void ESP01S_RxHandler(uint8_t byte);

#endif
