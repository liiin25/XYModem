#include "App.h"

int main() {

    char cmd[16] = {0};

    appInit();

    while(1) {

        uint8_t index = 0;

        while(1) {
            if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE)) {
                cmd[index] = (char)USART1->DR;
                if((cmd[index++] == '\n') || (index == 15)) {
                    cmd[index] = '\0';
                    break;
                }
            }
        }

        if(strcmp(cmd, "UPDATE\n") == 0) {
            writeFlag(JUMP_UPDATE);
            NVIC_SystemReset();
        }
        if(strcmp(cmd, "ERASE\n") == 0) {
            writeFlag(JUMP_ERASE);
            NVIC_SystemReset();
        }
        if(strcmp(cmd, "BOOTLOADER\n") == 0) {
            writeFlag(JUMP_CMD);
            NVIC_SystemReset();
        }
    }
}

uint8_t timeCount = 0;
char string[] = {"Running"};
void SysTick_Handler(void) {

    timeCount++;
    if(timeCount >= 5) {

        GPIO_WriteBit(GPIOB, GPIO_Pin_5, (BitAction)(1 - GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_5)));
        timeCount = 0;
        while(string[timeCount] != '\0') {
            USART_ClearFlag(USART1, USART_FLAG_TC);
            USART_SendData(USART1, string[timeCount]);
            while(!USART_GetFlagStatus(USART1, USART_FLAG_TC)) {
            };
            timeCount++;
        }
        timeCount = 0;
    }
}
