#ifndef __DS18B20_H
#define __DS18B20_H

#include "stm32f1xx_hal.h"

/**
 * @brief 初始化 DS18B20（启动一次温度转换）
 * @retval 1: 存在设备  0: 未检测到
 */
uint8_t DS18B20_Init(void);

/**
 * @brief 读取当前温度*100（单位：℃）
 * @param temp 输出温度指针
 * @retval 1: 成功  0: 失败
 */
uint8_t DS18B20_ReadTemperature(int16_t *temp);

#endif
