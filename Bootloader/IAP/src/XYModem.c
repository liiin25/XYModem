#include "XYmodem.h"
#include "aes.h"
#include "string.h"

uint8_t timeout = 0;
uint8_t packetData[PACKET_1K_SIZE + PACKET_OVERHEAD_SIZE] = {0};

static uint32_t str2Int(const uint8_t *pStr) {

    uint32_t size = 0;

    while(*pStr != '\0') {
        if(*pStr > '9' || *pStr < '0') {
            return 0;
        }

        size = size * 10 + ((*pStr++) - '0');
    }

    return size;
}

static uint16_t packet2CRC(const uint8_t *pPacket, uint16_t packetSize) {

    uint16_t crc = 0x00;

    for(uint16_t i = 0; i < packetSize; i++) {
        crc ^= (uint16_t)(pPacket[i] << 8);
        for(uint16_t j = 0; j < 8; j++) {
            if(crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }

    return crc;
}

static uint16_t packet2CheckSum(const uint8_t *pPacket, uint8_t packetSize) {

    uint16_t checkSum = 0;

    for(uint8_t i = 0; i < packetSize; i++) {
        checkSum += pPacket[i];
    }

    return checkSum;
}

static receiveStatus receivePacket(uint8_t *isXModem, uint8_t *pPacket, uint16_t *pPacketSize, const uint8_t *receiveNum) {

    uint16_t crc = 0;

    *pPacketSize = 0;
    timeout = NAK_TIMEOUT;
    if(readByte(pPacket, &timeout)) {
        return receiveTimeout;
    }

    switch(*pPacket) {
        case SOH:
        case STX: {
            *pPacketSize = (*pPacket == SOH) ? PACKET_SIZE : PACKET_1K_SIZE;
            for(uint16_t i = 1; i < (*pPacketSize + PACKET_OVERHEAD_SIZE); ++i) {
                timeout = NAK_TIMEOUT;
                if(readByte(pPacket + i, &timeout)) {
                    return receiveError;
                }
            }

            if(pPacket[PACKET_NUM_INDEX] != (pPacket[PACKET_CNUM_INDEX] ^ 0xFF)) {
                return receiveError;
            }
            if(pPacket[PACKET_NUM_INDEX] != *receiveNum) {
                return receiveError;
            }

            // clang-format off
            crc = ((pPacket[*pPacketSize + PACKET_CRCH_INDEX] << 8)
                    | pPacket[*pPacketSize + PACKET_CRCL_INDEX]);
            // clang-format on
            if(!(*isXModem & 0x01)) {
                if(packet2CRC(&pPacket[PACKET_DATA_INDEX], *pPacketSize) != crc) {
                    return receiveError;
                }
            } else {
                if(packet2CheckSum(&pPacket[PACKET_DATA_INDEX], PACKET_SIZE) != crc) {
                    return receiveError;
                }
            }

            if(*isXModem == 0x00) {
                *isXModem = pPacket[130] | 0x80;
            }
            break;
        }
        case EOT: {
            return receiveEOT;
        }
        case ABORT1:
        case ABORT2: {
            return receiveAbort;
        }
        default: {
            return receiveError;
        }
    }

    return receiveOK;
}

static iapStatus updateImage() {

    uint8_t *filePtr = 0;
    uint8_t ptrIndex = 0;
    uint8_t isXModem = 0;
    uint8_t receiveNum = 0, requestNum = 0, resendNum = 0;
    uint8_t isFileDone = 0, isSessionDone = 0, isSessionBegin = 0;
    uint8_t fileSizeBuff[SIZE_MAX_LENGTH + 1] = {0};
    uint16_t packetSize = 0;
    uint32_t fileSize = 0;
    uint32_t imageDest = IMAGE_DEST_ADDR;
    iapStatus status = iapOK;

#if USE_CIPHERTEXT
    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, AES_KEY, AES_IV);
#endif

    sendByte(CRC16);
    while((isSessionDone == 0) && (status == iapOK)) {
        ptrIndex = 0;
        isFileDone = 0;
        requestNum = 0;
        receiveNum = 0;
        while((isFileDone == 0) && (status == iapOK)) {
            switch(receivePacket(&isXModem, packetData, &packetSize, &receiveNum)) {
                case receiveOK: {
                    resendNum = 0;
                    if(receiveNum == 0 && ptrIndex == 0) {
                        if(packetData[PACKET_DATA_INDEX] != 0x00) {
                            filePtr = &packetData[PACKET_DATA_INDEX];
                            while(ptrIndex < NAME_MAX_LENGTH) {
                                ptrIndex++;
                                if(*filePtr++ == 0) {
                                    ptrIndex = 0;
                                    break;
                                }
                            }
                            /* file size */
                            while(ptrIndex < SIZE_MAX_LENGTH) {
                                fileSizeBuff[ptrIndex++] = *filePtr++;
                                if(*filePtr == 0) {
                                    fileSizeBuff[ptrIndex] = '\0';
                                    fileSize = str2Int(fileSizeBuff);
                                    break;
                                }
                            }
                            if((fileSize == 0) || (fileSize > IMAGE_MAX_SIZE)) {
                                sendByte(CA);
                                sendByte(CA);
                                status = iapImageLimit;
                            } else {
                                sendByte(ACK);
                                sendByte(CRC16);
                                isSessionBegin = 1;
                            }
                        } else {
                            isFileDone = 1;
                            isSessionDone = 1;
                            sendByte(ACK);
                        }
                    } else {
#if USE_CIPHERTEXT
                        AES_CBC_decrypt_buffer(&ctx, &packetData[PACKET_DATA_INDEX], packetSize);
#endif
                        if(!progDev(imageDest, (const uint32_t *)&packetData[PACKET_DATA_INDEX], packetSize)) {
                            imageDest += packetSize;
                            sendByte(ACK);
                        } else {
                            sendByte(CA);
                            sendByte(CA);
                            status = iapFlashError;
                        }
                    }
                    receiveNum++;
                    ledStatus(receiveNum % 2);
                    break;
                }
                case receiveEOT: {
                    isFileDone = 1;
                    sendByte(ACK);
                    break;
                }
                case receiveAbort: {
                    status = iapAbort;
                    break;
                }
                default: {
                    if(isSessionBegin) {
                        if(resendNum < MAX_RESEND) {
                            resendNum++;
                            sendByte(NAK);
                        } else {
                            sendByte(CA);
                            sendByte(CA);
                            status = iapMaxResend;
                        }
                    } else {
                        if(requestNum < MAX_REQUEST) {
                            requestNum++;
                            sendByte(CRC16);
                        } else {
                            sendByte(CA);
                            sendByte(CA);
                            status = iapMaxRequest;
                        }
                    }
                    break;
                }
            }
        }
    }

    ledStatus(0);
    sendByte(status);

    return status;
}

// static iapStatus uploadImage() {
//     return iapOK;
// }

static iapStatus eraseImage() {

    iapStatus status = iapOK;
    uint16_t nbrOfPage = IMAGE_MAX_SIZE / FLASH_PAGE_SIZE;

    sendByte(nbrOfPage);

    for(uint16_t eraseCount = 0; eraseCount < nbrOfPage; eraseCount++) {

        ledStatus(eraseCount % 2);

        if(eraseDev(IMAGE_DEST_ADDR + FLASH_PAGE_SIZE * eraseCount)) {
            status = iapError;
            break;
        }
        sendByte(ACK);
    }

    ledStatus(0);
    return status;
}

static uint16_t parseCMD() {

    char cmd[16] = {0};

    while(1) {
        readString(cmd, 16);
        if(strcmp(cmd, "UPDATE\n") == 0) {
            return JUMP_UPDATE;
        } else if(strcmp(cmd, "UPLOAD\n") == 0) {
            return JUMP_UPLOAD;
        } else if(strcmp(cmd, "ERASE\n") == 0) {
            return JUMP_ERASE;
        } else if(strcmp(cmd, "JUMP2APP\n") == 0) {
            return IMAGE_READY;
        }
    }
}

void XYModem() {

    usartInit();
    ledInit();
    systickInt();

    uint16_t flag = readFlag();
    if(flag == IMAGE_READY)
        flag = JUMP_CMD;

    while(1) {
        switch(flag) {
            case IMAGE_OK:
            case IMAGE_READY: {
                jump2App();
                flag = JUMP_CMD;
                break;
            }

            case JUMP_UPDATE: {
                flag = JUMP_CMD;
                unlockDev();
                if(!updateImage()) {
                    flag = JUMP_APP;
                }
                writeFlag(flag);
                lockDev();
                break;
            }
            case JUMP_UPLOAD: {
                flag = JUMP_CMD;
                break;
            }
            case JUMP_ERASE: {
                unlockDev();
                eraseImage();
                flag = JUMP_CMD;
                break;
            }
            case JUMP_APP: {
                flag = IMAGE_READY;
                break;
            }
            default: {
                flag = parseCMD();
                break;
            }
        }
        writeFlag(flag);
    }
}


