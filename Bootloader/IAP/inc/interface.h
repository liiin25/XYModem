#ifndef INTERFACE_H
#define INTERFACE_H
#include "stm32f10x.h"

#define USE_CIPHERTEXT 1
#if USE_CIPHERTEXT
    #define AES_KEY "0123456789101112"  /* 密钥 必须是16字节 */
    #define AES_IV  "ABCDEFGHIJKLMNOP"  /* 向量 必须是16字节 */
#endif

#define IMAGE_DEST_ADDR (0x08003000)    /* App起始地址 */
#define IMAGE_MAX_SIZE  (0x7D000)       /* App可用空间 */
#define FLASH_PAGE_SIZE (0x800)         /* Flash页大小 */

typedef void (*pFunction)(void);

typedef enum {
    interfaceOK = 0,
    interfaceError,
} interfaceStatus;


void usartInit(void);
void ledInit(void);
void systickInt(void);
void ledStatus(uint8_t status);
uint16_t readFlag(void);
void writeFlag(uint16_t flag);
interfaceStatus sendByte(uint8_t char1);
interfaceStatus readByte(uint8_t *pChar, const uint8_t *timeout);
interfaceStatus sendString(const uint8_t *pString);
interfaceStatus readString(char *pString, uint8_t max);
interfaceStatus lockDev(void);
interfaceStatus unlockDev(void);
interfaceStatus progDev(uint32_t dest, const uint32_t *pSrc, uint16_t srcSize);
interfaceStatus eraseDev(uint32_t pageAddr);
interfaceStatus jump2App(void);

#endif
