#ifndef XYMODEM_H
#define XYMODEM_H
#include <QEventLoop>
#include <QThread>
#include <QTimer>

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

class XYmodem : public QObject {

    Q_OBJECT

  public:
    XYmodem(QObject *parent = nullptr);
    ~XYmodem();

    typedef enum {
        // clang-format off
        SOH               = (0x01),     /* 128字节数据包帧头 */
        STX               = (0x02),     /* 1024字节数据包帧头 */
        EOT               = (0x04),     /* 传输结束 */
        ACK               = (0x06),     /* 应答信号 */
        NAK               = (0x15),     /* 非应答信号 */
        CA                = (0x18),     /* 两个连续CA以终止传输 */
        CRC16             = (0x43),     /* 'C' == 0x43, 请求数据 */
        ABORT1            = (0x41),     /* 'A' == 0x41, 用户请求中止 */
        ABORT2            = (0x61),     /* 'a' == 0x61, 用户请求中止 */

        PACKET_HEAD       = (3),
        PACKET_TAIL       = (2),
        PACKET_OVERHEAD   = (5),
        PACKET_SIZE       = (128),
        PACKET_1K_SIZE    = (1024),

        MAX_ERRORS        = (5),
        NAK_TIMEOUT       = (1500),
        // clang-format on
    } xymodem;

    typedef enum {
        progress = 0,
        updateOK,
        uploadOK,
        eraseOK,
        updateFail,
        uploadFail,
        eraseFail,
        updateCancel,
        uploadCancel,
        disconnect,
    } xymodemState;

  signals:
    void response();
    void push_txData(const QByteArray &data);
    void push_xymodemState(xymodemState state, quint16 progressValue, quint16 error);

  public slots:
    void get_stopCMD(bool stopMode) {
        m_responseEventLoop->quit();
        m_currentRun = 5 + stopMode;
    }

    void get_exitCMD() {
        m_isSessionEnd = true;
        m_responseEventLoop->quit();
        QThread::currentThread()->quit();
    }

    void get_updateCMD(bool isXModem, const QString &fileName, const QByteArray &data);
    void get_uploadCMD();
    void get_eraseCMD();

    void get_rxData(const QByteArray &data);

  private:
    QTimer *m_timeoutTimer;
    QEventLoop *m_responseEventLoop;

    bool m_isSessionEnd = 0;
    quint8 m_currentRun = 0;
    QByteArray m_responsePacket;

    quint16 packet2CRC(const QByteArray &packet);
    quint16 packet2CheckSum(const QByteArray &packet);
    quint16 data2Packet(bool isXModem, QByteArray &packet, const QByteArray &data, quint32 &dataIndex, const quint32 &residualSize, const quint8 &number);
};

#endif