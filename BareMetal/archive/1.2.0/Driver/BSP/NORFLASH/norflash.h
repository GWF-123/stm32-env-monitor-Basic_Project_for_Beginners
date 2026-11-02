#ifndef __norflash_H
#define __norflash_H

#include "./SYSTEM/sys/sys.h"

#define FCS_PIN     GPIO_PIN_12
#define FCS_PORT    GPIOB
#define FCS_CLK_ENABLE()    do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)

#define NORFLASH_CS(x) do{ x ? \
                                  HAL_GPIO_WritePin(FCS_PORT, FCS_PIN, GPIO_PIN_SET) : \
                                  HAL_GPIO_WritePin(FCS_PORT, FCS_PIN, GPIO_PIN_RESET); \
                          }while(0)

/* FLASHÐ¾Æ¬ÁÐ±í */
#define W25Q80      0XEF13          /* W25Q80   Ð¾Æ¬ID */
#define W25Q16      0XEF14          /* W25Q16   Ð¾Æ¬ID */
#define W25Q32      0XEF15          /* W25Q32   Ð¾Æ¬ID */
#define W25Q64      0XEF16          /* W25Q64   Ð¾Æ¬ID */
#define W25Q128     0XEF17          /* W25Q128  Ð¾Æ¬ID */
#define W25Q256     0XEF18          /* W25Q256  Ð¾Æ¬ID */
#define BY25Q64     0X6816          /* BY25Q64  Ð¾Æ¬ID */
#define BY25Q128    0X6817          /* BY25Q128 Ð¾Æ¬ID */
#define NM25Q64     0X5216          /* NM25Q64  Ð¾Æ¬ID */
#define NM25Q128    0X5217          /* NM25Q128 Ð¾Æ¬ID */

/* Ö¸Áî±í */
#define FLASH_WriteEnable           0x06 
#define FLASH_WriteDisable          0x04 
#define FLASH_ReadStatusReg1        0x05 
#define FLASH_ReadStatusReg2        0x35 
#define FLASH_ReadStatusReg3        0x15 
#define FLASH_WriteStatusReg1       0x01 
#define FLASH_WriteStatusReg2       0x31 
#define FLASH_WriteStatusReg3       0x11 
#define FLASH_ReadData              0x03 
#define FLASH_FastReadData          0x0B 
#define FLASH_FastReadDual          0x3B 
#define FLASH_FastReadQuad          0xEB  
#define FLASH_PageProgram           0x02 
#define FLASH_PageProgramQuad       0x32 
#define FLASH_BlockErase            0xD8 
#define FLASH_SectorErase           0x20 
#define FLASH_ChipErase             0xC7 
#define FLASH_PowerDown             0xB9 
#define FLASH_ReleasePowerDown      0xAB 
#define FLASH_DeviceID              0xAB 
#define FLASH_ManufactDeviceID      0x90 
#define FLASH_JedecDeviceID         0x9F 
#define FLASH_Enable4ByteAddr       0xB7
#define FLASH_Exit4ByteAddr         0xE9
#define FLASH_SetReadParam          0xC0 
#define FLASH_EnterQPIMode          0x38
#define FLASH_ExitQPIMode           0xFF

extern uint16_t norflash_TYPE;      /* ¶¨ÒåFLASHÐ¾Æ¬ÐÍºÅ */

//¾²Ì¬º¯Êý
static void Norflash_Wait_Busy(void);               /* µÈ´ý¿ÕÏÐ */
static void Norflash_Send_Address(uint32_t address);/* ·¢ËÍµØÖ· */
static void Norflash_Write_Page(uint8_t *pbuf, uint32_t addr, uint16_t datalen);    /* Ð´Èëpage */
static void Norflash_Write_Nocheck(uint8_t *pbuf, uint32_t addr, uint16_t datalen); /* Ð´flash,²»´ø²Á³ý */

void Norflash_Init(void);
uint16_t Norflash_Read_Id(void);            /* ¶ÁÈ¡FLASH ID */
void Norflash_Write_Enable(void);           /* Ð´Ê¹ÄÜ */
uint8_t Norflash_Read_Sr(uint8_t regno);    /* ¶ÁÈ¡×´Ì¬¼Ä´æÆ÷ */
void Norflash_Write_Sr(uint8_t regno,uint8_t sr);   /* Ð´×´Ì¬¼Ä´æÆ÷ */

void Norflash_Erase_Chip(void);             /* ÕûÆ¬²Á³ý */
void Norflash_Erase_Sector(uint32_t saddr); /* ÉÈÇø²Á³ý */
void Norflash_Read(uint8_t *pbuf, uint32_t addr, uint16_t datalen);     /* ¶ÁÈ¡flash */
void Norflash_Write(uint8_t *pbuf, uint32_t addr, uint16_t datalen);    /* Ð´Èëflash */

#endif
