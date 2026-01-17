#include "ds18b20.h"

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
 * @brief 读取温度（阻塞，内部自动启动下一次转换）
 */
uint8_t DS18B20_ReadTemperature(int16_t *temp)
{
    uint8_t lsb, msb;
    int16_t raw;

    if (!DS18B20_Reset())
        return 0;

    DS18B20_WriteByte(0xCC);   // Skip ROM
    DS18B20_WriteByte(0xBE);   // Read Scratchpad

    lsb = DS18B20_ReadByte();
    msb = DS18B20_ReadByte();

    raw = (msb << 8) | lsb;
    *temp = (raw * 25) / 4;

    /* 启动下一次温度转换 */
    if (DS18B20_Reset())
    {
        DS18B20_WriteByte(0xCC);
        DS18B20_WriteByte(0x44);
    }

    return 1;
}
