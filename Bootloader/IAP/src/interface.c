#include "interface.h"
#include "XYModem.h"

void usartInit() {

    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO | RCC_APB2Periph_USART1, ENABLE);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // Tx
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // Rx
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_DeInit(USART1);
    USART_InitStructure.USART_BaudRate = 115200;
    /* 注意：使用奇偶校验时数据位要设置成9Bit */
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);

    USART_Cmd(USART1, ENABLE);
}

void ledInit() {

    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOE, &GPIO_InitStructure);
    GPIO_SetBits(GPIOE, GPIO_Pin_5);
}

void systickInt() {

    SysTick_Config(SystemCoreClock / 10);
}

void ledStatus(uint8_t status) {

    if(status) {
        GPIO_ResetBits(GPIOE, GPIO_Pin_5);
    } else {
        GPIO_SetBits(GPIOE, GPIO_Pin_5);
    }
}

uint16_t readFlag() {
    return BKP_ReadBackupRegister(BKP_DR1);
}

void writeFlag(uint16_t flag) {

    PWR_BackupAccessCmd(ENABLE);
    BKP_WriteBackupRegister(BKP_DR1, flag);
    PWR_BackupAccessCmd(DISABLE);
}

interfaceStatus sendByte(uint8_t char1) {

    USART_ClearFlag(USART1, USART_FLAG_TC);
    USART_SendData(USART1, char1);
    while(!USART_GetFlagStatus(USART1, USART_FLAG_TC)) {
    };

    return interfaceOK;
}

interfaceStatus readByte(uint8_t *pChar, const uint8_t *timeout) {

    while(*timeout) {
        if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE)) {
            *pChar = (uint8_t)USART1->DR;
            return interfaceOK;
        }
    }

    return interfaceError;
}

interfaceStatus sendString(const uint8_t *pString) {

    while(*pString != '\0') {
        USART_ClearFlag(USART1, USART_FLAG_TC);
        USART_SendData(USART1, *pString);
        while(!USART_GetFlagStatus(USART1, USART_FLAG_TC)) {
        };
        pString++;
    }

    return interfaceOK;
}

interfaceStatus readString(char *pString, uint8_t max) {

    uint8_t index = 0;

    while(1) {
        if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE)) {
            pString[index] = (char)USART1->DR;
            if((pString[index++] == '\n') || (index == max - 1)) {
                pString[index] = '\0';
                break;
            }
        }
    }

    return interfaceOK;
}

interfaceStatus lockDev() {

    FLASH_Unlock();
    if(FLASH_EraseOptionBytes() == FLASH_COMPLETE) {
        if(FLASH_EnableWriteProtection(FLASH_WRProt_AllPages) == FLASH_COMPLETE) {
            FLASH_Lock();
            NVIC_SystemReset();
        }
    }
    FLASH_Lock();

    return interfaceError;
}

interfaceStatus unlockDev() {

    if(FLASH_GetWriteProtectionOptionByte() == 0x00000000) {
        FLASH_Unlock();
        if(FLASH_EraseOptionBytes() == FLASH_COMPLETE) {
            if(FLASH_EnableWriteProtection(0x00000007) == FLASH_COMPLETE) {
                FLASH_Lock();
                NVIC_SystemReset();
            }
        }
        FLASH_Lock();
    }

    return interfaceError;
}

interfaceStatus progDev(uint32_t dest, const uint32_t *pSrc, uint16_t srcSize) {

    uint16_t size = srcSize / 4;

    FLASH_Unlock();
    if((dest % FLASH_PAGE_SIZE) == 0) {
        if(FLASH_ErasePage(dest) != FLASH_COMPLETE) {
            FLASH_Lock();
            return interfaceError;
        }
    }

    for(uint16_t i = 0; (i < size) && (dest <= 0x0807FFFC); i++) {
        if(FLASH_ProgramWord(dest, *(const uint32_t *)(pSrc + i)) == FLASH_COMPLETE) {
            if(*(const uint32_t *)dest != *(const uint32_t *)(pSrc + i)) {
                FLASH_Lock();
                return interfaceError;
            }
            dest += 4;
        } else {
            FLASH_Lock();
            return interfaceError;
        }
    }
    FLASH_Lock();

    return interfaceOK;
}

interfaceStatus eraseDev(uint32_t pageAddr) {

    FLASH_Status status = FLASH_COMPLETE;

    FLASH_Unlock();
    status = FLASH_ErasePage(pageAddr);
    if(status != FLASH_COMPLETE) {
        FLASH_Lock();
        return interfaceError;
    }
    FLASH_Lock();

    return interfaceOK;
}

interfaceStatus jump2App() {

    if(((*(__IO uint32_t *)IMAGE_DEST_ADDR) & 0x2FFE0000) == 0x20000000) {

        uint32_t JumpAddress;
        pFunction Jump_To_Application;

        JumpAddress = *(__IO uint32_t *)(IMAGE_DEST_ADDR + 4);
        Jump_To_Application = (pFunction)JumpAddress;
        __set_MSP(*(__IO uint32_t *)IMAGE_DEST_ADDR);
        Jump_To_Application();
    }

    return interfaceError;
}

void SysTick_Handler()
{
    //100ms
    if(timeout > 0) {
        --timeout;
    }
}
