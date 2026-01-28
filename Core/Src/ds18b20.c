#include "ds18b20.h"
#include <string.h>

/* ========= 用户可配置区 ========= */
#define DS18B20_GPIO_PORT   GPIOA
#define DS18B20_GPIO_PIN    GPIO_PIN_1

/* ========= DQ 操作宏 ========= */
#define DQ_LOW()   HAL_GPIO_WritePin(DS18B20_GPIO_PORT, DS18B20_GPIO_PIN, GPIO_PIN_RESET)
#define DQ_HIGH()  HAL_GPIO_WritePin(DS18B20_GPIO_PORT, DS18B20_GPIO_PIN, GPIO_PIN_SET)
#define DQ_READ()  HAL_GPIO_ReadPin(DS18B20_GPIO_PORT, DS18B20_GPIO_PIN)

/* ========= 私有函数声明 ========= */
static void     DWT_Delay_us(uint32_t us);
static uint8_t  DS18B20_Reset(void);
static void     DS18B20_WriteByte(uint8_t data);
static uint8_t  DS18B20_ReadByte(void);

/* ========= DWT 微秒延时 ========= */
static void DWT_Delay_us(uint32_t us)
{
    uint32_t cycles = us * (SystemCoreClock / 1000000);
    uint32_t start = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < cycles);
}

/* ========= 1-Wire Reset ========= */
static uint8_t DS18B20_Reset(void)
{
    uint8_t presence;

    DQ_LOW();
    DWT_Delay_us(480);
    DQ_HIGH();
    DWT_Delay_us(70);

    presence = !DQ_READ();   // 低电平表示存在
    DWT_Delay_us(410);

    return presence;
}

/* ========= 写 1 字节 ========= */
static void DS18B20_WriteByte(uint8_t data)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        DQ_LOW();

        if (data & 0x01)
        {
            DWT_Delay_us(5);
            DQ_HIGH();
            DWT_Delay_us(55);
        }
        else
        {
            DWT_Delay_us(60);
            DQ_HIGH();
            DWT_Delay_us(5);
        }

        data >>= 1;
    }
}

/* ========= 读 1 字节 ========= */
static uint8_t DS18B20_ReadByte(void)
{
    uint8_t data = 0;

    for (uint8_t i = 0; i < 8; i++)
    {
        DQ_LOW();
        DWT_Delay_us(2);
        DQ_HIGH();

        DWT_Delay_us(10);
        if (DQ_READ())
        {
            data |= (1 << i);   // LSB first
        }

        DWT_Delay_us(50);
    }

    return data;
}

/* 读取单个位 */
uint8_t DS18B20_ReadBit(void)
{
    uint8_t bit = 0;
    DQ_LOW();
    DWT_Delay_us(2);
    DQ_HIGH();
    DWT_Delay_us(10);
    if(DQ_READ()) bit = 1;
    DWT_Delay_us(50);
    return bit;
}

/* 写单个位 */
void DS18B20_WriteBit(uint8_t bit)
{
    DQ_LOW();
    if(bit)
    {
        DWT_Delay_us(5);
        DQ_HIGH();
        DWT_Delay_us(55);
    }
    else
    {
        DWT_Delay_us(60);
        DQ_HIGH();
        DWT_Delay_us(5);
    }
}

/* =========================================================
 *                 对外接口函数
 * ========================================================= */

/**
 * @brief DS18B20 初始化（启动一次温度转换）
 */
uint8_t DS18B20_Init(void)
{
    if (!DS18B20_Reset())
        return 0;

    DS18B20_WriteByte(0xCC);   // Skip ROM
    DS18B20_WriteByte(0x44);   // Convert T

    return 1;
}

/**
 * 扫描总线上的所有 DS18B20 ROM
 * @param roms 输出 ROM 数组，每个 ROM 8 字节
 * @param count 输出设备数量
 * @return 1 成功，0 失败
 */
uint8_t DS18B20_SearchROM(uint8_t roms[MAX_DEVICES][8], uint8_t *count)
{
    uint8_t last_discrepancy = 0;
    uint8_t last_device_flag = 0;
    uint8_t rom[8];
    uint8_t device_index = 0;

    *count = 0;
    memset(roms, 0, sizeof(uint8_t) * MAX_DEVICES * 8);

    while(!last_device_flag && device_index < MAX_DEVICES)
    {
        uint8_t id_bit_number = 1;
        uint8_t last_zero = 0;
        uint8_t rom_byte_number = 0;
        uint8_t rom_byte_mask = 1;
        uint8_t search_result = 0;

        if(!DS18B20_Reset()) break;

        DS18B20_WriteByte(0xF0); // Search ROM

        memset(rom, 0, 8);

        while(rom_byte_number < 8)
        {
            // 读一位和补码位
            uint8_t id_bit = DS18B20_ReadBit();
            uint8_t cmp_id_bit = DS18B20_ReadBit();

            if(id_bit == 1 && cmp_id_bit == 1)
            {
                // 无设备响应
                search_result = 0;
                break;
            }

            uint8_t search_direction;

            if(id_bit != cmp_id_bit)
            {
                // 没有冲突，方向就是 id_bit
                search_direction = id_bit;
            }
            else
            {
                // 冲突位
                if(id_bit_number < last_discrepancy)
                    search_direction = (rom[rom_byte_number] & rom_byte_mask) ? 1 : 0;
                else
                    search_direction = (id_bit_number == last_discrepancy) ? 1 : 0;

                if(search_direction == 0)
                    last_zero = id_bit_number;
            }

            // 设置 ROM bit
            if(search_direction)
                rom[rom_byte_number] |= rom_byte_mask;
            else
                rom[rom_byte_number] &= ~rom_byte_mask;

            // 写位到总线
            DS18B20_WriteBit(search_direction);

            id_bit_number++;
            rom_byte_mask <<= 1;
            if(rom_byte_mask == 0)
            {
                rom_byte_number++;
                rom_byte_mask = 1;
            }
        }

        last_discrepancy = last_zero;
        if(last_discrepancy == 0)
            last_device_flag = 1;

        if(search_result != 0 || id_bit_number > 64)
        {
            memcpy(roms[device_index], rom, 8);
            device_index++;
        }
    }

    *count = device_index;
    return (device_index > 0) ? 1 : 0;
}


/**
 * @brief 读取温度（阻塞，内部自动启动下一次转换）
 */
uint8_t DS18B20_ReadTemperature(int16_t *temp)
{
    uint8_t lsb, msb;
    int16_t raw;

    if(!DS18B20_Reset()) return 0;

    DS18B20_WriteByte(0xCC);
    DS18B20_WriteByte(0x44);

    // 等待转换完成 (12-bit 最大750ms)
    HAL_Delay(750);

    if(!DS18B20_Reset()) return 0;
    DS18B20_WriteByte(0xCC);   // Skip ROM
    DS18B20_WriteByte(0xBE);   // Read Scratchpad

    lsb = DS18B20_ReadByte();
    msb = DS18B20_ReadByte();

    raw = (msb << 8) | lsb;
    *temp = (raw * 25) / 4;

    return 1;
}

/**
 * @brief 读取指定ROM的温度（阻塞，内部自动启动下一次转换）
 */
uint8_t DS18B20_ReadTemperatureByROM(uint8_t rom[8], int16_t *temp) {
    if (!DS18B20_Reset()) return 0;

    // 选择探头
    DS18B20_WriteByte(0x55); // Match ROM
    for (int i = 0; i < 8; i++)
        DS18B20_WriteByte(rom[i]);

    // 启动温度转换
    DS18B20_WriteByte(0x44);

    // 等待转换完成 (12-bit 最大750ms)
    HAL_Delay(750);

    // 再次复位 + 选择探头
    if (!DS18B20_Reset()) return 0;
    DS18B20_WriteByte(0x55);
    for (int i = 0; i < 8; i++)
        DS18B20_WriteByte(rom[i]);

    // 读取温度
    DS18B20_WriteByte(0xBE); // Read Scratchpad
    uint8_t lsb = DS18B20_ReadByte();
    uint8_t msb = DS18B20_ReadByte();
    int16_t raw = (msb << 8) | lsb;

    *temp = (raw * 25) / 4; // 0.01°C
    return 1;
}

void DS18B20_Config12Bit_ByROM(uint8_t rom[8])
{
    /* 1. Reset */
    if (!DS18B20_Reset())
        return;

    /* 2. Match ROM */
    DS18B20_WriteByte(0x55);
    for (int i = 0; i < 8; i++)
        DS18B20_WriteByte(rom[i]);

    /* 3. Write Scratchpad */
    DS18B20_WriteByte(0x4E);
    DS18B20_WriteByte(0x4B);   // TH (随意)
    DS18B20_WriteByte(0x46);   // TL (随意)
    DS18B20_WriteByte(0x7F);   // 分辨率，9-bit：0x1F｜0.5 °C｜~94 ms  10-bit：0x3F｜0.25 °C｜~188 ms  11-bit：0x5F｜0.125 °C｜~375 ms  12-bit：0x7F｜0.0625 °C｜~750 ms

    /* 4. Reset */
    if (!DS18B20_Reset())
        return;

    /* 5. Match ROM */
    DS18B20_WriteByte(0x55);
    for (int i = 0; i < 8; i++)
        DS18B20_WriteByte(rom[i]);

    /* 6. Copy Scratchpad -> EEPROM */
    DS18B20_WriteByte(0x48);

    /* EEPROM 写入时间，datasheet 要求 */
    HAL_Delay(20);
}
