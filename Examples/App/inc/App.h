#ifndef __APP_H_
#define __APP_H_
#include "stm32f10x.h"
#include "string.h"

#define IMAGE_OK    (0x4F4B)

#define JUMP_CMD    (0x0000)
#define JUMP_UPDATE (0x0001)
#define JUMP_UPLOAD (0x0020)
#define JUMP_ERASE  (0x0400)

void appInit(void);
void writeFlag(uint16_t flag);

#endif
