#ifndef __DS18B20_H
#define __DS18B20_H

#include "stm32f1xx_hal.h"

#define MAX_DEVICES 2

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

/**
 * @brief 读取ROM
 */
uint8_t DS18B20_SearchROM(uint8_t roms[MAX_DEVICES][8], uint8_t *count);

/**
 * @brief 读取指定ROM的温度（阻塞，内部自动启动下一次转换）
 */
uint8_t DS18B20_ReadTemperatureByROM(uint8_t rom[8], int16_t *temp);

/**
 * 配置精度最高
 * @param rom ROM
 */
void DS18B20_Config12Bit_ByROM(uint8_t rom[8]);

#endif
