#ifndef XYMODEM_H
#define XYMODEM_H
#include "interface.h"

/*                          XYModem帧格式                            /
/----------------------------  起始帧  ------------------------------/
/ SOH / 0x00 / 0xFF / name / 0x00 / size / 0x00 / Mode / CRCH / CRCL /
1.SOH:帧头
2.0x00 0xFF:固定数据
3.name: 固件名
4.size: 固件大小
5.Mode: X/YModem
6.CRCH: 校验高8位
7.CRCL: 校验低8位
8.Mode固定在帧的第130位。
9.CRC根据协议不同使用的校验方式不一样。

/----------------------------  数据帧  -----------------------------/
/  SOH/STX  /  NUM  /  ~NUM  /  Data0...DataN  /  CRCH  /  CRCL  /
1.SOH/STX:帧头
    1.1 SOH:128字节数据包
    1.2 STX:1024字节数据包
2.NUM:序号
3.~NUM:序号反码
4.Data:数据包
5.CRCH: 校验高8位
6.CRCL: 校验低8位
7.XModem固定以SOH帧格式发送。
8.YModem:
    8.1 剩余数据等于1024，使用STX帧格式发送。
    8.2 剩余数据时小于1024，使用SOH帧格式发送。

/----------------------------  结束帧  -----------------------------/
/   SOH  /   0x00   /   0xFF   /   0x00   /   0x00   /     0x00    /
1.结束帧固定格式。

/---------------------------  更新通信流程  -------------------------/
/                                                                  /
/---------  上位机  -----------------------------  下位机  --------- /
/            指令                                                   /
/                                                 CRC16            /
/           起始帧                                                  /
/                                                  ACK             /
/                                                 CRC16            /
/           数据帧0                                                 /
/                                                  ACK             /
/           数据帧1                                                 /
/                                                  ACK             /
/           数据帧2                                                 /
/                                            NAK(异常情况，数据重传)  /
/           数据帧2                                                 /
/                                                  ACK             /
/           数据帧3                                                 /
/                                                  ACK             /
/            EOT                                                   /
/                                                  ACK             /
/           结束帧                                                  /
/                                                  ACK             /
*/

#define SOH    (0x01) /* 128字节数据包帧头 */
#define STX    (0x02) /* 1024字节数据包帧头 */
#define EOT    (0x04) /* 传输结束 */
#define ACK    (0x06) /* 应答信号 */
#define NAK    (0x15) /* 非应答信号 */
#define CA     (0x18) /* 两个连续CA以终止传输 */
#define CRC16  (0x43) /* 'C' == 0x43, 请求数据 */
#define ABORT1 (0x41) /* 'A' == 0x41, 用户请求中止 */
#define ABORT2 (0x61) /* 'a' == 0x61, 用户请求中止 */

#define PACKET_HEADER_SIZE   (3)
#define PACKET_TRAILER_SIZE  (2)
#define PACKET_OVERHEAD_SIZE (5)
#define PACKET_SIZE          (128)
#define PACKET_1K_SIZE       (1024)

#define PACKET_DATA_INDEX (3)
#define PACKET_NUM_INDEX  (1)
#define PACKET_CNUM_INDEX (2)
#define PACKET_CRCH_INDEX (3)
#define PACKET_CRCL_INDEX (4)

#define NAME_MAX_LENGTH (64)
#define SIZE_MAX_LENGTH (16)

#define MAX_RESEND   (5)
#define MAX_REQUEST  (5)
#define NAK_TIMEOUT  (10)

#define IMAGE_OK    (0x4F4B)
#define IMAGE_READY (0x5259)

#define JUMP_CMD    (0x0000)
#define JUMP_UPDATE (0x0001)
#define JUMP_UPLOAD (0x0020)
#define JUMP_ERASE  (0x0400)
#define JUMP_APP    (0x8000)

typedef enum {
    receiveOK = 0,
    receiveError,
    receiveEOT,
    receiveAbort,
    receiveTimeout,
} receiveStatus;

typedef enum {
    iapOK = 0,
    iapError,
    iapAbort,
    iapMaxRequest,
    iapMaxResend,
    iapImageLimit,
    iapFlashError,
} iapStatus;

void XYModem(void);

extern uint8_t timeout;

#endif
